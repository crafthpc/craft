# {{{ initialize_search
def initialize_search
    # supervisor initialization
    
    $start_time = Time.now

    # create search config files
    File.new($settings_fn, "w").close
    File.new($workqueue_fn, "w").close
    File.new($inproc_fn, "w").close
    File.new($tested_fn, "w").close
    File.new($mainlog_fn, "w").close

    if $strategy_name == 'rprec' then
        puts "Initializing reduced-precision search. Options:"
        puts "  split_threshold=#{$rprec_split_threshold}"
        puts "  runtime_pct_threshold=#{$rprec_runtime_pct_threshold}"
        puts "  skip_app_level=#{$rprec_skip_app_level}"
    else
        puts "Initializing standard search:  strategy=#{$strategy_name}"
        puts "  mixed_use_rprec=#{$mixed_use_rprec}"
        if $mixed_use_rprec then
            puts "  split_threshold=#{$rprec_split_threshold}"
        end
    end

    # initial performance run
    print "Performing baseline performance test ... "
    $stdout.flush
    if !run_baseline_performance then
        puts "Baseline performance test failed verification!"
        puts "Aborting search."
        exit
    end
    puts "Done.  [Base error: #{$baseline_error}  walltime: #{format_time($baseline_runtime.to_f)}]"

    # generate initial configuration by calling fpconf
    # and initialize global data structures
    print "Generating initial configuration ... "
    $stdout.flush
    orig_cfg = File.open($orig_config_fn, "w")
    if $initial_cfg_fn.length > 0 && File.exists?($initial_cfg_fn) then
        # a base config was given; just read it
        print "(using base: #{$initial_cfg_fn}) "
        $stdout.flush
        IO.foreach($initial_cfg_fn) do |line|
            orig_cfg.puts line
        end
    else
        if $typeforge_mode then
            # look for "craft_t" declarations to generate initial config
            orig_cfg.puts "{\n  \"version\": 1,\n  \"tool_id\": \"CRAFT\","
            orig_cfg.puts "  \"actions\": ["
            Dir.glob("*.cpp") do |fn| add_variables_to_config_file(fn, orig_cfg) end
            orig_cfg.puts "  ]\n}"
        else
            # run fpconf to generate config
            IO.popen("#{$fpconf_invoke} #{$fpconf_options} #{$binary_path}") do |io|
                io.each_line do |line|
                    orig_cfg.puts line
                end
            end
        end
    end
    Dir.glob("fpconf.log").each do |f| File.delete(f) end
    orig_cfg.close
    puts "Done."

    # read all program control points
    # depends on initial configuration being present
    initialize_program

    # initial profiling run
    # depends on $program being initialized
    print "Performing profiling test ... "
    $stdout.flush
    if not File.exist?($prof_path) and !run_profiler then
        puts "Baseline profiling test failed verification!"
        #puts "Aborting search."        # not really necessary; we can run
        #exit                           # without profiling data
    elsif File.exist?($prof_path) then
        files = Dir.glob("#{$prof_path}/*-c_inst*.log")
        if files.size > 0 then
            $prof_log_fn = files.first
        else
            puts "\nError: profile folder exists but there is no c_inst log file. Run \"craft clean\" and restart to generate it."
        end
        puts "Cached."
    else
        puts "Done."
    end
    read_profiler_data

    # initialize strategy object
    # depends on $program being initialized
    print "Initializing search strategy ... "
    initialize_strategy
    puts "Done.  [#{$total_candidates} candidates]"

    # save project settings to file (for workers to load)
    save_settings

    # initialize work queue
    configs = $strategy.build_initial_configs
    add_to_workqueue_bulk(configs)
    $max_queue_length = get_workqueue_length

    # create folders for saving configurations
    FileUtils.rm_rf($best_path)
    Dir.mkdir($best_path)
    FileUtils.rm_rf($passed_path)
    Dir.mkdir($passed_path)
    FileUtils.rm_rf($failed_path)
    Dir.mkdir($failed_path)
    FileUtils.rm_rf($aborted_path)
    Dir.mkdir($aborted_path)

end # }}}
# {{{ resume_lower_search
def resume_lower_search

    # figure out new base type
    old_base_type = $base_type
    new_base_rank = $TYPE_RANK[old_base_type]+1
    if !$TYPE_RRANK.has_key?(new_base_rank) then
        puts "Already at lowest base type."
    else
        puts "Lowering base type from #{$base_type} to #{$TYPE_RRANK[new_base_rank]}."
        $base_type = $TYPE_RRANK[new_base_rank]
        $strategy.base_type = $base_type
    end


    # look for new configurations
    configs = Array.new
    already_added = Hash.new(false)        # prevents adding identical cuids twice
    old_configs = get_tested_configs
    if $strategy_name == "rprec" then
        $strategy.reload_bounds_from_results(old_configs)
    end
    old_configs.each do |cfg|
        if is_single_base(cfg, old_base_type) and 
                (cfg.attrs["result"] != $RESULT_PASS or $strategy_name == "rprec") then
            $strategy.split_config(cfg).each do |c|
                if not already_added[c.cuid] then
                    configs << c
                    already_added[c.cuid] = true
                end
            end
        end
    end

    # initialize work queue
    add_to_workqueue_bulk(configs)
    $max_queue_length = get_workqueue_length

end # }}}
# {{{ run_main_search_loop
def run_main_search_loop
    cfg = get_next_workqueue_item

    # main search loop
    while !cfg.nil? do

        add_to_inproc(cfg)

        # queue status output
        queue_length = get_workqueue_length
        $status_buffer += "[Queue length: #{"%3d" % (queue_length+1)}]  "
        $max_queue_length = queue_length if queue_length > $max_queue_length

        # check list of already-tested configs
        cached = false
        load_cached_configs
        $cached_configs.each do |c|
            if c.cuid == cfg.cuid then

                # if we've already run this test, no need to run it again
                result = c.attrs["result"]
                puts "Using cached result for config #{cfg.label}: #{result}"
                cfg.attrs["cached"] = "yes"
                cfg.attrs["result"]  = c.attrs["result"]
                cfg.attrs["error"]   = c.attrs["error"]
                cfg.attrs["runtime"] = c.attrs["runtime"]
                cached = true
            end
        end

        # run the test and update queue
        if not cached then
            puts "Testing config #{cfg.label} ..."
            result = run_config(cfg)
        end
        add_tested_config(cfg)
        remove_from_inproc(cfg)
        rebuild_final_config

        add_children = true
        if $strategy_name == "rprec" then

            # always add children, but reload results first
            # so the strategy can make an informed decision
            $strategy.reload_bounds_from_results(get_tested_configs)

        elsif $strategy_name == "ddebug" then

            # supervisor thread handles splitting
            add_children = false

        else

            # check to see if we passed
            if result == $RESULT_PASS then
                add_children = false
            end

            # check to see if we're at the base type; stop if so
            if is_single_base(cfg, $base_type) then
                add_children = false
            end
        end

        # add any children we need to test
        if add_children then
            configs = $strategy.split_config(cfg)
            configs.each do |child|
                add_child = true

                # skip single-exception configs where the only exception
                # is lower than the base type; currently there is no
                # situation where we will encounter mixed-level configs
                if child.exceptions.keys.size == 1 then
                    pt = $program.lookup_by_uid(child.exceptions.keys.first)
                    if !pt.nil? and $TYPE_RANK[pt.type] > $TYPE_RANK[$base_type] then
                        add_child = false
                    end
                end

                # skip non-executed instructions if desired; assume they
                # will pass
                if $skip_nonexecuted and child.attrs["cinst"].to_i == 0 then
                    puts "Skipping non-executed config #{child.label}."
                    add_to_mainlog("    Skipping non-executed config #{child.label}.")
                    child.attrs["result"] = $RESULT_PASS
                    child.attrs["skipped"] = "yes"
                    if $strategy_name == "rprec" then
                        child.precisions.each_key do |k|
                            child.precisions[k] = 0
                        end
                    end
                    calculate_pct_stats(child)
                    add_tested_config(child)
                    rebuild_final_config
                    add_child = false
                end

                if add_child then
                    add_to_workqueue(child)
                    puts "Added config #{child.label} to workqueue."
                end
            end
        end

        cfg = get_next_workqueue_item
    end
end # }}}
#{{{ worker thread helper functions (can be used by custom supervisors)
def get_dead_workers
    #print "Checking for dead workers ... "
    dead_workers = Array.new
    $workers.each_pair do |job, dir|
        if job < $num_workers then
            # this job has never actually been started
            # (pid < $num_workers)
            dead_workers << [job, dir]
        else
            cmd = "ps -o s= p #{job.to_i}"
            stat = `#{cmd}`
            stat.chomp!
            if stat != "S" then
                dead_workers << [job, dir]
            end
        end
    end
    #puts "#{dead_workers.size} dead worker(s) found."
    return dead_workers
end
def restart_dead_workers(dead_workers)
    [get_workqueue_length, dead_workers.size].min.times do |i|
        job,dir = dead_workers.pop
        $workers.delete(job)
        id = File.basename(dir)
        if id =~ /worker(\d+)/ then
            id = $1
        else
            id = id[$binary_name.length+1,id.length-$binary_name.length-1]
        end
        FileUtils.rm_rf(dir)
        Dir.mkdir(dir)
        job = fork do
            exec "cd #{dir} && #{$self_invoke} worker #{id} #{$search_path}"
        end
        #puts "Respawned worker thread #{id} (pid=#{job})."
        $workers[job] = dir
    end
end
def wait_for_workers
    $workers.each_pair do |job, dir|
        begin
            Process.wait(job.to_i)
        rescue
        end
        FileUtils.rm_rf(dir)
    end
end
# }}}
# {{{ run_main_supervisor_loop
def run_main_supervisor_loop

    # keep going until there are no more configs to test
    while get_workqueue_length > 0 or get_inproc_length > 0 do

        keep_running = true
        while keep_running do

            $max_queue_length = get_workqueue_length if get_workqueue_length > $max_queue_length

            # check for dead worker processes
            dead_workers = get_dead_workers

            if dead_workers.size == $num_workers and get_workqueue_length == 0 then

                # everyone's done and there's nothing else in the queue
                keep_running = false

            elsif get_workqueue_length > 0 then

                # there's still stuff to do; restart dead workers
                restart_dead_workers(dead_workers)

            end

            # wait a while
            sleep 5
        end

        # clean up any workers and directories
        wait_for_workers

        # if the workqueue is empty and all the workers are dead but there
        # are still configs in the inproc queue, then something was aborted;
        # move any unfinished configs back to the workqueue and restart
        if get_inproc_length > 0 then
            puts "Unfinished jobs; restarting workers ..."
            move_all_inproc_to_workqueue
        end
    end
end # }}}
# {{{ finalize_search
def finalize_search

    if File.exist?($final_path) then
        # another process is already finalizing
        puts "Finalization already underway."
        return
    else
        Dir.mkdir($final_path)
    end

    # queue status output
    puts ""
    puts "Candidate queue exhausted.  [Max queue length: ~#{$max_queue_length} item(s)]"

    # generate final config file
    print "Generating final configuration ... "
    $stdout.flush
    rebuild_final_config
    FileUtils.cp($final_config_fn, $final_path)
    puts "Done."

    # try the final config (and keep results)
    puts "Testing final configuration ... "
    Dir.chdir($final_path)
    if $run_final_config then
        run_config_file($final_config_fn, true, $final_config.label)
    end
    Dir.chdir($search_path)

    # start generating final report
    report = build_best_report(10, true)

    # finalize report
    elapsed = Time.now.to_f - $start_time.to_f
    summary = get_tested_configs_summary
    report << " "
    report << "Total candidates:          #{"%11d" % $total_candidates}"
    report << "Total configs tested:      #{"%11d" % summary["total"]}"
    report << "  Total executed:          #{"%11d" % summary["tested"]}"
    report << "  Total cached:            #{"%11d" % summary["cached"]}"
    report << "  Total passed:            #{"%11d" % summary["pass"]}"
    report << "    Total skipped:         #{"%11d" % summary["skipped"]}"
    report << "  Total failed:            #{"%11d" % summary["fail"]}"
    report << "  Total aborted:           #{"%11d" % summary["error"]}"
    report << "Done.  [Total elapsed walltime: #{format_time(elapsed)}]"
    report << " "
    add_to_mainlog(report.join("\n"))
    puts report.join("\n")

    # wtf = wall time file
    # what did *you* think it meant?!?!?
    #
    wtf = File.new($walltime_fn,"w")
    wtf.puts elapsed.to_s
    wtf.close

end # }}}
