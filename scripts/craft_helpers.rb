# {{{ initialization


def read_craft_driver
    if !File.exists?("#{$search_path}#{$craft_driver}") then
        puts "No craft_driver found here."
        exit
    end
    IO.foreach("#{$search_path}#{$craft_driver}") do |line|
        if line =~ /^#\s*BINARY\s*=\s*(.*)/ then
            $binary_name = File.basename($1)
            $binary_path = File.expand_path($1)
        elsif line =~ /^#\s*NUM_WORKERS\s*=\s*(\d+)/ then
            $num_workers = $1.to_i
        elsif line =~ /^#\s*PREFERRED_STATUS\s*=\s*(.)/ then
            $status_preferred = $1
        elsif line =~ /^#\s*ALTERNATE_STATUS\s*=\s*(.)/ then
            $status_alternate = $1
        elsif line =~ /^#\s*INITIAL_CFG\s*=\s*(\w+)/ then
            $initial_cfg_fn = $1
        elsif line =~ /^#\s*CACHED_CFGS\s*=\s*(\w+)/ then
            $cached_fn = $1
        elsif line =~ /^#\s*FORTRAN_MODE\s*=\s*(\w+)/ then
            if $1 == "yes" then
                $fortran_mode = true
            end
        elsif line =~ /^#\s*BASE_TYPE\s*=\s*(\w*)/ then
            type = $1.chomp.downcase
            if type =~ /ins/ then
                $base_type = $TYPE_INSTRUCTION
            elsif type =~ /var/ then
                $base_type = $TYPE_VARIABLE
            elsif type =~ /block/ then
                $base_type = $TYPE_BASICBLOCK
            elsif type =~ /func/ then
                $base_type = $TYPE_FUNCTION
            elsif type =~ /mod/ then
                $base_type = $TYPE_MODULE
            elsif type =~ /app/ then
                $base_type = $TYPE_APPLICATION
            end
        elsif line =~ /^#\s*SEARCH_STRAT\s*=\s*(\w+)/ then
            if not ($1 == "default") then
                $strategy_name = $1
            end
        end
    end
end


def parse_command_line
    $main_mode = ARGV.shift.strip.downcase
    if $main_mode == "search" then
        $main_mode = "start"
    end
    if not ["start","resume","worker","status","help","clean"].include?($main_mode) then
        puts "Invalid mode: #{$main_mode}"
        exit
    end
    if $main_mode == "worker" then
        $worker_id = ARGV.shift
        $search_path = ARGV.shift
    end
    parsed_binary = false
    while ARGV.size > 0 do
        opt = ARGV.shift
        if $main_mode == "start" then
            if opt == "-j" then
                # parallel: "-j 4" variant
                $num_workers = ARGV.shift.to_i
            elsif opt =~ /^-j/ then
                # parallel: "-j4" variant
                $num_workers = opt[2,opt.length-2].to_i
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
            elsif opt == "-A" then
                # archived/cached configuration results
                $cached_fn = File.expand_path(ARGV.shift)
            elsif opt == "-c" then
                # initial configuration
                $initial_cfg_fn = ARGV.shift
            elsif opt == "-C" then
                # initial fpconf options
                $fpconf_options = ARGV.shift
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
                $strategy_name = "combinational"
            elsif opt == '-b' then
                # stop splitting at basic blocks
                $base_type = $TYPE_BASICBLOCK
            elsif opt == '-f' then
                # stop splitting at functions
                $base_type = $TYPE_FUNCTION
            elsif opt == '-F' then
                # don't run final config
                $run_final_config = false
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
            if opt == "-j" then
                # parallel: "-j 4" variant
                $num_workers = ARGV.shift.to_i
            elsif opt =~ /^-j/ then
                # parallel: "-j4" variant
                $num_workers = opt[2,opt.length-2].to_i
            elsif opt == "-l" then
                $resume_lower = true
            elsif opt == "-n" then
                $restart_inproc = true
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
    global_variables = []
    functions = Hash.new()
    if cfg.has_key?("actions") then
        cfg["actions"]. each do |a|
            if a.has_key?("action") and a["action"] == "replace_var_type" and
                    a.has_key?("to_type") and a["to_type"] == "float" and
                    a.has_key?("variable_name") then
                if not a.has_key?("scope") or a["scope"] == "global" then
                    global_variables << a["variable_name"]
                elsif a["scope"] == "function_local" and
                        a.has_key?("functions") and
                        a["functions"].size == 1 then
                    func = a["functions"][0]
                    functions[func] = [] if not functions.has_key?(func)
                    functions[func] << a["variable_name"]
                end
            end
        end
    end
    program = PPoint.new("APP #1", $TYPE_APPLICATION, $STATUS_NONE)
    mod     = PPoint.new("MOD #1", $TYPE_MODULE,      $STATUS_NONE)
    program.children << mod
    vidx = 0
    fidx = 0
    global_variables.each do |v|
        vidx += 1
        var = PPoint.new("VAR ##{vidx}", $TYPE_VARIABLE, $STATUS_CANDIDATE)
        var.attrs["addr"] = "0x0"
        var.attrs["desc"] = v
        mod.children << var
    end
    functions.each_key do |f|
        fidx += 1
        func = PPoint.new("FUNC ##{fidx}", $TYPE_FUNCTION, $STATUS_NONE)
        func.attrs["desc"] = f
        mod.children << func
        bblk = PPoint.new("BBLK ##{fidx}", $TYPE_BASICBLOCK, $STATUS_NONE)
        bblk.attrs["addr"] = "0x0"
        func.children << bblk
        functions[f].each do |v|
            vidx += 1
            var = PPoint.new("VAR ##{vidx}", $TYPE_VARIABLE, $STATUS_CANDIDATE)
            var.attrs["addr"] = "0x0"
            var.attrs["desc"] = v
            func.children << var
        end
    end
    return program
end

def read_craft_config(cfg)
    program =  nil
    mod = nil
    func = nil
    bblk = nil
    prolog = ""
    cfg.each_line do |line|
        if line =~ /(APPLICATION #\d+: [x0-9A-Fa-f]+) \"(.+)\"/ then
            program = PPoint.new($1, $TYPE_APPLICATION, line[1,1])
            program.attrs["desc"] = $2
            mod = nil
            func = nil
            bblk = nil
        elsif line =~ /(MODULE #\d+: 0x[0-9A-Fa-f]+) \"(.+)\"/ then
            mod = PPoint.new($1, $TYPE_MODULE, line[1,1])
            mod.attrs["desc"] = $2
            program.children << mod if program != nil
            func = nil
            bblk = nil
        elsif line =~ /(FUNC #\d+: 0x[0-9A-Fa-f]+) \"(.+)\"/ then
            func = PPoint.new($1, $TYPE_FUNCTION, line[1,1])
            func.attrs["desc"] = $2
            mod.children << func if mod != nil
            bblk = nil
        elsif line =~ /(BBLK #\d+: (0x[0-9A-Fa-f]+))/ then
            bblk = PPoint.new($1, $TYPE_BASICBLOCK, line[1,1])
            bblk.attrs["addr"] = $2
            func.children << bblk if func != nil
        elsif line =~ /(INSN #\d+: (0x[0-9A-Fa-f]+)) \"(.+)\"/ then
            insn = PPoint.new($1, $TYPE_INSTRUCTION, line[1,1])
            insn.attrs["addr"] = $2
            insn.attrs["desc"] = $3
            bblk.children << insn if bblk != nil
        elsif line =~ /(VAR #\d+: (0x[0-9A-Fa-f]+)) \"(.+)\"/ then
            var = PPoint.new($1, $TYPE_VARIABLE, line[1,1])
            var.attrs["addr"] = $2
            var.attrs["desc"] = $3
            bblk.children << var if bblk != nil
            mod.children  << var if bblk == nil
        else
            prolog << line.chomp
            prolog << "\n"
        end
    end
    program.attrs["prolog"] = prolog
    return program
end

def add_variables_to_config_file(fn, cfg)
    vcount = 0
    IO.foreach(fn) do |line|
        if line =~ /craft_t\s*(\*)?\s*(\w+)/ then
            vname = $2
            cfg.print "    "
            cfg.print "," if vcount > 0
            cfg.puts "{ \"action\": \"replace_var_type\", \"scope\": \"global\","
            cfg.puts "      \"variable_name\": \"#{vname}\","
            cfg.puts "      \"from_type\": \"craft_t\", \"to_type\":\"float\" }"
            vcount += 1
        end
    end
end

def initialize_strategy
    if $strategy_name == "simple" then
        $strategy = Strategy.new($program, $status_preferred, $status_alternate)
    elsif $strategy_name == "bin_simple" then
        $strategy = BinaryStrategy.new($program, $status_preferred, $status_alternate)
    elsif $strategy_name == "exhaustive" then
        $strategy = ExhaustiveStrategy.new($program, $status_preferred, $status_alternate)
    elsif $strategy_name == "combinational" then
        $strategy = ExhaustiveCombinationalStrategy.new($program, $status_preferred, $status_alternate)
    elsif $strategy_name == "rprec" then
        $strategy = RPrecStrategy.new($program, $status_preferred, $status_alternate)
        $strategy.split_threshold = $rprec_split_threshold
        $strategy.runtime_pct_threshold = $rprec_runtime_pct_threshold
        $strategy.skip_app_level = $rprec_skip_app_level
    elsif $strategy_name == "ddebug" then
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
    passed = false
    if not File.exist?($perf_path) then
        Dir.mkdir($perf_path)
    end
    Dir.chdir($perf_path)
    $baseline_error = 0.0
    $baseline_runtime = 0.0
    if $variable_mode then
        File.open("tmp.cfg","w") do |f| f.print("") end
        cmd = "#{$search_path}#{$craft_driver} tmp.cfg"
    else
        cmd = "#{$search_path}#{$craft_driver} #{$binary_path}"
    end
    puts cmd
    Open3.popen3(cmd) do |io_in, io_out, io_err|
        io_out.each_line do |line|
            if line =~ /status:\s*(pass|fail)/i then
                if $1 =~ /pass/i then
                    passed = true
                end
            end
            if line =~ /error:\s*(.+)/i then
                $baseline_error = $1.to_f
            end
            if line =~ /time:\s*(.+)/i then
                $baseline_runtime = $1.to_f
            end
        end
    end
    if $baseline_runtime == 0.0 then
        $baseline_runtime = 0.0001      # avoid divide-by-zero errors later
    end
    Dir.chdir($search_path)
    return passed
end


def run_profiler
    # there's no need for profile info in variable mode
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

def run_config (cfg)

    # write actual configuration file
    cfg_path = $search_path + cfg.filename
    if $variable_mode then
        #puts "Writing JSON file for #{cfg.cuid}"
        $program.build_typeforge_file(cfg, cfg_path)
        # TODO: add support for JSON output
    else
        #puts "Writing CRAFT file for #{cfg.cuid}"
        $program.build_config_file(cfg, cfg_path)
    end

    # pass off to filename version
    result,error,runtime = run_config_file(cfg_path, false, cfg.label)

    # save result
    cfg.attrs["result"] = result
    cfg.attrs["error"] = error
    cfg.attrs["runtime"] = runtime

    # delete actual configuration file
    FileUtils.rm_rf(cfg_path)

    return result
end

def run_config_file (fn, keep, label)

    # status updates
    basename = File.basename(fn)
    if basename =~ /^#{$binary_name}(.*)/ then
        basename = $1
    end
    #puts "  Testing #{basename} ..."
    $status_buffer = "    Finished testing #{label}:\n"

    # build rewritten mutatee
    if not $variable_mode then
        cmd = "#{$fpinst_invoke} -i #{$fortran_mode ? "-N" : ""}"
        cmd += " -c #{fn}"
        cmd += " #{$binary_path}"
        add_to_mainlog("    Building mutatee for #{basename}: #{cmd}")
        Open3.popen3(cmd) do |io_in, io_out, io_err|
            io_out.each_line do |line|
                if line =~ /Inplace: (.*)$/ then
                    $status_buffer += "        #{$1}\n"
                #elsif line =~ /replacing "(\w+)"/ then
                    #$status_buffer += " #{$1}"
                end
            end
        end
    end

    # execute rewritten mutatee and check for success
    result = $RESULT_ERROR
    runtime = 0.0
    error = 0.0
    cmd = "#{$search_path}#{$craft_driver}"
    if $variable_mode then
        cmd += " #{fn}"
    else
        cmd += " #{Dir.getwd}/mutant"
    end
    #add_to_mainlog("    Testing mutatee for #{basename}: #{cmd}")
    Open3.popen3(cmd) do |io_in, io_out, io_err|
        io_out.each_line do |line|
            if line =~ /status:\s*(pass|fail)/i then
                tmp = $1
                if tmp =~ /pass/i then
                    result = $RESULT_PASS
                elsif tmp =~ /fail/i then
                    result = $RESULT_FAIL
                end
            elsif line =~ /error:\s*(.+)/i then
                error = $1.to_f
            elsif line =~ /time:\s*(.+)/i then
                runtime = $1.to_f
            end
        end
    end

    # scan log file(s) for info
    Dir.glob("*.log").each do |lfn|
        if lfn != "fpinst.log" then
            IO.foreach(lfn) do |line|
                if line =~ /Inplace: (.*)$/ then
                    $status_buffer += "        #{$1.gsub(/<.*>/, "")}\n"
                end
            end
        end
    end

    # print output
    $status_buffer += "        #{result}"
    $status_buffer += "  [Walltime: #{format_time(runtime.to_f)}"
    $status_buffer += " %.1fX"%[runtime / $baseline_runtime.to_f]
    $status_buffer += " Error: %g]"%[error]
    puts $status_buffer
    add_to_mainlog($status_buffer)
    if keep then
        f = File.new("result.txt", "w")
        f.puts $status_buffer
        f.close
    end
    $status_buffer = ""

    # copy config to appropriate folder
    if result == $RESULT_PASS then
        FileUtils.cp(fn, $passed_path)
    elsif result == $RESULT_FAIL then
        FileUtils.cp(fn, $failed_path)
    elsif result == $RESULT_ERROR then
        FileUtils.cp(fn, $aborted_path)
    end

    # clean out mutant, logs, and rewritten library files
    if !keep then
        begin
            toDelete = Array.new
            Dir.glob("*").each do |lfn|
                toDelete << lfn
                #if lfn =~ /mutant/ || lfn =~ /\.log/ || lfn =~ /lib(c|m)\.so\.6/ then
                #if lfn =~ /mutant/ || lfn =~ /\.log/ || lfn =~ /\.so$/ then
                    #File.delete(lfn)
                #end
            end
            toDelete.each do |fn|
                File.delete(fn)
            end
        rescue
            puts "Error clearing old files"
        end
    end

    return [result,error,runtime]
end


# }}}
# {{{ output


def print_usage
    puts " "
    puts "CRAFT #{$CRAFT_VERSION}"
    puts "Automatic search script"
    puts " "
    puts "Usage:  #{$self_invoke} start [start-options] <binary>     (supervisor process)"
    puts "        #{$self_invoke} search [start-options] <binary>    (supervisor process)"
    puts "          or"
    puts "        #{$self_invoke} resume [resume-options]            (resume supervisor process)"
    puts "          or"
    puts "        #{$self_invoke} status                             (display search status)"
    puts "          or"
    puts "        #{$self_invoke} clean                              (reset search)"
    puts "          or"
    puts "        #{$self_invoke} worker <id> <dir>                  (worker process; used internally)"
    puts " "
    puts "Shortcut mode modifications:"
    puts "   <default>      operation-based mixed-precision analysis"
    puts "   -m             memory-based mixed-precision analysis"
    puts "                    equivalent to '-C \"-c --svinp mem_double\"'"
    puts "   -r             reduced-precision analysis"
    puts "                    equivalent to '-d #{$STATUS_RPREC} -s \"rprec\" -C \"-c --rprec 52\"'"
    puts " "
    puts "Initiation options:"
    puts "   -a             test all generated configs (don't skip non-executed instructions)"
    puts "   -A <file>      use an archived craft.tested file to avoid re-running tests from a previous search"
    puts "   -b             stop splitting configs at the basic block level"
    puts "   -c <file>      use <file> as initial base configuration"
    puts "   -C \"<opts>\"    pass the given options to fpconf to generate initial base configuration"
    puts "   -d <p>         debug mode (<p> -> ignore instead of double->single)"
    puts "   -D <p> <a>     debug mode (<p> -> <a> instead of double->single)"
    puts "                    (-d and -D also adjust the fpconf options appropriately)"
    puts "   -f             stop splitting configs at the function level"
    puts "   -F             don't test final configuration"
    puts "   -j <np>        spawn <np> worker threads"
    puts "   -N             enable Fortran mode (passes \"-N\" to fpinst)"
    puts "   -s <name>      use <name> strategy (default is \"bin_simple\")"
    puts "                    valid strategies:  \"simple\", \"bin_simple\", \"exhaustive\", \"combinational\", \"rprec\""
    puts "   -S             disable queue sorting (improves overall performance but may converge slower"
    puts "   -V             enable variable mode for variable-level tuning and performance testing"
    puts "                    (-V also sets the default strategy to \"combinational\")"
    puts " "
    puts "Mixed-precision-specific options:"
    puts "   --mixed-use_rprec <X>                  use reduced-precision to simulate <X> bits for single precision"
    puts " "
    puts "Reduced-precision-specific options:"
    puts "   --rprec-split_threshold <X>            don't descend into nodes with <=X bits of precision"
    puts "   --rprec-runtime_pct_threshold <X>      don't descend into nodes with <X% runtime execution"
    puts "   --rprec-skip_app_level                 don't test at the whole-application level"
    puts " "
    puts "Resumption options:"
    puts "   -j <np>        spawn <np> worker threads"
    puts "   -l             resume search at lower level (e.g., INSN instead of BBLK)"
    puts "   -n             resume search and restart in-process tests"
    puts " "
    puts "Using Ruby #{RUBY_VERSION} #{RUBY_RELEASE_DATE}"
    puts " "
end


def get_worker_thread_count
    cmd = "ps -C #{$self_invoke} -o s="
    psr = `#{cmd}`
    num_workers = [0,psr.lines.count - ($main_mode == "status" ? 2 : 1)].max
    return num_workers
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


def print_status

    hbar   = "===================================================================================================="
    indent = "                            "
    ptag = Time.now.strftime("%Y-%m-%d %H:%M:%S %z")
    ftag = Time.now.strftime("status_%Y_%m_%d_%H_%M_%S_%z.txt")
    overall_status = get_status     # calls load_settings
    full_output = (overall_status == "waiting" or overall_status == "running" or
                   overall_status == "finalizing" or overall_status == "DONE")
    nworkers = get_worker_thread_count

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
        status_text << "#{indent}Application: #{"%27s" % $binary_name}"
        #status_text << "#{indent}Fortran mode:  #{"%24s" % ($fortran_mode ? "Y" : "N")}"
        status_text << "#{indent}Typeforge mode: #{"%24s" % ($variable_mode ? "Y" : "N")}"
        status_text << "#{indent}Base type:  #{"%28s" % $base_type}"
        status_text << "#{indent}Worker threads:            #{"%13d" % nworkers}"
        status_text << "#{indent}Total candidates:          #{"%13d" % $total_candidates}"
        summary = get_tested_configs_summary
        status_text << "#{indent}Total configs tested:      #{"%13d" % summary["total"]}"
        status_text << "#{indent}   Total executed:         #{"%13d" % summary["tested"]}"
        status_text << "#{indent}   Total cached:           #{"%13d" % summary["cached"]}"
        status_text << "#{indent}   Total passed:           #{"%13d" % summary["pass"]}"
        status_text << "#{indent}     Total skipped:        #{"%13d" % summary["skipped"]}"
        status_text << "#{indent}   Total failed:           #{"%13d" % summary["fail"]}"
        status_text << "#{indent}   Total aborted:          #{"%13d" % summary["error"]}"
        status_text << "#{indent}   Module-level:           #{"%13d" % summary[$TYPE_MODULE]}"
        status_text << "#{indent}   Function-level:         #{"%13d" % summary[$TYPE_FUNCTION]}"
        status_text << "#{indent}   Block-level:            #{"%13d" % summary[$TYPE_BASICBLOCK]}"
        status_text << "#{indent}   Instruction-level:      #{"%13d" % summary[$TYPE_INSTRUCTION]}"
        status_text << "#{indent}   Variable-level:         #{"%13d" % summary[$TYPE_VARIABLE]}"
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
            status_text << ""
            progress = "Current workqueue (" + (queue.size > inproc.size ? inproc.size : queue.size).to_s + "/" + queue.size.to_s + "):"
            if $variable_mode then
                status_text << "  Currently testing:                                #{"%-33s"%progress}"
            else
                status_text << "  Currently testing:                  STA\%   DYN\%   #{"%-33s"%progress}   STA\%   DYN\%"
            end
            0.upto(inproc.size-1) do |i|
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
                        line += "    - %-30s              "%[format_text(icfg.label,30)]
                    else
                        line += "    - %-30s %5.1f  %5.1f "%[format_text(qcfg.label,30),qcfg.attrs["pct_icount"],qcfg.attrs["pct_cinst"]]
                    end
                end
                status_text << line if line.strip != ""
            end
            if queue.size > inproc.size then
                status_text << "%50s      ..."%" "
            end
            status_text << ""
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
        $program.build_typeforge_file($final_config, $final_config_fn)
        # TODO: add support for JSON output
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
            line += "            - %-35s   %-5.2f                %-5.1f"%
                [format_text(pcfg.label,35),prtime,speedup]
        end
        report << line if line.strip != ""
    end
    report << " "
    if max_speedup > 1.0 then
        report << "          Speedup achieved! (max: %.1fx, baseline: %.2fs)"%[max_speedup,$baseline_runtime]
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
        if cfg.attrs.has_key?("cached") and cfg.attrs["cached"] == "yes" then
            summary["cached"] += 1
        end
        if cfg.attrs.has_key?("level") then
            summary[cfg.attrs["level"]] += 1
        end
    end
    summary["total"] = tested_cfgs.size
    summary["tested"] = summary["total"] - summary["skipped"] - summary["cached"]
    return summary
end


def clean_everything
    toDelete = Array.new
    Dir.glob("#{$search_tag}.*") do |fn| toDelete << fn end
    Dir.glob("#{$search_tag}_*.cfg") do |fn| toDelete << fn end
    Dir.glob("*_worker*") do |fn| toDelete << fn end
    Dir.glob("*.log") do |fn| toDelete << fn end
    Dir.glob("baseline") do |fn| toDelete << fn end
    #Dir.glob("profile") do |fn| toDelete << fn end
    Dir.glob("final") do |fn| toDelete << fn end
    Dir.glob("best") do |fn| toDelete << fn end
    Dir.glob("passed") do |fn| toDelete << fn end
    Dir.glob("failed") do |fn| toDelete << fn end
    Dir.glob("aborted") do |fn| toDelete << fn end
    Dir.glob("snapshots") do |fn| toDelete << fn end
    toDelete.each do |fn|
        FileUtils.rm_rf(fn)
    end
end


# }}}