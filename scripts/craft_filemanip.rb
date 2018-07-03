# {{{ file manipulation


# FOR THE SYSTEM TO WORK, ALL OF THESE I/O ROUTINES *MUST* BE ATOMIC!
# None should be able to interrupt or starve another. This means that
# we need to lock the file in each function, and no function should
# call another.


def save_settings
    f = File.new($settings_fn, "r+")
    f.flock File::LOCK_EX
    f.truncate(0)

    f.puts "start_time=#{$start_time.to_i}"
    f.puts "binary_name=#{$binary_name}"
    f.puts "binary_path=#{$binary_path}"
    f.puts "strategy_name=#{$strategy_name}"
    f.puts "status_preferred=#{$status_preferred}"
    f.puts "status_alternate=#{$status_alternate}"
    f.puts "baseline_runtime=#{$baseline_runtime}"
    f.puts "lang=#{($fortran_mode ? "fortran" : "other")}"
    f.puts "variable_mode=#{($variable_mode ? "yes" : "no")}"
    f.puts "base_type=#{$base_type}"
    f.puts "skip_nonexecuted=#{$skip_nonexecuted}"
    f.puts "initial_cfg_fn=#{$initial_cfg_fn}"
    f.puts "cached_fn=#{$cached_fn}"
    f.puts "prof_log_fn=#{$prof_log_fn}"
    f.puts "total_candidates=#{$total_candidates}"
    f.puts "rprec_split_threshold=#{$rprec_split_threshold}"
    f.puts "rprec_runtime_pct_threshold=#{$rprec_runtime_pct_threshold}"
    f.puts "rprec_skip_app_level=#{$rprec_skip_app_level}"
    f.puts "disable_queue_sort=#{$disable_queue_sort.to_s}"
    f.puts "run_final_config=#{$run_final_config.to_s}"
    f.puts "mixed_use_rprec=#{$mixed_use_rprec.to_s}"

    #f.puts $start_time.to_i
    #f.puts $binary_name
    #f.puts $binary_path
    #f.puts $strategy_name
    #f.puts $status_preferred
    #f.puts $status_alternate
    #f.puts $baseline_runtime
    #f.puts($fortran_mode ? "fortran" : "other")
    #f.puts $base_type
    #f.puts $skip_nonexecuted
    #f.puts $initial_cfg_fn
    #f.puts $prof_log_fn
    #f.puts $total_candidates
    #f.puts $rprec_split_threshold
    #f.puts $rprec_runtime_pct_threshold
    #f.puts $rprec_skip_app_level
    #f.puts $disable_queue_sort
    #f.puts $run_final_config
    #f.puts $mixed_use_rprec

    f.close
end
def load_settings
    f = File.new($settings_fn, "r")
    f.flock File::LOCK_SH
    parser = /^\s*(\w+)\s*=\s*(.*)\s*$/
    temp = f.gets
    if temp =~ parser then
        while temp != nil and temp =~ parser do
            key = $1
            value = $2
            case key
            when "start_time"
                $start_time = Time.at(value.to_i)
            when "binary_name"
                $binary_name = value
            when "binary_path"
                $binary_path = value
            when "strategy_name"
                $strategy_name = value
            when "status_preferred"
                $status_preferred = value
            when "status_alternate"
                $status_alternate = value
            when "baseline_runtime"
                $baseline_runtime = value.to_f
            when "lang"
                if value == "fortran" then $fortran_mode = true
                else                       $fortran_mode = false
                end
            when "variable_mode"
                if value == "yes" then $variable_mode = true
                else                   $variable_mode = false
                end
            when "base_type"
                $base_type = value
            when "skip_nonexecuted"
                $skip_nonexecuted = (value == true.to_s)
            when "cached_fn"
                $cached_fn = value
            when "initial_cfg_fn"
                $initial_cfg_fn = value
            when "prof_log_fn"
                $prof_log_fn = value
            when "total_candidates"
                $total_candidates = value.to_i
            when "rprec_split_threshold"
                $rprec_split_threshold = value.to_i
            when "rprec_runtime_pct_threshold"
                $rprec_runtime_pct_threshold = value.to_f
            when "rprec_skip_app_level"
                $rprec_skip_app_level = (value == true.to_s)
            when "disable_queue_sort"
                $disable_queue_sort = (value == true.to_s)
            when "run_final_config"
                $run_final_config = (value == true.to_s)
            when "mixed_use_rprec"
                $mixed_use_rprec = (value == true.to_s)
            end
            temp = f.gets
        end
    else
        # old style settings file; included for backwards compatibility
        $start_time = Time.at(temp.to_i)
        $binary_name = f.gets.chomp
        $binary_path = f.gets.chomp
        $strategy_name = f.gets.chomp
        $status_preferred = f.gets.chomp
        $status_alternate = f.gets.chomp
        $baseline_runtime = f.gets.chomp.to_i
        lang = f.gets.chomp
        if lang == "fortran" then
            $fortran_mode = true
        else
            $fortran_mode = false
        end
        $base_type = f.gets.chomp
        $skip_nonexecuted = (f.gets.chomp == true.to_s)
        $initial_cfg_fn = f.gets.chomp
        line = f.gets; $prof_log_fn           = (line != nil ? line.chomp      : "")
        line = f.gets; $total_candidates      = (line != nil ? line.chomp.to_i : 0)
        line = f.gets; $rprec_split_threshold = (line != nil ? line.chomp.to_i : 0)
        line = f.gets; $rprec_runtime_pct_threshold = (line != nil ? line.chomp.to_i : 0)
        line = f.gets; $rprec_skip_app_level  = (line != nil ? line.chomp == true.to_s : false)
        line = f.gets; $disable_queue_sort    = (line != nil ? line.chomp == true.to_s : false)
        line = f.gets; $run_final_config      = (line != nil ? line.chomp == true.to_s : false)
        line = f.gets; $mixed_use_rprec       = (line != nil ? line.chomp == true.to_s : false)
    end
    f.close
end


def read_cfg_array(io)
    configs = Array.new
    begin
        line = io.readline.chomp
        if line == "BINARY" then
            configs = Marshal.load(io)
        else
            io.rewind
            YAML.load_stream(io).each do |doc|
                configs << doc
            end
        end
    rescue
    end
    return configs
end

def write_cfg_array(configs, io)
    if $binary_serialization then
        io.puts "BINARY"
        Marshal.dump(configs, io)
    else
        configs.each do |doc|
            YAML.dump(doc, io)
        end
    end
end

def add_to_workqueue_bulk(configs)
    f = File.new("#{$workqueue_fn}", "r+")
    f.flock File::LOCK_EX
    queue = read_cfg_array(f)
    configs.each do |cfg|
        calculate_pct_stats(cfg)
        queue << cfg
        puts "Added config #{cfg.label} to workqueue."
    end
    if not $disable_queue_sort then
        queue.sort!
    end
    f.truncate(0)
    f.pos = 0
    write_cfg_array(queue, f)
    f.close
end

def add_to_workqueue(cfg)
    f = File.new("#{$workqueue_fn}", "r+")
    f.flock File::LOCK_EX
    queue = read_cfg_array(f)
    calculate_pct_stats(cfg)
    queue << cfg
    if not $disable_queue_sort then
        queue.sort!
    end
    
    # debug output
    #puts "Current workqueue:"
    #queue.each do |c|
        #puts "  - #{c.label}"
    #end
    
    f.truncate(0)
    f.pos = 0
    write_cfg_array(queue, f)
    f.close
end
def get_next_workqueue_item
    next_cfg = nil
    f = File.new($workqueue_fn, "r+")
    f.flock File::LOCK_EX
    queue = read_cfg_array(f)
    next_cfg = queue.shift
    f.pos = 0
    write_cfg_array(queue, f)
    f.truncate(f.pos)
    f.close
    return next_cfg
end
def get_workqueue_configs
    f = File.new("#{$workqueue_fn}", "r")
    f.flock File::LOCK_SH
    queue = read_cfg_array(f)
    f.close
    return queue
end
def get_workqueue_length
    len = 0
    f = File.new($workqueue_fn, "r")
    f.flock File::LOCK_SH
    queue = read_cfg_array(f)
    len = queue.size
    f.close
    return len
end


def add_to_inproc(cfg)
    f = File.new($inproc_fn, "r+")
    f.flock File::LOCK_EX
    inproc = read_cfg_array(f)
    inproc << cfg
    f.pos = 0
    write_cfg_array(inproc, f)
    f.truncate(f.pos)
    f.close
end
def remove_from_inproc(cfg)
    inproc = Array.new
    f = File.new($inproc_fn, "r+")
    f.flock File::LOCK_EX
    configs = read_cfg_array(f)
    configs.each do |cfg|
        inproc << cfg if not (cfg.cuid == cfg.cuid)
    end
    f.pos = 0
    write_cfg_array(inproc, f)
    f.truncate(f.pos)
    f.close
end
def get_inproc_configs
    f = File.new("#{$inproc_fn}", "r")
    f.flock File::LOCK_SH
    inproc = read_cfg_array(f)
    f.close
    return inproc
end
def get_inproc_length
    len = 0
    f = File.new($inproc_fn, "r")
    f.flock File::LOCK_SH
    inproc = read_cfg_array(f)
    len = inproc.size
    f.close
    return len
end
def move_all_inproc_to_workqueue
    f = File.new($inproc_fn, "r+")
    f.flock File::LOCK_EX
    configs = read_cfg_array(f)
    add_to_workqueue_bulk(configs)
    f.truncate(0)
    f.close
end


def add_tested_config(cfg)
    f = File.new($tested_fn, "a")
    f.flock File::LOCK_EX
    YAML.dump(cfg, f)
    f.close
end
def get_tested_configs
    tested = Array.new
    f = File.new("#{$tested_fn}", "r")
    f.flock File::LOCK_SH
    YAML.load_stream(f).each do |doc|
        tested << doc
    end
    f.close
    return tested
end
def get_tested_config_count
    len = 0
    f = File.new("#{$tested_fn}", "r")
    f.flock File::LOCK_SH
    YAML.load_stream(f).each do |doc|
        len += 1
    end
    f.close
    return len
end


def load_cached_configs
    if ($cached_configs.size == 0) and (not $cached_fn == "") and (File.exists?($cached_fn)) then
        f = File.new("#{$cached_fn}", "r")
        f.flock File::LOCK_SH
        ydocs = YAML.load_stream(f)
        f.close
        if not ydocs.nil? then
            ydocs.documents.each do |doc|
                $cached_configs << doc
            end
        end
    end
end


def add_to_mainlog(reg)
    f = File.new($mainlog_fn, "a")
    f.flock File::LOCK_EX
    f.puts(reg.to_s)
    f.close
end
def load_mainlog
    f = File.new($mainlog_fn, "r")
    f.flock File::LOCK_SH
    lines = f.readlines
    f.close
    return lines
end


# }}}
