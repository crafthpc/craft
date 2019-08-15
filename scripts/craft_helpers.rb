# {{{ initialization


def parse_command_line
    $main_mode = ARGV.shift.strip.downcase
    if $main_mode == "search" then
        $main_mode = "start"
    end
    if not ["start","resume","status","clean"].include?($main_mode) then
        puts "Invalid mode: #{$main_mode}"
        exit
    end
    parsed_binary = false
    while ARGV.size > 0 do
        opt = ARGV.shift
        if $main_mode == "start" then
            if opt == "-j" then
                # parallel: "-j 4" variant
                $max_inproc = ARGV.shift.to_i
            elsif opt =~ /^-j/ then
                # parallel: "-j4" variant
                $max_inproc = opt[2,opt.length-2].to_i
            elsif opt == "-k" then
                # keep all temporary files
                $keep_all_runs = true
            elsif opt == "-d" then
                # debug mode
                $status_preferred = ARGV.shift
                $status_alternate = $STATUS_IGNORE
                $fpconf_options = $FPCONF_OPTS[$status_preferred]
            elsif opt == "-D" then
                # debug mode
                $status_preferred = ARGV.shift
                $status_alternate = ARGV.shift
                $fpconf_options = $FPCONF_OPTS[$status_preferred]
            elsif opt == "-c" then
                # initial configuration
                $initial_cfg_fn = ARGV.shift
            elsif opt == "-C" then
                # initial fpconf options
                $fpconf_options = ARGV.shift
            elsif opt == "-A" then
                # additional advisory configuration (e.g., ADAPT)
                $addt_cfg_fns << ARGV.shift
            elsif opt == '-m' then
                # memory-based analysis
                $fpconf_options = "-c --svinp mem_double "
            elsif opt == '-r' then
                # reduced-precision analysis
                $status_preferred = $STATUS_RPREC
                $status_alternate = $STATUS_IGNORE
                $fpconf_options = "-c --rprec 52"
                $strategy_name = "rprec"
            elsif opt == '-a' then
                # skip non-executed configs
                $skip_nonexecuted = false
            elsif opt == '-N' then
                # fortran mode
                $fortran_mode = true
            elsif opt == '-V' then
                # variable mode
                $variable_mode = true
                $base_type = $TYPE_VARIABLE
                $disable_queue_sort = true
                $skip_nonexecuted = false   # no cinst data in variable mode
            elsif opt == '-b' then
                # stop splitting at basic blocks
                $base_type = $TYPE_BASICBLOCK
            elsif opt == '-f' then
                # stop splitting at functions
                $base_type = $TYPE_FUNCTION
            elsif opt == '-s' then
                # strategy
                $strategy_name = ARGV.shift
            elsif opt == '--mixed-use_rprec' then
                # precision split threshold for reduced precision simulation of
                # mixed precision
                $mixed_use_rprec = true
                $rprec_split_threshold = ARGV.shift.to_i
            elsif opt == '--rprec-split_threshold' then
                # precision split threshold for reduced precision
                $rprec_split_threshold = ARGV.shift.to_i
            elsif opt == '--rprec-runtime_pct_threshold' then
                # runtime percentage threshold for reduced precision
                $rprec_runtime_pct_threshold = ARGV.shift.to_f
            elsif opt == '--rprec-skip_app_level' then
                # runtime percentage threshold for reduced precision
                $rprec_skip_app_level = true
            elsif opt == '-S' then
                # disable workqueue sorting
                $disable_queue_sort = true
            elsif opt == '-t' then
                # set trial count
                $num_trials = ARGV.shift.to_i
            elsif opt == '-T' then
                # set timeout interval
                $timeout_limit = ARGV.shift.to_i
            elsif opt == '-g' then
                # group by label
                $group_by_labels.concat(ARGV.shift.split(","))
            elsif opt == '-M' then
                # merge overlapping groups
                $merge_overlapping_groups = true
            elsif opt == '-J' then
                # job submission system
                $job_mode = ARGV.shift
            elsif !parsed_binary then
                if not File.exists?(opt) then
                    puts "Cannot find target binary: #{opt}"
                    exit
                end
                $binary_name = File.basename(opt)
                $binary_path = File.expand_path(opt)
                parsed_binary = true
            else
                if parsed_binary then
                    puts "Invalid parameter after target binary: #{opt}"
                    exit
                else
                    puts "Invalid resume parameter: #{opt}"
                    exit
                end
            end
        elsif $main_mode == "resume" then
            if opt == "-l" then
                $resume_lower = true
            else
                puts "Invalid resume option: #{opt}"
                exit
            end
        else
            puts "Invalid parameter: #{opt}"
            exit
        end
    end
    if !$variable_mode and $main_mode == "start" and !parsed_binary then
        puts "No binary included on command-line."
        exit
    end
end

def merge_additional_configs
    return if $addt_cfg_fns.size == 0
    main_cfg = JSON.parse(IO.read($orig_config_fn))
    $addt_cfg_fns.each do |fn|
        begin
            cfg = JSON.parse(IO.read(fn))
            if cfg.has_key?("tool_id") and cfg["tool_id"] == "ADAPT" then
                print "Merging ADAPT output #{fn} ... "
                if not cfg.has_key?("actions") or cfg["actions"].size == 0 then
                    puts "WARNING: No recommendations!"
                    next
                end

                # discard all non-replacement actions
                cfg["actions"].select! { |a| a["action"] == "change_var_basetype" }
                print "(#{cfg["actions"].size} recommendations) "

                # create handle => action mapping for ADAPT actions
                adapt_actions = Hash.new
                cfg["actions"].each { |a| adapt_actions[a["handle"]] = a }

                # remove all variables that are not in the ADAPT file
                if $strategy_name != "ddebug" then
                    main_cfg["actions"].select! { |a| adapt_actions.has_key?(a["handle"]) }
                end

                # merge all ADAPT information into main configuration
                main_cfg["actions"].each do |a|
                    if a.has_key?("handle") and adapt_actions.has_key?(a["handle"]) then
                        a["error"] = adapt_actions[a["handle"]]["error"]
                        a["ivcount"] = adapt_actions[a["handle"]]["dynamic_assignments"]
                    end
                end
                puts "Done."
            end
        rescue => e
            puts "Error: Unable to read additional configuration #{fn}"
            puts e
        end
    end
    File.open($orig_config_fn, "w") do |f|
        f.puts(JSON.pretty_generate(main_cfg))
    end
end

def initialize_program
    config = IO.read($orig_config_fn)
    begin
        $program = read_json_config(JSON.parse(config))
    rescue => e
        $program = read_craft_config(config)
    end
    if $program.nil? then
        puts "Unable to initialize program control structure."
        puts "Aborting search."
        exit
    end
    $program.build_lookup_hash
end

def read_json_config(cfg)
    midx = 0
    fidx = 0
    vidx = 0
    program = PPoint.new("#{$TYPE_APPLICATION} #1: 0", $TYPE_APPLICATION, $STATUS_NONE)
    modules = Hash.new
    modules["DEFAULT"] = PPoint.new("#{$TYPE_MODULE} ##{midx}", $TYPE_MODULE, $STATUS_NONE)
    modules["DEFAULT"].attrs["desc"] = "DEFAULT"
    program.children << modules["DEFAULT"]
    functions = Hash.new    # module name => function name => function
    functions["DEFAULT"] = Hash.new
    if cfg.has_key?("actions") then
        cfg["actions"].each do |a|
            if a.has_key?("action") and a["action"] == "change_var_basetype" and
                    a.has_key?("to_type") and a["to_type"] == "float" and
                    a.has_key?("handle") then

                # find enclosing module (or just the default module)
                if a.has_key?("source_info") and a["source_info"] =~ /([^\/]*\.\w+)(:\d+)*$/ then
                    modname = $1
                    if modules.has_key?(modname) then
                        mod = modules[modname]
                    else
                        midx += 1
                        mod = PPoint.new("#{$TYPE_MODULE} ##{midx}: 0", $TYPE_MODULE, $STATUS_NONE)
                        mod.attrs["desc"] = modname
                        modules[modname] = mod
                        functions[modname] = Hash.new
                        program.children << mod
                    end
                else
                    modname = "DEFAULT"
                    mod = modules[modname]
                end

                # find enclosing function (if any)
                func = nil
                if a.has_key?("scope") and a["scope"] =~ /function:<(.*)>/ then
                    funcname = $1
                    if functions[modname].has_key?(funcname) then
                        func = functions[modname][funcname]
                    else
                        fidx += 1
                        func = PPoint.new("#{$TYPE_FUNCTION} ##{fidx}: 0", $TYPE_FUNCTION, $STATUS_NONE)
                        func.attrs["desc"] = funcname
                        functions[modname][funcname] = func
                        mod.children << func
                    end
                end

                # create variable point
                vidx += 1
                var = PPoint.new("#{$TYPE_VARIABLE} ##{vidx}: 0", $TYPE_VARIABLE, $STATUS_CANDIDATE)
                var.attrs["addr"] = a["handle"]
                if a.has_key?("name") then
                    var.attrs["desc"] = a["name"]
                else
                    var.attrs["desc"] = a["handle"]
                end
                var.attrs["desc"] = var.attrs["desc"][/([^:]*)$/, 1]   # strip name from FQN
                var.attrs["scope"] = a["scope"] if a.has_key?("scope")
                var.attrs["source_info"] = a["source_info"] if a.has_key?("source_info")
                var.attrs["error"] = a["error"] if a.has_key?("error")
                var.attrs["ivcount"] = a["ivcount"] if a.has_key?("ivcount")
                var.attrs["labels"] = a["labels"] if a.has_key?("labels")
                # TODO: incorporate "uid" field information if present

                if func.nil? then
                    mod.children << var
                else
                    func.children << var
                end
            end
        end
    end
    return program
end

def read_craft_config(cfg)
    program = nil
    mod     = nil
    func    = nil
    bblk    = nil
    vars    = {}   # id => var (used to update handles later in parsing)
    prolog  = []
    cfg.each_line do |line|
        if line =~ /(APPLICATION #\d+: [x0-9A-Fa-f]+) \"(.*)\"/ then
            program = PPoint.new($1, $TYPE_APPLICATION, line[1,1])
            program.attrs["desc"] = $2
            mod  = nil
            func = nil
            bblk = nil
        elsif line =~ /(MODULE #\d+: [x0-9A-Fa-f]+) \"(.*)\"/ then
            mod = PPoint.new($1, $TYPE_MODULE, line[1,1])
            mod.attrs["desc"] = $2
            program.children << mod if program != nil
            func = nil
            bblk = nil
        elsif line =~ /(FUNC #\d+: [x0-9A-Fa-f]+) \"(.*)\"/ then
            func = PPoint.new($1, $TYPE_FUNCTION, line[1,1])
            func.attrs["desc"] = $2
            mod.children << func if mod != nil
            bblk = nil
        elsif line =~ /(BBLK #\d+: [x0-9A-Fa-f]+)/ then
            bblk = PPoint.new($1, $TYPE_BASICBLOCK, line[1,1])
            bblk.attrs["addr"] = $2
            func.children << bblk if func != nil
        elsif line =~ /(INSN #\d+: ([x0-9A-Fa-f]+)) \"(.*)\"/ then
            insn = PPoint.new($1, $TYPE_INSTRUCTION, line[1,1])
            insn.attrs["addr"] = $2
            insn.attrs["desc"] = $3
            bblk.children << insn if bblk != nil
        elsif line =~ /(VAR #\d+: ([x0-9A-Fa-f]+)) \"(.*)\"/ then
            var = PPoint.new($1, $TYPE_VARIABLE, line[1,1])
            vars[var.id] = var
            var.attrs["addr"] = $2
            var.attrs["desc"] = $3
            bblk.children << var if bblk != nil
            func.children << var if func != nil
            mod.children  << var if func == nil
        else
            prolog << line.chomp
        end
    end
    prolog.each do |line|
        if line =~ /VAR_(\d+)_handle=\"(.+)\"/ then
            vars[$1.to_i].attrs["addr"] = $2
        elsif line =~ /VAR_(\d+)_scope=\"(.+)\"/ then
            vars[$1.to_i].attrs["scope"] = $2
        elsif line =~ /VAR_(\d+)_source_info=\"(.+)\"/ then
            vars[$1.to_i].attrs["source_info"] = $2
        elsif line =~ /VAR_(\d+)_error=\"(.+)\"/ then
            vars[$1.to_i].attrs["error"] = $2
        end
    end
    program.attrs["prolog"] = prolog.join("\n")
    return program
end

def initialize_strategy
    case $strategy_name
    when "simple"
        $strategy = Strategy.new($program, $status_preferred, $status_alternate)
    when "bin_simple"
        $strategy = BinaryStrategy.new($program, $status_preferred, $status_alternate)
    when "exhaustive"
        $strategy = ExhaustiveStrategy.new($program, $status_preferred, $status_alternate)
    when "combinational"
        $strategy = ExhaustiveCombinationalStrategy.new($program, $status_preferred, $status_alternate)
    when "compositional"
        $strategy = CompositionalStrategy.new($program, $status_preferred, $status_alternate)
    when "comp_simple"
        $strategy = SimpleCompositionalStrategy.new($program, $status_preferred, $status_alternate)
    when "rprec"
        $strategy = RPrecStrategy.new($program, $status_preferred, $status_alternate)
        $strategy.split_threshold = $rprec_split_threshold
        $strategy.runtime_pct_threshold = $rprec_runtime_pct_threshold
        $strategy.skip_app_level = $rprec_skip_app_level
    when "ddebug"
        $strategy = DeltaDebugStrategy.new($program, $status_preferred, $status_alternate)
    else
        puts "Unrecognized search strategy \"#{$strategy_name}\""
        puts "Aborting search."
        exit
    end
    $strategy.base_type = $base_type
    if $program.attrs.has_key?("ivcount") then
        $total_candidates = $program.attrs["ivcount"]
    else
        $total_candidates = 0
    end
end

def run_baseline_performance
    perf_cfg = AppConfig.new($PERFCFG_CUID, "baseline", $STATUS_NONE)
    run_config(perf_cfg)
    FileUtils.cp_r("#{$run_path}#{perf_cfg.filename(false)}", $perf_path[0...-1])
    $baseline_error   = perf_cfg.attrs["error"]
    $baseline_runtime = perf_cfg.attrs["runtime"]
    $baseline_casts   = perf_cfg.attrs["casts"]

    # calculate timeout limit if not provided as a command-line argument;
    # baseline runtime (unless it's less than 1 sec) and increase by 50%
    $timeout_limit = (max($baseline_runtime,1.0) * 1.5).ceil if $timeout_limit.nil?

    return perf_cfg.attrs["result"] == $RESULT_PASS
end

def run_profiler
    # there's no equivalent to a profiling pass in variable mode
    return true if $variable_mode

    passed = false
    if not File.exist?($prof_path) then
        Dir.mkdir($prof_path)
    end
    Dir.chdir($prof_path)

    # run mutator
    cmd = "#{$fpinst_invoke} -i --cinst #{$binary_path}"
    Open3.popen3(cmd) do |io_in, io_out, io_err|
        io_out.each_line do |line| end
    end

    # run rewritten mutatee
    cmd = "#{$search_path}#{$craft_driver} #{Dir.getwd}/mutant"
    Open3.popen3(cmd) do |io_in, io_out, io_err|
        io_out.each_line do |line|
            if line =~ /status:\s*(pass|fail)/i then
                if $1 =~ /pass/i then
                    passed = true
                end
            end
        end
    end

    # extract log file name
    if passed then
        $prof_log_fn = "#{Dir.getwd}/#{Dir.glob("*-c_inst*.log").first}"
    end

    Dir.chdir($search_path)
    return passed
end

def read_profiler_data
    pt_by_id = Hash.new

    if File.exists?($prof_log_fn) then

        # read individual instruction counts from log file

        # pass 1: extract ID => point mappings
        IO.foreach($prof_log_fn) do |line|
            if line =~ /instruction id="(\d+)" address="([x0-9a-zA-Z]+)"/ then
                pt_by_id[$1] = $program.lookup_by_addr($2)
            end
        end

        # pass 2: read counts
        cur_count = ""
        IO.foreach($prof_log_fn) do |line|
            if line =~ /priority="(\d+)" type="InstCount"/ then
                cur_count = $1
            end
            if line=~ /<inst_id>(\d+)<\/inst_id>/ then
                cur_id = $1
                cur_pt = pt_by_id[cur_id]
                if not cur_pt.nil? and cur_pt.orig_status == $STATUS_CANDIDATE then
                    # only count candidates
                    cur_pt.attrs["cinst"] = cur_count
                end
            end
        end
    end

    # propogate instruction counts up to the root
    update_cinst($program)
end

def update_cinst (pt)
    if not pt.attrs.has_key?("cinst") then
        total_cinst = 0
        pt.children.each do |child|
            update_cinst(child)
            total_cinst += child.attrs["cinst"].to_i
        end
        pt.attrs["cinst"] = total_cinst.to_s
    end
end

def group_by_labels (configs)
    return configs if $group_by_labels.size == 0
    grouped_cfgs = []
    groups = Hash.new
    configs.each do |cfg|
        if not cfg.attrs.has_key?("labels") or cfg.attrs["labels"].size == 0 then
            grouped_cfgs << cfg
            next
        end
        added_to_group = nil
        cfg.attrs["labels"].each do |lbl|

            # is there a label that we want to group by?
            group = false
            $group_by_labels.each { |tag| group = true if lbl.start_with?(tag) }
            next unless group

            if groups.has_key?(lbl) then
                if added_to_group.nil? or not $merge_overlapping_groups then
                    # add to existing group config
                    #puts "  adding #{cfg.cuid} to #{lbl}"
                    grp = groups[lbl]
                    cfg.exceptions.each do |k,v|
                        grp.exceptions[k] = v
                    end
                    if grp.attrs.has_key?("cinst") and cfg.attrs.has_key?("cinst") then
                        grp.attrs["cinst"] += cfg.attrs["cinst"]
                    end
                    added_to_group = grp
                elsif $merge_overlapping_groups then
                    # merge groups
                    grp = added_to_group
                    merge_grp = groups[lbl]
                    #puts "  merging #{merge_grp.cuid} into #{grp.cuid}"
                    merge_grp.exceptions.each do |k,v|
                        grp.exceptions[k] = v
                    end
                    if grp.attrs.has_key?("cinst") and merge_grp.attrs.has_key?("cinst") then
                        grp.attrs["cinst"] += merge_grp.attrs["cinst"]
                    end
                    groups[merge_grp.cuid] = grp
                    grouped_cfgs.delete(merge_grp)
                end
            elsif added_to_group.nil? then
                # new group config
                #puts "  adding #{cfg.cuid} to create #{lbl}"
                cfg.cuid = lbl
                cfg.label = lbl
                groups[lbl] = cfg
                grouped_cfgs << cfg
                added_to_group = cfg
            end
        end
        grouped_cfgs << cfg unless added_to_group
    end
    puts "Grouped by labels: #{$group_by_labels.join(",")}" +
         " (before: #{configs.size} configs, after: #{grouped_cfgs.size} configs)"
    return grouped_cfgs
end


# }}}
# {{{ configuration testing


def calculate_pct_stats (cfg)
    pct_icount = 0
    pct_cinst = 0
    cfg.exceptions.keys.each do |k|
        pt = $program.lookup_by_uid(k)
        if !pt.nil? then
            pct_icount += pt.attrs["ivcount"].to_i
            pct_cinst += pt.attrs["cinst"].to_i
            if cfg.exceptions.keys.size == 1 then
                cfg.attrs["single_type"] = pt.type
            end
        end
    end
    pct_icount = (pct_icount.to_f / $program.attrs["ivcount"].to_f * 100.0).to_s
    pct_cinst = (pct_cinst.to_f / $program.attrs["cinst"].to_f * 100.0).to_s
    cfg.attrs["pct_icount"] = pct_icount.to_s
    cfg.attrs["pct_cinst"] = pct_cinst.to_s
end

def get_all_child_pids (pid)

    # build ppid => [pids] lookup table
    children = {}
    `ps -eo pid= -o ppid=`.split("\n").map { |s| s.split }.each do |p,pp|
        children[pp.to_i] = [] if not children.has_key?(pp.to_i)
        children[pp.to_i] << p.to_i
    end

    # breadth-first search through ppid => [pid] relationship
    pids = Set.new
    todo = [pid]
    while todo.size > 0
        p = todo.shift
        next if not children.has_key?(p)
        children[p].each do |c|
            todo << c if not pids.include?(c)
            pids.add(c)
        end
    end
    return pids.to_a
end

def run_config (cfg)
    # synchronous wrapper around the following calls
    start_config(cfg)
    wait_for_config(cfg)
    get_config_results(cfg)
end

def start_config (cfg)

    # create temporary folder
    cfg_path = "#{$run_path}#{cfg.filename(false)}/"
    FileUtils.rm_rf(cfg_path) if File.exists?(cfg_path)
    Dir.mkdir(cfg_path)

    # write actual configuration file
    cfg_file = cfg_path + cfg.filename
    if $variable_mode then
        $program.build_json_file(cfg, cfg_file)
    else
        $program.build_config_file(cfg, cfg_file)
    end

    # create run script
    run_fn = "#{cfg_path}#{$craft_run}"
    out_fn = "#{cfg_path}#{$craft_output}"
    script = File.new(run_fn, "w")
    script.puts "#!/usr/bin/env bash"
    if $job_mode == "slurm" then
        script.puts "#SBATCH -J \"#{cfg.shortlabel.gsub(/"/," ")}\""
        script.puts "#SBATCH -o #{cfg_path}slurm.log"
    end
    script.puts "cd #{cfg_path}"
    if $variable_mode then
        script.print "#{$search_path}#{$craft_builder} #{cfg_file} | tee .build_status"
        script.puts " &>> #{out_fn}"
    else
        script.print "#{$fpinst_invoke} -i #{$fortran_mode ? "-N" : ""}"
        script.print " -c #{cfg_file} #{$binary_path} | tee .build_status"
        script.puts " &>> #{out_fn}"
    end
    script.puts 'if [ -z "$(grep -E "status:\s*error" .build_status)" ]; then'
    $num_trials.times do
        script.print "    #{$search_path}#{$craft_driver}"
        script.print $variable_mode ? "" : " #{cfg_path}mutant"
        script.puts " &>> #{out_fn}"
    end
    script.puts 'fi'

    # finalize and launch the run script
    script.close
    File.chmod(0700, run_fn)
    case $job_mode
    when "exec"
        pid = fork { exec "#{run_fn}" }
        Process.detach(pid)
    when "slurm"
        output = `sbatch #{run_fn} 2>&1`
        if output =~ /Submitted batch job (\d+)/ then
            pid = $1.to_i
        else
            puts "Error submitting batch job: #{output}"
            exit
        end
    end
    cfg.attrs["pid"] = pid
    cfg.attrs["start_time"] = Time.now.to_i
end

def is_config_running? (cfg)
    case $job_mode
    when "exec"
        # TODO: check all child pids as well? (unnecessary so far in testing)
        return `ps -o state= -p #{cfg.attrs["pid"]}`.chomp =~ /R|D|S/
    when "slurm"
        status = `sacct -nDX -o state -j #{cfg.attrs["pid"]}`
        return false if status =~ /BOOT_FAIL|CANCELLED|COMPLETED|DEADLINE|FAILED/
        return false if status =~ /NODE_FAIL|OUT_OF_MEMORY|PREEMPTED|TIMEOUT/
        return true
    end
end

def halt_config (cfg)

    # stop the running job
    pid = cfg.attrs["pid"].to_i
    case $job_mode
    when "exec"
        pids = [pid] + get_all_child_pids(pid)
        pids.each do |p|
            begin
                Process.kill("KILL", p.to_i)
            rescue => e
                # process has already died
            end
        end
    when "slurm"
        `scancel #{pid}`
    end
    cfg.attrs["timeout"] = "yes"
    puts "Configuration #{cfg.shortlabel} failed due to timeout."

    # make sure output file has enough info for get_config_results to work
    outfn = "#{$run_path}#{cfg.filename(false)}/#{$craft_output}"
    File.open(outfn, "a") do |f|
        f.puts "status:  fail"
        f.puts "time: #{$timeout_limit}"
    end
end

def wait_for_config (cfg)
    wait_time = 1
    while is_config_running?(cfg)
        sleep wait_time
        wait_time *= 2
        wait_time = min(wait_time, 60)  # check at least once per minute
    end
end

def get_config_results (cfg)

    # output filename
    outfn = "#{$run_path}#{cfg.filename(false)}/#{$craft_output}"

    # if we're using SLURM, give the filesystem time to catch up
    if $job_mode == "slurm" then sleep 2 while File.size?(outfn).nil? end

    # extract results
    result = nil
    error = nil
    runtime = nil
    casts = 0
    File.foreach(outfn) do |line|
        if line =~ /^\s*status:\s*pass/i and result.nil? then
            result = $RESULT_PASS
        elsif line =~ /^\s*status:\s*fail/i and (result.nil? or
                                                result == $RESULT_PASS) then
            result = $RESULT_FAIL
        elsif line =~ /^\s*status:\s*error/i and
            result = $RESULT_ERROR
        elsif line =~ /^\s*error:\s*(.+)/i then
            error = error.nil? ? $1.to_f : max(error, $1.to_f)
        elsif line =~ /^\s*time:\s*(.+)/i then
            runtime = runtime.nil? ? $1.to_f : min(runtime, $1.to_f)
        elsif line =~ /^\s*(float|double)\s*=>\s*(float|double)\s*:\s*(\d+)/ then
            casts += $3.to_i
        end
    end
    runtime = 0.00001 if runtime == 0.0
    cfg.attrs["result"]    = result
    cfg.attrs["error"]     = error.nil?   ? 0.0 : error
    cfg.attrs["runtime"]   = runtime.nil? ? 1.0 : runtime
    cfg.attrs["casts"]     = casts
    cfg.attrs["new_casts"] = casts - $baseline_casts if not $baseline_casts.nil?

    # clean up files
    fn = "#{$run_path}#{cfg.filename(false)}/#{cfg.filename}"
    if $variable_mode then
        $program.build_json_file(cfg, fn)
    else
        $program.build_config_file(cfg, fn)
    end
    if cfg.attrs["result"] == $RESULT_PASS then
        FileUtils.cp(fn, $passed_path)
    elsif cfg.attrs["result"] == $RESULT_FAIL then
        FileUtils.cp(fn, $failed_path)
    elsif cfg.attrs["result"] == $RESULT_ERROR then
        FileUtils.cp(fn, $aborted_path)
    end
    FileUtils.rm_rf("#{$run_path}#{cfg.filename(false)}") unless $keep_all_runs or
        (cfg.cuid == $PERFCFG_CUID or cfg.cuid == $FINALCFG_CUID)
end

def min(a, b)
    return a < b ? a : b
end

def max(a, b)
    return a > b ? a : b
end


# }}}
# {{{ output


def print_usage
    puts " "
    puts "CRAFT #{$CRAFT_VERSION}"
    puts "Automatic search script"
    puts " "
    puts "Usage:  #{$self_invoke} start  [start-options] <binary>    (run search; binary unnecessary if \"-V\" is given)"
    puts "        #{$self_invoke} search [start-options] <binary>"
    puts "          or"
    puts "        #{$self_invoke} resume [resume-options]            (resume search)"
    puts "          or"
    puts "        #{$self_invoke} status                             (display search status)"
    puts "          or"
    puts "        #{$self_invoke} clean                              (reset search)"
    puts " "
    puts "Standard options:"
    puts "   -h             print help text and exit"
    puts "   -v             print version number and exit"
    puts " "
    puts "Start options:"
    puts "   -A <file>      use <file> as additional advisory configuration (e.g., ADAPT results)"
    puts "   -c <file>      use <file> as initial base configuration"
    puts "   -j <np>        spawn up to <np> simultaneous jobs/configurations (-1 to remove limit)"
    puts "   -J <name>      use <name> job submission system (default is \"exec\")"
    puts "                    valid systems:  \"exec\", \"slurm\""
    puts "   -k             keep all temporary run files"
    puts "   -s <name>      use <name> strategy (default is \"bin_simple\")"
    puts "                    valid strategies:  \"simple\", \"bin_simple\", \"comp_simple\", \"exhaustive\","
    puts "                                       \"combinational\", \"compositional\", \"rprec\""
    puts "   -t <n>         run <n> trials and use max error / min runtime for evaluation"
    puts "   -T <n>         timeout trials after <n> seconds (default is 1.5x baseline runtime)"
    puts "   -V             enable variable mode for variable-level tuning and performance testing"
    puts " "
    puts "Binary-only options (no effect with \"-V\"):"
    puts "   -a             test all generated configs (don't skip non-executed instructions)"
    puts "   -b             stop splitting configs at the basic block level"
    puts "   -C \"<opts>\"    pass the given options to fpconf to generate initial base configuration"
    puts "   -d <p>         debug mode (<p> -> ignore instead of double->single)"
    puts "   -D <p> <a>     debug mode (<p> -> <a> instead of double->single)"
    puts "                    (-d and -D also adjust the fpconf options appropriately)"
    puts "   -f             stop splitting configs at the function level"
    puts "   -N             enable Fortran mode (passes \"-N\" to fpinst)"
    puts "   -S             disable queue sorting (improves overall performance but may converge slower)"
    puts " "
    puts "Variable-only options (no effect without \"-V\"):"
    puts "   -g <tag>       group variables by labels beginning with the given tag"
    puts "                    valid tags: \"set-analysis\""
    puts "   -M             merge overlapping groups (no effect without \"-g\")"
    puts " "
    puts "Shortcuts:"
    puts "   -m             memory-based mixed-precision analysis"
    puts "                    equivalent to '-C \"-c --svinp mem_double\"'"
    puts "   -r             reduced-precision analysis"
    puts "                    equivalent to '-d #{$STATUS_RPREC} -s \"rprec\" -C \"-c --rprec 52\"'"
    puts " "
    puts "Mixed-precision-specific options:"
    puts "   --mixed-use_rprec <X>                  use reduced-precision to simulate <X> bits for single precision"
    puts " "
    puts "Reduced-precision-specific options:"
    puts "   --rprec-split_threshold <X>            don't descend into nodes with <=X bits of precision"
    puts "   --rprec-runtime_pct_threshold <X>      don't descend into nodes with <X% runtime execution"
    puts "   --rprec-skip_app_level                 don't test at the whole-application level"
    puts " "
    puts "Resume options:"
    puts "   -l             resume search at lower level (e.g., INSN instead of BBLK)"
    puts " "
    puts "Using Ruby #{RUBY_VERSION} #{RUBY_RELEASE_DATE}"
    puts " "
end

def get_status
    overall_status = "not running"
    if File.exist?($settings_fn) then
        begin
            load_settings
            overall_status = "initializing"
            overall_status = "waiting" if File.exist?($workqueue_fn)
            overall_status = "running" if (get_workqueue_length > 0 or get_inproc_length > 0)
            overall_status = "finalizing" if File.exist?($final_path)
            overall_status = "DONE" if File.exist?($walltime_fn)
        rescue
            overall_status = "initializing"
        end
    end
    return overall_status
end

MAX_CONFIGS_TO_PRINT=20

def print_status

    hbar   = "===================================================================================================="
    indent = "                            "
    ptag = Time.now.strftime("%Y-%m-%d %H:%M:%S %z")
    ftag = Time.now.strftime("status_%Y_%m_%d_%H_%M_%S_%z.txt")
    overall_status = get_status     # calls load_settings
    full_output = (overall_status == "waiting" or overall_status == "running" or
                   overall_status == "finalizing" or overall_status == "DONE")

    status_text = Array.new
    status_text << "Snapshot taken at #{ptag}"
    status_text << " "
    status_text << hbar
    status_text << "#{indent}                  CRAFT"

    # basic statistics (app, config count, queue size, etc.)
    if full_output then
        elapsed = Time.now.to_f - $start_time.to_f
        if File.exist?($walltime_fn) then
            elapsed = IO.readlines($walltime_fn)[0].to_f
        end
        status_text << "#{indent}Application: #{"%27s" % $binary_name}" unless $variable_mode
        #status_text << "#{indent}Fortran mode:  #{"%24s" % ($fortran_mode ? "Y" : "N")}"
        #status_text << "#{indent}Variable mode:  #{"%24s" % ($variable_mode ? "Y" : "N")}"
        status_text << "#{indent}Search strategy: #{"%23s" % $strategy_name}"
        status_text << "#{indent}Base type:  #{"%28s" % $base_type}"
        status_text << "#{indent}Max in-proc configs:       #{"%13d" % $max_inproc}"
        status_text << "#{indent}Trials per config:         #{"%13d" % $num_trials}"
        status_text << "#{indent}Total candidates:          #{"%13d" % $total_candidates}"
        summary = get_tested_configs_summary
        status_text << "#{indent}Total configs tested:      #{"%13d" % summary["total"]}"
        status_text << "#{indent}   Total passed:           #{"%13d" % summary["pass"]}"
        status_text << "#{indent}     Total skipped:        #{"%13d" % summary["skipped"]}" unless $variable_mode
        status_text << "#{indent}   Total failed:           #{"%13d" % summary["fail"]}"
        status_text << "#{indent}     Total timed out:      #{"%13d" % summary["timeout"]}"
        status_text << "#{indent}   Total aborted:          #{"%13d" % summary["error"]}"
        status_text << "#{indent}   Module-level:           #{"%13d" % summary[$TYPE_MODULE]}"
        status_text << "#{indent}   Function-level:         #{"%13d" % summary[$TYPE_FUNCTION]}"
        status_text << "#{indent}   Block-level:            #{"%13d" % summary[$TYPE_BASICBLOCK]}" unless $variable_mode
        status_text << "#{indent}   Instruction-level:      #{"%13d" % summary[$TYPE_INSTRUCTION]}" unless $variable_mode
        status_text << "#{indent}   Variable-level:         #{"%13d" % summary[$TYPE_VARIABLE]}" if $variable_mode
        status_text << "#{indent}Current in-proc length:    #{"%13d" % get_inproc_length}"
        status_text << "#{indent}Current workqueue length:  #{"%13d" % get_workqueue_length}"
        status_text << "#{indent}Total elapsed walltime: #{"%16s" % format_time(elapsed)}"
    end
    status_text << "#{indent}Overall status:         #{"%16s" % overall_status}"
    status_text << hbar

    # best results so far and current in-process configs
    if full_output then
        report = build_best_report(8)
        report.each do |rl|
            status_text << rl
        end
        status_text << hbar if report.size > 0
        if get_inproc_length > 0 or get_workqueue_length > 0 then
            inproc = get_inproc_configs
            queue = get_workqueue_configs
            lines_to_print = max(min(inproc.size, MAX_CONFIGS_TO_PRINT),
                                 min(queue.size,  MAX_CONFIGS_TO_PRINT))
            status_text << ""
            iprogress = "Currently testing (" + min(inproc.size, lines_to_print).to_s + "/" + inproc.size.to_s + "):"
            qprogress = "Current workqueue (" + min(queue.size,  lines_to_print).to_s + "/" + queue.size.to_s  + "):"
            if $variable_mode then
                status_text << "  #{"%-33s"%iprogress}                 #{"%-33s"%qprogress}"
            else
                status_text << "  #{"%-33s"%qprogress}   STA\%   DYN\% #{"%-33s"%qprogress}   STA\%   DYN\%"
            end
            lines_to_print.times do |i|
                line = ""
                if i < inproc.size then
                    icfg = inproc[i]
                    if $variable_mode then
                        line += "    - %-30s              "%[format_text(icfg.label,30)]
                    else
                        line += "    - %-30s %5.1f  %5.1f "%[format_text(icfg.label,30),icfg.attrs["pct_icount"],icfg.attrs["pct_cinst"]]
                    end
                else
                    line += "%50s"%" "
                end
                if i < queue.size then
                    qcfg = queue[i]
                    if $variable_mode then
                        line += "    - %-30s              "%[format_text(qcfg.label,30)]
                    else
                        line += "    - %-30s %5.1f  %5.1f "%[format_text(qcfg.label,30),qcfg.attrs["pct_icount"],qcfg.attrs["pct_cinst"]]
                    end
                end
                status_text << line if line.strip != ""
            end
            status_text << "%-50s"%(lines_to_print < inproc.size ? "      ..." : "") +
                                   (lines_to_print < queue.size  ? "      ..." : "")
            status_text << hbar
        end
    end

    # print to stdout
    puts status_text

    # create snapshot file
    if !File.exist?($snapshot_path) then
        Dir.mkdir($snapshot_path)
    end
    f = File.new("#{$snapshot_path}#{ftag}", "w")
    f.puts status_text
    f.close

    # remove latest snapshot if it's a duplicate of the previous one
    all_snapshots = Dir.glob("#{$snapshot_path}status*.txt").sort { |f1,f2| File.mtime(f2) <=> File.mtime(f1) }
    if all_snapshots.size > 1 then
        newest_fn = all_snapshots.at(0)
        older_fn = all_snapshots.at(1)
        diff = `diff #{older_fn} #{newest_fn}`.split("\n")

        # 2 always-changing snapshot lines (timestamp + elapse walltime)
        # 4 diff lines per snapshot line (assumes default diff output)
        if diff.size <= 2*4 then
            File.delete(newest_fn)
        end
    end
end

def format_time (elapsed)
    mins, secs = elapsed.divmod 60.0
    hrs, mins = mins.divmod 60.0
    #return "%d:%02d:%05.2f"%[hrs.to_i, mins.to_i, secs]
    return "%d:%02d:%02d"%[hrs.to_i, mins.to_i, secs.to_i]
end

def format_text (str, width)
    if str.size > width then
        str = str[0,width-3] + "..."
    end
    return str
end


# }}}
# {{{ finalization and cleanup


def rebuild_final_config
    if $variable_mode then
        $final_config = $strategy.build_fastest_config(get_tested_configs)
        $program.build_json_file($final_config, $final_config_fn)
        # also build regular config for UI
        $program.build_config_file($final_config, $final_config_fn[0...-4]+"cfg", false)
    else
        $final_config = $strategy.build_final_config(get_tested_configs)
        $program.build_config_file($final_config, $final_config_fn, false)
        calculate_pct_stats($final_config)
    end
end

def is_single_base (cfg, type)
    isb = false
    if cfg.exceptions.keys.size == 1 then
        pt = $program.lookup_by_uid(cfg.exceptions.keys.first)
        if !pt.nil? and pt.type == type then
            isb = true
        end
    end
    return isb
end

def build_best_report(num,copy_files=false)
    if $strategy_name == "rprec" then
        return Array.new
    elsif $variable_mode then
        return build_best_runtime_report(num,copy_files)
    else
        return build_best_regular_report(num,copy_files)
    end
end

def build_reduced_prec_report(num,copy_files=false)
    results = get_tested_configs
    $strategy.reload_bounds_from_results(results)
    report = Array.new

    # TODO: iterate over $program and find the best bounds for each instruction
    #       then merge this with the icount and cinst info and report it all

    return report
end

def build_best_regular_report(num,copy_files=false)
    # find best individual configs
    results = get_tested_configs
    inst_passed_cfgs = Array.new
    exec_passed_cfgs = Array.new
    exec_passed_cfgs = Array.new
    inst_passed = 0.0
    exec_passed = 0.0
    inst_failed_cfgs = Array.new
    exec_failed_cfgs = Array.new
    inst_failed = 0.0
    exec_failed = 0.0
    results.each do |cfg|
        if not (cfg.cuid == $FINALCFG_CUID) then
            if cfg.attrs["result"] == $RESULT_PASS then
                inst_passed_cfgs << [cfg, cfg.attrs["pct_icount"].to_f]
                exec_passed_cfgs << [cfg, cfg.attrs["pct_cinst"].to_f]
                inst_passed += cfg.attrs["pct_icount"].to_f
                exec_passed += cfg.attrs["pct_cinst"].to_f
            else
                if cfg.attrs.has_key?("single_type") and cfg.attrs["single_type"] == $base_type then
                    inst_failed_cfgs << [cfg, cfg.attrs["pct_icount"].to_f]
                    exec_failed_cfgs << [cfg, cfg.attrs["pct_cinst"].to_f]
                    inst_failed += cfg.attrs["pct_icount"].to_f
                    exec_failed += cfg.attrs["pct_cinst"].to_f
                end
            end
        end
    end
    report = Array.new
    report << " "
    report << "  Top instrumented (passed):                STA\%    Top instrumented (failed/aborted):        STA\%"
    inst_passed_cfgs = inst_passed_cfgs.sort_by { |cfg,pct| -pct }
    inst_failed_cfgs = inst_failed_cfgs.sort_by { |cfg,pct| -pct }
    0.upto(num-1).each do |i|
        line = ""
        if i < inst_passed_cfgs.size then
            pcfg,ppct = inst_passed_cfgs[i]
            if copy_files and File.exists?($passed_path + pcfg.filename) then
                FileUtils.cp($passed_path + pcfg.filename, $best_path)
            end
            line += "    - %-35s  %5.1f  "%[format_text(pcfg.label,35),ppct]
        else
            line += "%50s"%" "
        end
        if i < inst_failed_cfgs.size then
            fcfg,fpct = inst_failed_cfgs[i]
            line += "    - %-35s  %5.1f  "%[format_text(fcfg.label,35),fpct]
        end
        report << line if line.strip != ""
    end
    report << " "
    report << "      %35s  %5.1f  "%["TOTAL PASSED",inst_passed] +
              "      %35s  %5.1f  "%["TOTAL FAILED",inst_failed]
    report << "%50s      %35s  %5.1f"%[" ","TOTAL",inst_passed+inst_failed]
    report << " "
    report << "  Top executed (passed):                    DYN\%    Top executed (failed/aborted):            DYN\%"
    exec_passed_cfgs = exec_passed_cfgs.sort_by { |cfg,pct| -pct }
    exec_failed_cfgs = exec_failed_cfgs.sort_by { |cfg,pct| -pct }
    0.upto(num-1).each do |i|
        line = ""
        if i < exec_passed_cfgs.size then
            pcfg,ppct = exec_passed_cfgs[i]
            if copy_files and File.exists?($passed_path + pcfg.filename) then
                FileUtils.cp($passed_path + pcfg.filename, $best_path)
            end
            line += "    - %-35s  %5.1f  "%[format_text(pcfg.label,35),ppct]
        else
            line += "%50s"%" " 
        end
        if i < exec_failed_cfgs.size then
            fcfg,fpct = exec_failed_cfgs[i]
            line += "    - %-35s  %5.1f  "%[format_text(fcfg.label,35),fpct]
        end
        report << line if line.strip != ""
    end
    report << " "
    report << "      %35s  %5.1f  "%["TOTAL PASSED",exec_passed] +
              "      %35s  %5.1f  "%["TOTAL FAILED",exec_failed]
    report << "%50s      %35s  %5.1f"%[" ","TOTAL",exec_passed+exec_failed]
    report << " "
    return report
end

def build_best_runtime_report(num,copy_files=false)
    # find best individual configs
    results = get_tested_configs
    passed_cfgs = Array.new
    results.each do |cfg|
        if not (cfg.cuid == $FINALCFG_CUID) then
            if cfg.attrs["result"] == $RESULT_PASS then
                passed_cfgs << [cfg, cfg.attrs["runtime"]]
            end
        end
    end
    report = Array.new
    report << " "
    report << "          Top instrumented (passed):                Runtime (s)          Speedup (X)"
    passed_cfgs = passed_cfgs.sort_by { |cfg,rtime| [rtime, -cfg.cuid.size] }
    max_speedup = 0.0
    0.upto(num-1).each do |i|
        line = ""
        if i < passed_cfgs.size then
            pcfg,prtime = passed_cfgs[i]
            speedup = $baseline_runtime / prtime
            max_speedup = [max_speedup, speedup].max
            if copy_files and File.exists?($passed_path + pcfg.filename) then
                FileUtils.cp($passed_path + pcfg.filename, $best_path)
            end
            line += "            - %-35s   %-5.2f                %-5.2f"%
                [format_text(pcfg.label,35),prtime,speedup]
        end
        report << line if line.strip != ""
    end
    report << " "
    if max_speedup > 1.0 then
        report << "          Speedup achieved! (max: %.2fx, baseline: %.2fs)"%[max_speedup,$baseline_runtime]
    else
        report << "          No speedup found."
    end
    report << " "
    return report
end

def get_tested_configs_summary
    summary = Hash.new(0)
    tested_cfgs = get_tested_configs
    tested_cfgs.each do |cfg|
        if cfg.attrs.has_key?("result") then
            if cfg.attrs["result"] == $RESULT_PASS then
                summary["pass"] += 1
            elsif cfg.attrs["result"] == $RESULT_FAIL then
                summary["fail"] += 1
            elsif cfg.attrs["result"] == $RESULT_ERROR then
                summary["error"] += 1
            end
        end
        if cfg.attrs.has_key?("skipped") and cfg.attrs["skipped"] == "yes" then
            summary["skipped"] += 1
        end
        if cfg.attrs.has_key?("level") then
            summary[cfg.attrs["level"]] += 1
        end
        if cfg.attrs.has_key?("timeout") then
            summary["timeout"] += 1
        end
    end
    summary["total"] = tested_cfgs.size
    summary["tested"] = summary["total"] - summary["skipped"]
    return summary
end

def clean_everything
    toDelete = Array.new
    Dir.glob("#{$search_tag}.*") do |fn| toDelete << fn end
    Dir.glob($perf_path) do |fn| toDelete << fn end
    Dir.glob($prof_path) do |fn| toDelete << fn end
    Dir.glob($run_path) do |fn| toDelete << fn end
    Dir.glob($final_path) do |fn| toDelete << fn end
    Dir.glob($best_path) do |fn| toDelete << fn end
    Dir.glob($passed_path) do |fn| toDelete << fn end
    Dir.glob($failed_path) do |fn| toDelete << fn end
    Dir.glob($aborted_path) do |fn| toDelete << fn end
    Dir.glob($snapshot_path) do |fn| toDelete << fn end
    toDelete.each do |fn|
        FileUtils.rm_rf(fn)
    end
end


# }}}
