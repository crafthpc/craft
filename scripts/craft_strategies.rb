# Strategy
#
# Provides rules and procedures governing the search process. This class is
# intended to be inherited and overloaded; this particular implementation does a
# very simple, naive, breadth-first search through the application. It also has
# two simple optimizations:
#
#   1) Ignore program control branches that do not contain any FP instructions
#   2) Ignore program control branches that only have a single child (just do
#      that one instead)
#
# {{{ Strategy
class Strategy
    attr_accessor :program
    attr_accessor :preferred
    attr_accessor :alternate
    attr_accessor :base_type

    def initialize(program, pref, alt)
        @preferred = pref
        @alternate = alt
        @program = program
        update_ivcount(@program)
    end

    def has_custom_supervisor?
        return false
    end

    def update_ivcount(point)
        # allow us to ignore branches of the program that don't actually contain
        # any FP instructions or variables
        total = 0
        point.children.each do |child|
            update_ivcount(child)
            total += child.attrs["ivcount"].to_i
        end
        if point.orig_status == $STATUS_CANDIDATE and
                 (point.type == $TYPE_INSTRUCTION or
                  point.type == $TYPE_VARIABLE) then
            total += 1
        end
        point.attrs["ivcount"] = total.to_s
    end

    def build_initial_configs
        # try to replace the entire program
        configs = Array.new
        cfg = AppConfig.new(@program.build_cuid, @program.build_label, @alternate)
        cfg.add_pt_info(@program)
        cfg.exceptions[@program.uid] = @preferred
        cfg.attrs["level"] = $TYPE_APPLICATION
        configs << cfg
        return configs
    end

    def split_config(config)
        # add a config for each child node of each exception
        configs = Array.new
        config.exceptions.each_pair do |k,v|
            pt = @program.lookup_by_uid(k)
            pt.children.each do |child|

                # skip nodes with no FP instructions
                if child.attrs["ivcount"].to_i > 0 then
                    child_pt = child

                    # skip single-child nodes
                    if child.children.size == 1 then
                        child_pt = child.children.first
                    end

                    cfg = AppConfig.new(child_pt.build_cuid, child_pt.build_label, config.default)
                    cfg.add_pt_info(pt)
                    cfg.exceptions[child_pt.uid] = v
                    cfg.attrs["level"] = child_pt.type
                    configs << cfg
                end
            end
        end
        return configs
    end

    def build_final_config(results)
        # simple union of all passing configs
        cfg = AppConfig.new($FINALCFG_CUID, "final", @alternate)
        results.each do |r|
            if r.attrs.has_key?("result") and r.attrs["result"] == $RESULT_PASS then
                r.exceptions.each_pair do |k,v|
                    if cfg.exceptions.has_key?(k) and not (cfg.exceptions[k] == v) then
                        puts "WARNING: Conflicting results for #{cfg.cuid}: #{cfg.exceptions[k]} and #{v}"
                    end
                    cfg.exceptions[k] = v
                end
            end
        end
        return cfg
    end

    def build_fastest_config(results)
        # fastest passing config
        cfg = AppConfig.new($FINALCFG_CUID, "final", @alternate)
        passed_cfgs = Array.new
        results.each do |r|
            if r.attrs.has_key?("result") and r.attrs["result"] == $RESULT_PASS then
                passed_cfgs << [r, r.attrs["runtime"]]
            end
        end
        passed_cfgs = passed_cfgs.sort_by { |cfg,rtime| [rtime, -cfg.cuid.size] }
        if passed_cfgs.size > 0 then
            passed_cfgs[0][0].exceptions.each_pair do |k,v|
                cfg.exceptions[k] = v
            end
        end
        return cfg
    end

end # }}}

# BinaryStrategy
#
# Improved search algorithm that performs a binary search on failed configs
# rather than immediately adding all subchild configs to the queue. It also
# prioritizes new configs using the profiling results.
#
# {{{ BinaryStrategy
class BinaryStrategy < Strategy

    def split_config(config)
        configs = Array.new

        # build list of new configurations
        if config.exceptions.size > 1 then

            # last config tried multiple exceptions;
            # add a config for each exception and try again
            config.exceptions.each_pair do |k,v|
                pt = @program.lookup_by_uid(k)
                cfg = AppConfig.new(pt.build_cuid, pt.build_label, config.default)
                cfg.add_pt_info(pt)
                cfg.exceptions[pt.uid] = v
                cfg.attrs["level"] = pt.type
                configs << cfg
            end
        else

            # add a config for each child node of the current exception
            config.exceptions.each_pair do |k,v|
                pt = @program.lookup_by_uid(k)
                pt.children.each do |child|

                    # skip nodes with no FP instructions
                    # stop at basic blocks if that's desired
                    if child.attrs["ivcount"].to_i > 0 then
                        child_pt = child

                        # skip single-child nodes
                        if child.children.size == 1 and child.type != @base_type then
                            child_pt = child.children.first
                        end

                        cfg = AppConfig.new(child_pt.build_cuid, child_pt.build_label, config.default)
                        cfg.add_pt_info(child_pt)
                        #puts "ADDING #{cfg.cuid} #{child_pt.type}"
                        cfg.exceptions[child_pt.uid] = v
                        cfg.attrs["level"] = child_pt.type
                        configs << cfg
                    end
                end
            end
        end

        # sort in descending order of instruction executions
        configs.sort! { |c1, c2| c2.attrs["cinst"].to_i <=> c1.attrs["cinst"].to_i }

        # merge into two (binary search) if we have a lot of new configs
        if configs.size > 5 then
            pivot = configs.size / 2
            left_cfg  = AppConfig.new(config.cuid + " LEFT",  config.label + " LEFT",  config.default)
            right_cfg = AppConfig.new(config.cuid + " RIGHT", config.label + " RIGHT", config.default)
            left_cfg.attrs["cinst"]  = "0"
            right_cfg.attrs["cinst"] = "0"
            configs.each do |cfg|
                cfg.exceptions.each_pair do |k,v|
                    if pivot > 0 then
                        left_cfg.exceptions[k] = v
                        if cfg.attrs.has_key?("cinst") then
                            left_cfg.attrs["cinst"] = (left_cfg.attrs["cinst"].to_i + cfg.attrs["cinst"].to_i).to_s
                        end
                    else
                        right_cfg.exceptions[k] = v
                        if cfg.attrs.has_key?("cinst") then
                            right_cfg.attrs["cinst"] = (right_cfg.attrs["cinst"].to_i + cfg.attrs["cinst"].to_i).to_s
                        end
                    end
                end
                pivot -= 1
            end
            left_cfg.attrs["level"]  = config.attrs["level"]
            right_cfg.attrs["level"] = config.attrs["level"]
            configs = Array.new
            configs << left_cfg
            configs << right_cfg
        end
        return configs
    end
end # }}}

# DeltaDebugStrategy
#
# Implements a lower-cost delta-debugging strategy as described in
# Rubio-Gonzalez et al. [SC'13].
#
# {{{ DeltaDebugStrategy
class DeltaDebugStrategy < Strategy

    def has_custom_supervisor?
        return true
    end

    def get_cost(cfg)
        if cfg.attrs.has_key?("cinst") and cfg.attrs["cinst"].to_i != 0 then
            return 1.0 / cfg.attrs["cinst"].to_f
        else
            return 1.0
        end
    end

    def run_custom_supervisor
        
        # get set of all possible basetype-level changes
        root = AppConfig.new("ALL", "ALL", @alternate)
        find_configs(@program, root)

        # number of divisions at current level
        div = 2

        # lowest-cost (most replacements) config found so far
        @lc_cfg = root
        @lc_cfg.attrs["cinst"] = 0

        done = false
        while not done do

            puts "Current LC: #{@lc_cfg.exceptions.size} change(s), #{@lc_cfg.attrs["cinst"]} execution(s), cost=#{get_cost(@lc_cfg)}"

            # partition current 
            
            lc_div = div
            divs = @lc_cfg.exceptions.each_slice([1,(@lc_cfg.exceptions.size.to_f/div.to_f).ceil.to_i].max).to_a
            set_cuids = []
            com_cuids = []
            divs.size.times do |i|

                puts "Test #{i+1}/#{divs.size} in round of #{div}"

                # build test subset
                setuid = @lc_cfg.cuid + " S_#{i+1}_#{div}"
                set = AppConfig.new(setuid, setuid, @lc_cfg.default)
                set.attrs["cinst"] = 0
                divs[i].each do |k,v|
                    set.exceptions[k] = v
                    set.attrs["cinst"] += @program.lookup_by_uid(k).attrs["cinst"].to_i
                end
                puts "SET: #{set.exceptions.size} change(s), #{set.attrs["cinst"]} execution(s)"
                add_to_workqueue(set)
                set_cuids << setuid

                # build and test complement set
                comuid = @lc_cfg.cuid + " C_#{i+1}_#{div}"
                com = AppConfig.new(comuid, comuid, @lc_cfg.default)
                com.attrs["cinst"] = 0
                divs.size.times do |j|
                    if i != j then
                        divs[j].each do |k,v|
                            com.exceptions[k] = v
                            com.attrs["cinst"] += @program.lookup_by_uid(k).attrs["cinst"].to_i
                        end
                    end
                end
                puts "COMPLEMENT: #{com.exceptions.size} change(s), #{com.attrs["cinst"]} execution(s)"
                add_to_workqueue(com)
                com_cuids << comuid

            end
            puts "ADDED #{set_cuids.size + com_cuids.size} configs to queue"

            # wait until all the previously-added configs are finished
            run_main_supervisor_loop

            # look through the tested configurations and update LC if
            # appropriate
            changed = false
            configs = get_tested_configs
            configs.each do |cfg|
                #puts "EVALUATING: #{cfg.inspect}  cost=#{get_cost(cfg)}"
                if set_cuids.include?(cfg.cuid) and cfg.attrs["result"] == $RESULT_PASS and
                        get_cost(cfg) < get_cost(@lc_cfg) then
                    @lc_cfg = cfg
                    lc_div = 2
                    changed = true
                    puts "LC REPLACED by #{cfg.cuid}!  cost=#{get_cost(cfg)}"
                end
                if com_cuids.include?(cfg.cuid) and cfg.attrs["result"] == $RESULT_PASS and
                        get_cost(cfg) < get_cost(@lc_cfg) then
                    @lc_cfg = cfg
                    lc_div = div-1
                    changed = true
                    puts "LC REPLACED by #{cfg.cuid}!  cost=#{get_cost(cfg)}"
                end
            end

            # set up next iteration (if not done)
            if changed then
                #puts "FOUND NEW LC: #{@lc_cfg.inspect}  cost=#{get_cost(@lc_cfg)}"
                div = lc_div
            else
                print "NO NEW LC - "
                if div > @lc_cfg.exceptions.size then
                    puts "DONE!"
                    done = true
                else
                    puts "moving to next round (round of #{2*div})"
                    div = 2 * div
                end
            end

            puts ""
        end
    end

    def build_final_config(results)
        final_cfg = AppConfig.new("FINAL", "FINAL", @alternate)
        final_cfg.attrs["cinst"] = 0
        results.each do |r|
            if not @lc_cfg.nil? and r.cuid == @lc_cfg.cuid then
                return r
            end
            if r.attrs.has_key?("result") and r.attrs["result"] == $RESULT_PASS then
                if final_cfg.nil? or get_cost(r) < get_cost(final_cfg) then
                    final_cfg = r
                end
            end
        end
        return final_cfg
    end

    def build_initial_configs
        return []
    end

    def find_configs(pt, cfg)
        if pt.type == @base_type then
            cfg.add_pt_info(pt)
            cfg.exceptions[pt.uid] = @preferred
        else
            pt.children.each do |child|
                find_configs(child, cfg)
            end
        end
    end

    def split_config(config)
        return []
    end
end
# }}}

# ExhaustiveStrategy
#
# Naive search algorithm that searches all instructions immediately. Highly
# parallelizable but generally less effecient than binary searching. Potentially
# useful for pointing out inconsistencies in binary search results.
#
# {{{ ExhaustiveStrategy
class ExhaustiveStrategy < Strategy

    def build_initial_configs
        # find all base_type-level points
        configs = Array.new
        find_configs(@program, configs)
        return configs
    end

    def find_configs(pt, configs)
        if pt.type == @base_type then
            cfg = AppConfig.new(pt.build_cuid, pt.build_label, @alternate)
            cfg.add_pt_info(pt)
            cfg.exceptions[pt.uid] = @preferred
            cfg.attrs["level"] = pt.type
            configs << cfg
        else
            pt.children.each do |child|
                find_configs(child, configs)
            end
        end
    end

    def split_config(config)
        # don't split
        return Array.new
    end

end # }}}

# ExhaustiveCombinationalStrategy
#
# Naive search algorithm that searches all combinations of all instructions
# immediately. Highly parallelizable but enormously ineffecient. Useful as a
# baseline for comparing more intelligent strategies.
#
# {{{ ExhaustiveCombinationalStrategy
class ExhaustiveCombinationalStrategy < Strategy

    def build_initial_configs
        # find all base_type-level points
        all_points = []
        find_points(@program, all_points)

        # build all configurations
        all_configs = []
        1.upto(all_points.size) do |n|
            all_points.combination(n).each do |cmb|
                name = ""
                cmb.each do |pt|
                    name += "_" if name.size > 0
                    name += pt.attrs["desc"]
                end
                cfg = AppConfig.new(name, name, @alternate)
                cmb.each do |pt|
                    cfg.add_pt_info(pt)
                    cfg.exceptions[pt.uid] = @preferred
                    cfg.attrs["level"] = pt.type
                end
                all_configs << cfg
            end
        end
        return all_configs
    end

    def find_points(pt, all_points)
        all_pts = []
        if pt.type == @base_type then
            all_points << pt
        else
            pt.children.each do |child|
                find_points(child, all_points)
            end
        end
    end

    def split_config(config)
        # don't split
        return Array.new
    end
end # }}}

# RPrecStrategy
#
# Search algorithm that find the lowest reduced-precision level for each
# instruction that allows the entire program to pass verification. Performs a
# binary search on the precision level to speed convergence.
#
# {{{ RPrecStrategy
class RPrecStrategy < Strategy

    attr_accessor :lower_bound
    attr_accessor :upper_bound
    attr_accessor :num_iterations
    attr_accessor :split_threshold
    attr_accessor :runtime_pct_threshold
    attr_accessor :skip_app_level

    def initialize(program, pref, alt)
        super(program, pref, alt)
        @lower_bound = Hash.new(-1)
        @upper_bound = Hash.new(52)
        @num_iterations = Hash.new(52)
        @split_threshold = 23               # single-precision by default
        @runtime_pct_threshold = 0.0        # disabled by default
        @skip_app_level = false             # don't skip app level by default
    end

    def reload_bounds_from_results(results)
        @lower_bound = Hash.new(-1)
        @upper_bound = Hash.new(52)
        @num_iterations = Hash.new(0)
        results.each do |cfg|
            cfg.precisions.each_pair do |uid, prec|
                #puts "adjusting #{uid} for result #{cfg.attrs["result"]} at #{prec}"
                if cfg.attrs["result"] == $RESULT_PASS then
                    @upper_bound[uid] = [@upper_bound[uid],prec].min
                else
                    @lower_bound[uid] = [@lower_bound[uid],prec].max
                end
                @num_iterations[uid] = @num_iterations[uid] + 1
            end
        end
    end

    def build_initial_configs
        configs = Array.new
        if @skip_app_level then
            # start at the module level
            @program.children.each do |child|
                child_pt = child

                # skip single-child nodes
                if child.children.size == 1 and child.type != @base_type then
                    child_pt = child.children.first
                end

                add_midpoint_cfg(child_pt, configs)
            end
        else
            # start at the app level
            add_midpoint_cfg(@program, configs)
        end
        return configs
    end

    def add_midpoint_cfg(pt, configs)
        mid = (@upper_bound[pt.uid] - @lower_bound[pt.uid])/2 + @lower_bound[pt.uid]
        #puts "  new mid to test at pt #{pt.uid}: #{mid} in (#{@lower_bound[pt.uid]}, #{@upper_bound[pt.uid]}] cinst=#{pt.attrs["cinst"]}"
        if (pt.attrs["ivcount"].to_i > 0 and
                @upper_bound[pt.uid] > @lower_bound[pt.uid]+1) and
                (mid != @upper_bound[pt.uid]) and (mid != @lower_bound[pt.uid]) then
            tag = ("%02d-bit" % mid)
            cfg = AppConfig.new(pt.build_cuid(tag), pt.build_label(tag), @alternate)
            cfg.add_pt_info(pt)
            cfg.exceptions[pt.uid] = @preferred
            cfg.precisions[pt.uid] = mid
            cfg.attrs["level"] = pt.type
            cfg.attrs["prec"] = mid

            if @runtime_pct_threshold > 0.0 then
                # skip nodes with less than X% of total executions
                calculate_pct_stats(cfg)
                if cfg.attrs["pct_cinst"].to_f > @runtime_pct_threshold then
                    configs << cfg
                end
            else
                configs << cfg
            end
        end
    end

    def split_config(cfg)
        configs = Array.new

        max_upper_bound = 0

        # add new midpoint configs
        cfg.exceptions.each_key do |k|
            pt = @program.lookup_by_uid(k)
            max_upper_bound = [max_upper_bound, @upper_bound[pt.uid]].max
            add_midpoint_cfg(pt, configs)
        end

        # descend if done with this level and above precision-level threshold
        if configs.size == 0 and max_upper_bound > @split_threshold then

            cfg.exceptions.each_pair do |k,v|
                pt = @program.lookup_by_uid(k)
                pt.children.each do |child|

                    # skip nodes with no FP instructions
                    if child.attrs["ivcount"].to_i > 0 then

                        child_pt = child

                        # skip single-child nodes
                        if child.children.size == 1 and child.type != @base_type then
                            child_pt = child.children.first
                        end

                        add_midpoint_cfg(child_pt, configs)
                    end
                end
            end
        end

        return configs
    end

    def build_final_config(results)
        reload_bounds_from_results(results)
        # simple union of all passing configs (or the default upper bound if we
        # were unable to reduce precision at all)
        cfg = AppConfig.new($FINALCFG_CUID, "final", @alternate)
        uids = (@lower_bound.keys + @upper_bound.keys).uniq
        uids.each do |uid|
            prec = @upper_bound[uid]
            cfg.exceptions[uid] = $STATUS_RPREC
            cfg.precisions[uid] = prec
        end
        return cfg
    end

end
# }}}

