# {{{ file manipulation

# Originally, CRAFT used a multiple-worker thread-pool model, with multiple Ruby
# processes running tests, using files for coordination and synchronization.
# This led to significant scaling problems. Thus, CRAFT has been moved to a
# single-supervisor-process model where there is only ever a single Ruby process
# running these routines. Thus, these routines are now mostly wrappers around
# the in-memory data structures. Changes are flushed out to disk for archival
# and resuming, but there is no longer a need for significant synchronization
# and we do not have to read the files at the beginning of every operation.


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


def read_cfg_array(fn)
    return File.open(fn, File::RDONLY|File::CREAT) do |f|
        configs = Array.new
        begin
            line = f.readline.chomp
            if line == "BINARY" then
                configs = Marshal.load(f)
            else
                f.rewind
                YAML.load_stream(f).each do |doc|
                    configs << doc
                end
            end
        rescue
        end
        configs
    end
end

def write_cfg_array(configs, fn)
    File.open(fn, File::RDWR|File::CREAT) do |f|
        if $binary_serialization then
            f.puts "BINARY"
            Marshal.dump(configs, f)
        else
            configs.each do |doc|
                YAML.dump(doc, f)
            end
        end
        f.truncate(f.pos)
    end
end

def add_to_mainlog(reg)
    File.open($mainlog_fn, File::RDWR|File::APPEND|File::CREAT) { |f| f.puts(reg.to_s) }
end


def load_data_structures
    File.open("#{$workqueue_fn}", File::RDONLY|File::CREAT) do |f|
        read_cfg_array(f).each do |cfg|
            $workqueue << cfg
            $workqueue_lookup[cfg.cuid] = cfg
        end
    end
    File.open("#{$inproc_fn}", File::RDONLY|File::CREAT) do |f|
        read_cfg_array(f).each { |cfg| $inproc[cfg.cuid] = cfg }
    end
    File.open("#{$tested_fn}", File::RDONLY|File::CREAT) do |f|
        read_cfg_array(f).each { |cfg| $tested[cfg.cuid] = cfg }
    end
end


def add_to_workqueue_bulk(configs)
    num_added = 0
    configs.each do |cfg|
        # don't add a config if we've seen another one with an identical CUID
        if not $workqueue_lookup.has_key?(cfg.cuid) and
           not $inproc.has_key?(cfg.cuid) and
           not $tested.has_key?(cfg.cuid) then
            calculate_pct_stats(cfg)
            $workqueue << cfg
            $workqueue_lookup[cfg.cuid] = cfg
            puts "Added config #{cfg.shortlabel} to workqueue."
            num_added += 1
        end
    end
    write_cfg_array($workqueue, $workqueue_fn)
    return num_added
end

def add_to_workqueue(cfg)
    # don't add a config if we've seen another one with an identical CUID
    if not $workqueue_lookup.has_key?(cfg.cuid) and
        not $inproc.has_key?(cfg.cuid) and
        not $tested.has_key?(cfg.cuid) then
        calculate_pct_stats(cfg)
        $workqueue << cfg
        $workqueue_lookup[cfg.cuid] = cfg
        puts "Added config #{cfg.shortlabel} to workqueue."
    end
    write_cfg_array($workqueue, $workqueue_fn)
end
def get_next_workqueue_item
    next_cfg = $workqueue.shift
    $workqueue_lookup.delete(next_cfg.cuid)
    write_cfg_array($workqueue, $workqueue_fn)
    return next_cfg
end
def get_workqueue_configs
    return $workqueue
end
def get_workqueue_length
    return $workqueue.size
end


def add_to_inproc(cfg)
    $inproc[cfg.cuid] = cfg
    write_cfg_array($inproc.values(), $inproc_fn)
end
def remove_from_inproc(cfg)
    $inproc.delete(cfg.cuid)
    write_cfg_array($inproc.values(), $inproc_fn)
end
def get_inproc_configs
    return $inproc.values()
end
def get_inproc_length
    return $inproc.size
end
def move_all_inproc_to_workqueue
    cfgs = $inproc.values()
    cfgs.each { |cfg| $inproc.delete(cfg.cuid) }
    write_cfg_array($inproc.values(), $inproc_fn)
    add_to_workqueue_bulk(cfgs)
end


def add_tested_config(cfg)
    $tested[cfg.cuid] = cfg
    write_cfg_array($tested.values(), $tested_fn)
end
def get_tested_configs
    return $tested.values()
end
def get_tested_config_count
    return $tested.size
end

# }}}
