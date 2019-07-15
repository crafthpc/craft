# Strategy
#
# Provides rules and procedures governing the search process. This class is
# intended to be inherited and overloaded; this particular implementation does a
# very simple, naive, breadth-first search through the application. It also has
# three simple optimizations:
#
#   1) Ignore program control branches that do not contain any FP instructions
#   2) Ignore program control branches that only have a single child (just do
#      that one instead)
#   3) Do not split passing configurations (e.g., if replacing an entire
#      function passes, do not attempt any subsets of that function)
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

    def handle_completed_config(config, force_add=false)
        if force_add or (config.attrs["result"] != $RESULT_PASS and
                         not is_single_base(config, $base_type)) then

            # add any children we need to test
            configs = split_config(config)
            new_cfgs = Set.new
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
                    puts "Skipping non-executed config #{child.shortlabel}."
                    add_to_mainlog("    Skipping non-executed config #{child.shortlabel}.")
                    child.attrs["result"] = $RESULT_PASS
                    child.attrs["skipped"] = "yes"
                    if $strategy_name == "rprec" then
                        child.precisions.each_key do |k|
                            child.precisions[k] = 0
                        end
                    end
                    calculate_pct_stats(child)
                    add_tested_config(child)
                    add_child = false
                end

                if add_child then
                    new_cfgs << child
                end
            end

            add_to_workqueue_bulk(new_cfgs) if new_cfgs.size > 0
        end
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

    SPLIT_THRESHOLD = 9

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
        if configs.size > SPLIT_THRESHOLD then
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
# Rubio-Gonzalez et al. [SC'13]. This strategy really only works in variable
# mode because no speedup information is available for binary mode.
#
# Note that their notion of a "change set" is the opposite of what we store in a
# configuration. From the paper: "In this algorithm, a change set is a set of
# variables. The variables in the change set must have higher precision." In our
# configurations, however, the PPoints included as exceptions are the variables
# that are to be converted to lower precision. We maintain the authors'
# definition of a "change set" and take the complement of a change set to obtain
# a CRAFT configuration.
#
# {{{ DeltaDebugStrategy
class DeltaDebugStrategy < Strategy

    def has_custom_supervisor?
        return true
    end

    def get_cost(cfg)
        return cfg.attrs["runtime"].to_f
    end

    def build_config(change_set)
        # take complement of change set to obtain replacements
        replacements = @all_variables - change_set

        # generate CUID and label
        cuid  = replacements.map { |pt| pt.id.to_s }.sort.join("_")
        cuid  = "NONE" if cuid == ""
        label = replacements.map { |pt| pt.attrs["desc"] }.sort.join("_")
        label = "NONE" if label == ""

        # create configuration object
        cfg = AppConfig.new(cuid, label, @alternate)
        replacements.each do |pt|
            cfg.add_pt_info(pt)
            cfg.exceptions[pt.uid] = @preferred
            cfg.attrs["level"] = pt.type
        end
        cfg.attrs["changeset"] = change_set
        return cfg
    end

    def handle_completed_config(config)
        # do nothing (custom supervisor routine)
    end

    def run_custom_supervisor

        # get set of all possible basetype-level changes (usually variables)
        @all_variables = []
        find_variables(@program, @all_variables)

        #puts @all_variables.map { |v| v.attrs["desc"] +
                                  #(v.attrs.has_key?("error") ? " " + v.attrs["error"].to_s : "") }

        # sort variables by error (if present; e.g., provided by ADAPT)
        @all_variables.sort! do |x,y|
            if x.attrs.has_key?("error") and y.attrs.has_key?("error") then
                y.attrs["error"].to_f <=> x.attrs["error"].to_f
            else
                x.attrs.has_key?("error") ? -1 : 1
            end
        end

        #puts "SORTED:"
        #puts @all_variables.map { |v| v.attrs["desc"] +
                                  #(v.attrs.has_key?("error") ? " " + v.attrs["error"].to_s : "") }

        # number of divisions at current level
        div = 2

        # lowest-cost change set found so far
        @lc = @all_variables
        @lc_cfg = build_config(@lc)
        @lc_cfg.attrs["runtime"] = $baseline_runtime

        done = false
        while not done do

            puts "Current LC: #{@lc_cfg.exceptions.size} change(s), cost=#{get_cost(@lc_cfg)}, label=#{@lc_cfg.shortlabel}"

            # partition current

            lc_div = div
            divs = @lc.each_slice([1,(@lc.size.to_f/div.to_f).ceil.to_i].max).to_a
            set_cuids = Set.new
            com_cuids = Set.new
            new_cfgs = []
            divs.size.times do |i|

                puts "Test subset #{i+1}/#{divs.size} in round of #{div}"

                # build and test subset
                set_cfg = build_config(divs[i])
                if set_cfg.exceptions.keys.size > 0 then
                    new_cfgs << set_cfg
                    set_cuids << set_cfg.cuid
                end

                # build and test complement set
                com_cfg = build_config(@all_variables - divs[i])
                if com_cfg.exceptions.keys.size > 0 then
                    new_cfgs << com_cfg
                    com_cuids << com_cfg.cuid
                end

            end
            num_added = add_to_workqueue_bulk(new_cfgs)
            puts "ADDED #{num_added} configs to queue"

            # wait until all the previously-added configs are finished
            run_main_search_loop

            # look through the tested configurations and update LC if
            # appropriate
            changed = false
            configs = get_tested_configs
            configs.each do |cfg|
                if cfg.attrs["result"] == $RESULT_PASS and
                        (set_cuids.include?(cfg.cuid) or com_cuids.include?(cfg.cuid)) then
                    puts "EVALUATING: #{cfg.shortlabel}  cost=#{get_cost(cfg)} result=#{cfg.attrs["result"]}"
                    if set_cuids.include?(cfg.cuid) and get_cost(cfg) < get_cost(@lc_cfg) then
                        @lc = cfg.attrs["changeset"]
                        @lc_cfg = cfg
                        lc_div = 2
                        changed = true
                        puts "LC REPLACED by #{cfg.shortlabel}!  cost=#{get_cost(cfg)}"
                    end
                    if com_cuids.include?(cfg.cuid) and get_cost(cfg) < get_cost(@lc_cfg) then
                        @lc = cfg.attrs["changeset"]
                        @lc_cfg = cfg
                        lc_div = div-1
                        changed = true
                        puts "LC REPLACED by #{cfg.shortlabel}!  cost=#{get_cost(cfg)}"
                    end
                end
            end

            # set up next iteration (if not done)
            if changed then
                puts "FOUND NEW LC: #{@lc_cfg.shortlabel}  cost=#{get_cost(@lc_cfg)}"
                div = lc_div
            else
                print "NO NEW LC - "
                if div > @lc.size then
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

    def find_variables(pt, vars)
        if pt.type == @base_type then
            vars << pt
        else
            pt.children.each do |child|
                find_variables(child, vars)
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

        # if ADAPT info is present, filter out non-recommended points
        if is_adapt_info_present?(all_points) then
            all_points.select! { |p| p.attrs.has_key?("error") }
        end

        # build all configurations
        all_configs = []
        1.upto(all_points.size) do |n|
            all_points.combination(n).each do |cmb|
                ids   = cmb.map { |pt| pt.id.to_s }
                names = cmb.map { |pt| pt.attrs["desc"] }
                cfg = AppConfig.new(ids.join("_"), names.join("_"), @alternate)
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

    def is_adapt_info_present?(all_points)
        all_points.each do |p|
            if p.attrs.has_key?("error") then
                return true
            end
        end
        return false
    end

    def split_config(config)
        # don't split
        return Array.new
    end
end # }}}

# CompositionalStrategy
#
# Search algorithm that begins by testing each initial point individually, and
# then repeatedly attempts to build new configurations based on compositions of
# passing configurations.
#
# {{{ CompositionalStrategy
class CompositionalStrategy < Strategy

    SEP="+"

    attr_accessor :skip1card

    def initialize(program, pref, alt, powers_of_two_only=false)
        super(program, pref, alt)
        @skip1card = powers_of_two_only
    end

    def build_initial_configs
        all_configs = []
        find_points(@program).each do |pt|
            cfg = AppConfig.new(pt.id.to_s, pt.attrs["desc"], @alternate)
            cfg.add_pt_info(pt)
            cfg.exceptions[pt.uid] = @preferred
            cfg.attrs["level"] = pt.type
            all_configs << cfg
        end
        return all_configs
    end

    def find_points(pt)
        all_points = []
        if pt.type == @base_type then
            all_points << pt
        else
            pt.children.each { |child| all_points += find_points(child) }
        end
        return all_points
    end

    def handle_completed_config(config)
        return if not config.attrs["result"] == $RESULT_PASS

        # reload information about tested configurations
        good_configs = []
        get_tested_configs.each do |cfg|
            if cfg.attrs["result"] == $RESULT_PASS then
                ck = cfg.exceptions.keys.size
                while good_configs.size < ck+1 do
                    good_configs << []
                end
                good_configs[ck] << cfg
            end
        end

        # build 2k and k+1 cardinality configurations by combining other passing
        # k and 1 cardinality configurations
        k = config.exceptions.keys.size
        (@skip1card ? [k] : [1,k]).uniq.each do |nk|
            good_configs[nk].each do |cfg|

                # get list of combined replacements
                ids   = (config.cuid.split(SEP)  | cfg.cuid.split(SEP)).sort
                next if ids.size != k+nk   # skip if there were duplicates
                names = (config.label.split(SEP) | cfg.label.split(SEP)).sort

                # build new configuration
                new_cfg = AppConfig.new(ids.join(SEP), names.join(SEP), @alternate)
                if config.attrs.has_key?("cinst") and cfg.attrs.has_key?("cinst") then
                    new_cfg.attrs["cinst"] = config.attrs["cinst"] + cfg.attrs["cinst"]
                end
                (config.exceptions.keys | cfg.exceptions.keys).each do |x|
                    new_cfg.exceptions[x] = @preferred
                end
                new_cfg.attrs["level"] = config.attrs["level"]

                add_to_workqueue(new_cfg)
            end
        end
    end
end # }}}

# SimpleCompositionalStrategy
#
# Works like simple when a configuration fails (e.g., split it up) and like
# compositional when a configuration passes (e.g., combine it with other passing
# configurations). The only trick is making sure we don't try to split
# combinations again.
#
# {{{ SimpleCompositionalStrategy
class SimpleCompositionalStrategy < Strategy

    def initialize(program, pref, alt)
        super(program, pref, alt)
        @comp = CompositionalStrategy.new(program, pref, alt, true)
    end

    def handle_completed_config(config)
        if config.attrs["result"] == $RESULT_PASS then
            @comp.handle_completed_config(config)
        elsif not config.cuid.include?(CompositionalStrategy::SEP) then
            super(config)
        end
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

    def handle_completed_config(config)
        reload_bounds_from_results(get_tested_configs)
        super(config, true)
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

