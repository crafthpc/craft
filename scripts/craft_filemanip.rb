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
    f.puts "addt_cfg_fns=#{$addt_cfg_fns.join(',')}"
    f.puts "cached_fn=#{$cached_fn}"
    f.puts "prof_log_fn=#{$prof_log_fn}"
    f.puts "total_candidates=#{$total_candidates}"
    f.puts "rprec_split_threshold=#{$rprec_split_threshold}"
    f.puts "rprec_runtime_pct_threshold=#{$rprec_runtime_pct_threshold}"
    f.puts "rprec_skip_app_level=#{$rprec_skip_app_level}"
    f.puts "disable_queue_sort=#{$disable_queue_sort.to_s}"
    f.puts "run_final_config=#{$run_final_config.to_s}"
    f.puts "mixed_use_rprec=#{$mixed_use_rprec.to_s}"
    f.puts "num_trials=#{$num_trials.to_s}"
    f.puts "max_inproc=#{$max_inproc.to_s}"

    f.close
end
def load_settings
    f = File.new($settings_fn, "r")
    f.flock File::LOCK_SH
    parser = /^\s*(\w+)\s*=\s*(.*)\s*$/
    temp = f.gets
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
            $fortran_mode = (value == "fortran")
        when "variable_mode"
            $variable_mode = (value == "yes")
        when "base_type"
            $base_type = value
        when "skip_nonexecuted"
            $skip_nonexecuted = (value == true.to_s)
        when "cached_fn"
            $cached_fn = value
        when "initial_cfg_fn"
            $initial_cfg_fn = value
        when "addt_cfg_fns"
            $addt_cfg_fns = value.split(',')
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
        when "num_trials"
            $num_trials = value.to_i
        when "max_inproc"
            $max_inproc = value.to_i
        end
        temp = f.gets
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
    # don't add a config if we've seen another one with an identical CUID
    load_cached_configs
    seen_cuids = Set.new
    get_inproc_configs.each { |c| seen_cuids << c.cuid }
    get_tested_configs.each { |c| seen_cuids << c.cuid }
    $cached_configs.each    { |c| seen_cuids << c.cuid }

    num_added = 0
    File.open("#{$workqueue_fn}", File::RDWR|File::CREAT) do |f|
        f.flock File::LOCK_EX
        queue = read_cfg_array(f)
        queue.each          { |c| seen_cuids << c.cuid }
        configs.each do |cfg|
            if not seen_cuids.include?(cfg.cuid) then
                calculate_pct_stats(cfg)
                queue << cfg
                seen_cuids << cfg.cuid
                puts "Added config #{cfg.shortlabel} to workqueue."
                num_added += 1
            end
        end
        if not $disable_queue_sort then
            queue.sort!
        end
        f.rewind
        write_cfg_array(queue, f)
        f.truncate(f.pos)
    end
    return num_added
end

def add_to_workqueue(cfg)
    # don't add a config if we've seen another one with an identical CUID
    load_cached_configs
    seen_cuids = Set.new
    get_inproc_configs.each { |c| seen_cuids << c.cuid }
    get_tested_configs.each { |c| seen_cuids << c.cuid }
    $cached_configs.each    { |c| seen_cuids << c.cuid }

    File.open("#{$workqueue_fn}", File::RDWR|File::CREAT) do |f|
        f.flock File::LOCK_EX
        queue = read_cfg_array(f)
        queue.each          { |c| seen_cuids << c.cuid }
        if not seen_cuids.include?(cfg.cuid)
            calculate_pct_stats(cfg)
            queue << cfg
            puts "Added config #{cfg.shortlabel} to workqueue."
            if not $disable_queue_sort then
                queue.sort!
            end
            f.rewind
            write_cfg_array(queue, f)
            f.truncate(f.pos)
        end
    end
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
    f = File.new($inproc_fn, "r+")
    f.flock File::LOCK_EX
    inproc = read_cfg_array(f).select { |c| c.cuid != cfg.cuid }
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
    f = File.new($tested_fn, "r+")
    f.flock File::LOCK_EX
    tested = read_cfg_array(f)
    tested << cfg
    f.pos = 0
    write_cfg_array(tested, f)
    f.truncate(f.pos)
    f.close
end
def get_tested_configs
    f = File.new("#{$tested_fn}", "r")
    f.flock File::LOCK_SH
    tested = read_cfg_array(f)
    f.close
    return tested
end
def get_tested_config_count
    f = File.new("#{$tested_fn}", "r")
    f.flock File::LOCK_SH
    len = read_cfg_array(f).size
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
