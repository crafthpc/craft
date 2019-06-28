# {{{ initialize_search
def initialize_search
    $start_time = Time.now

    # create search config files
    File.new($settings_fn, "w").close
    File.new($workqueue_fn, "w").close
    File.new($inproc_fn, "w").close
    File.new($tested_fn, "w").close
    File.new($mainlog_fn, "w").close

    # create run folder
    FileUtils.rm_rf($run_path)
    Dir.mkdir($run_path)

    # create folders for saving configurations
    FileUtils.rm_rf($best_path)
    Dir.mkdir($best_path)
    FileUtils.rm_rf($passed_path)
    Dir.mkdir($passed_path)
    FileUtils.rm_rf($failed_path)
    Dir.mkdir($failed_path)
    FileUtils.rm_rf($aborted_path)
    Dir.mkdir($aborted_path)

    # print intro text
    if $strategy_name == 'rprec' then
        puts "Initializing reduced-precision search. Options:"
        puts "  split_threshold=#{$rprec_split_threshold}"
        puts "  runtime_pct_threshold=#{$rprec_runtime_pct_threshold}"
        puts "  skip_app_level=#{$rprec_skip_app_level}"
    else
        puts "Initializing #{$variable_mode ? "variable" : "binary"} search:  strategy=#{$strategy_name}"
        if $mixed_use_rprec then
            puts "  mixed_use_rprec=#{$mixed_use_rprec}"
            puts "  split_threshold=#{$rprec_split_threshold}"
        end
    end

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
        if $variable_mode then
            puts "\nERROR: Must provide initial configuration ('-c <file>') in variable mode"
            exit
        else
            # run fpconf to generate config
            IO.popen("#{$fpconf_invoke} #{$fpconf_options} #{$binary_path}") do |io|
                io.each_line do |line|
                    orig_cfg.puts line
                end
            end
            Dir.glob("fpconf.log").each do |f| File.delete(f) end
        end
    end
    orig_cfg.close
    puts "Done."
    merge_additional_configs

    # read all program control points
    # depends on initial configuration being present
    initialize_program

    # initial performance run
    # depends on $program being initialized
    print "Performing baseline performance test ... "
    $stdout.flush
    if !run_baseline_performance then
        puts "Baseline performance test failed verification!"
        puts "Aborting search."
        exit
    end
    puts "Done.  [Base error: #{$baseline_error}  walltime: #{format_time($baseline_runtime.to_f)}]"

    # initial profiling run (if not in variable mode)
    # depends on $program being initialized
    if not $variable_mode then
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
    end

    # initialize strategy object
    # depends on $program being initialized
    print "Initializing search strategy ... "
    initialize_strategy
    puts "Done.  [#{$total_candidates} candidates]"

    # save project settings to file
    save_settings

    # initialize work queue
    load_cached_configs
    configs = $strategy.build_initial_configs
    add_to_workqueue_bulk(configs)
    $max_queue_length = get_workqueue_length

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
    wait_time = 1   # exponential backoff for queue monitoring
    while get_workqueue_length + get_inproc_length > 0 do

        # check for any completed configurations
        get_inproc_configs.select { |cfg| not is_config_running?(cfg) }.each do |cfg|

            get_run_results(cfg)

            # print output
            msg = "Finished testing config #{cfg.shortlabel}: #{cfg.attrs["result"]}"
            if cfg.attrs["result"] != $RESULT_ERROR then
                msg += "\n     Walltime: #{format_time(cfg.attrs["runtime"].to_f)}"
                if $variable_mode then
                    msg += "   Speedup: %.1fx"%[$baseline_runtime / cfg.attrs["runtime"]]
                else
                    msg += "   Overhead: %.1fx"%[cfg.attrs["runtime"] / $baseline_runtime]
                end
                msg += "  Error: %g"%[cfg.attrs["error"]]
            end
            queue_length = get_workqueue_length
            msg += "  [Queue length: #{"%3d" % (queue_length)}]  "
            $max_queue_length = queue_length if queue_length > $max_queue_length
            puts msg
            add_to_mainlog(msg)


            # update data structures and invoke strategy to update search
            add_tested_config(cfg)
            remove_from_inproc(cfg)
            $strategy.handle_completed_config(cfg)

            # reset wait interval
            wait_time = 1
        end

        # start new configurations if possible
        while get_workqueue_length > 0 and ($max_inproc < 0 or
                                            get_inproc_length < $max_inproc) do
            cfg = get_next_workqueue_item

            # check list of already-tested configs
            cached = false
            $cached_configs.each do |c|
                if c.cuid == cfg.cuid then

                    # if we've already run this test, no need to run it again
                    result = c.attrs["result"]
                    puts "Using cached result for config #{cfg.shortlabel}: #{result}"
                    cfg.attrs["cached"] = "yes"
                    cfg.attrs["result"]  = c.attrs["result"]
                    cfg.attrs["error"]   = c.attrs["error"]
                    cfg.attrs["runtime"] = c.attrs["runtime"]
                    cached = true
                    add_tested_config(cfg)
                end
            end

            # run the test and update queue
            if not cached then
                puts "Testing config #{cfg.shortlabel}."
                run_config(cfg)
                add_to_inproc(cfg)
            end
        end

        sleep wait_time
        wait_time *= 2

    end
end # }}}
# {{{ finalize_search
def finalize_search

    if File.exist?($final_path) then
        # another process is already finalizing
        puts "Finalization already underway."
        return
    end

    # queue status output
    puts ""
    puts "Candidate queue exhausted.  [Max queue length: ~#{$max_queue_length} item(s)]"

    # generate and test final config file
    print "Generating and final configuration ... "
    $stdout.flush
    rebuild_final_config
    puts "Done."

    # try the final config (and keep results)
    puts "Testing final configuration ... "
    run_config($final_config, true)
    FileUtils.cp_r("#{$run_path}#{$final_config.filename(false)}", $final_path[0...-1])

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
