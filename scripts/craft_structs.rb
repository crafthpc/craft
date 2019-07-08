# PPoint
#
# Represents a single control point in a program. There is a single APPLICATION
# PPoint at the root, and then it branches into modules, functions, etc. This
# keeps the entire program's structure in memory while only requiring a single
# copy (as older versions of this script did). The PPoint knows how to output
# actual configuration files (given a AppConfig object) that the mutator can use.
#
# {{{ PPoint
class PPoint
    attr_accessor :uid              # unique regex identifier; e.g. "INSN #34: 0x804d3f"
    attr_accessor :id               # program component identifier; e.g. 34 in line above
    attr_accessor :type             # module, function, instruction, etc.
    attr_accessor :orig_status      # single, double, ignore, candidate, none
    attr_accessor :parent           # PPoint
    attr_accessor :children         # array of PPoints
    attr_accessor :attrs            # string => string
    attr_accessor :byuid            # uid => PPoint          (for increased lookup speed)
    attr_accessor :byaddr           # addr => insn PPoint    (for increased lookup speed)

    def initialize (uid, type, orig_status)
        @uid = uid
        @id = 0
        if @uid =~ /#(\d+)/ then
            @id = $1.to_i
        end
        @type = type
        @orig_status = orig_status
        @parent = nil
        @children = Array.new
        @attrs = Hash.new
        @byuid = Hash.new
        @byaddr = Hash.new
    end

    def build_config_file (config, fn, only_insns=true)
        # write out a configuration file for this program and the given config
        # this method will only be called in an APPLICATION node; it calls
        # output() to recursively print the other levels
        fout = File.new(fn, "w")
        if @attrs.has_key?("prolog") then
            @attrs["prolog"].each_line do |line|
                if $mixed_use_rprec and only_insns then
                    if line =~ /^sv_inp/ then
                        if line.chomp == "sv_inp=yes" then
                            fout.puts "r_prec=yes"
                            fout.puts "r_prec_default_precision=52"
                        end
                    else
                        fout.puts line
                    end
                else
                    fout.puts line
                end
            end
        end
        if only_insns then
            output(config, fout, nil, nil)
        else
            output_all(config, fout)
        end
        fout.close
    end

    def build_typeforge_file (config, fn)
        File.open(fn, "w") do |f|
            output_typeforge(config, f, nil, "$global")
        end
    end

    def build_json_file (config, fn)
        fout = Hash.new
        fout["version"] = "1"
        fout["tool_id"] = "CRAFT"
        fout["source_files"] = []
        actions = []
        output_json(config, actions, nil)
        fout["actions"] = actions
        attrs = Hash.new
        config.attrs.each { |k,v| attrs[k] = v.to_s }
        fout["#{$search_tag}_attrs"] = attrs
        File.open(fn, "w") do |f|
            f.puts(JSON.pretty_generate(fout))
        end
    end

    def build_lookup_hash(pt=nil)
        if pt.nil? then
            @byuid[@uid] = self
            @children.each do |child|
                build_lookup_hash(child)
            end
        else
            if pt.type == $TYPE_INSTRUCTION and pt.attrs.has_key?("addr") then
                @byaddr[pt.attrs["addr"]] = pt
            end
            @byuid[pt.uid] = pt
            pt.children.each do |child|
                build_lookup_hash(child)
            end
        end
    end

    def lookup_by_uid(uid)
        return @byuid[uid]
    end

    def lookup_by_addr(addr)
        return @byaddr[addr]
    end

    def build_cuid(tag = nil)
        cuid = @uid
        if not tag.nil? then
            cuid += " #{tag}"
        end
        return cuid
    end

    def build_label(tag = nil)
        # build human-readable label
        label = @uid
        if tag.nil? then
            tag = ""
        else
            tag += " "
        end
        if @uid.include?($TYPE_APPLICATION) then
            label = "APP #{tag}#{@attrs["desc"]}"
        elsif @uid.include?($TYPE_MODULE) then
            label = "MOD #{tag}#{@attrs["desc"]}"
        elsif @uid.include?($TYPE_FUNCTION) then
            label = "FUNC #{tag}#{@attrs["desc"]}"
        elsif @uid.include?($TYPE_BASICBLOCK) then
            label = "BBLK #{@attrs["addr"]} #{tag}"
        elsif @uid.include?($TYPE_INSTRUCTION) then
            label = "INSN #{@attrs["addr"]} #{tag}\"#{@attrs["desc"]}\""
        elsif @uid.include?($TYPE_VARIABLE) then
            label = "VAR #{@attrs["addr"]} #{tag}\"#{@attrs["desc"]}\""
        end
        return label.strip
    end

    def output_all(config, fout)
        prec_line = nil
        fout.print("^")
        if config.exceptions.has_key?(@uid) then
            # status overridden by current config
            fout.print(config.exceptions[@uid])
            if config.exceptions[@uid] == $STATUS_RPREC then
                prec_line = "#{type}_#{@id}_precision=#{config.precisions[@uid]}"
            end
        else
            if @orig_status == $STATUS_CANDIDATE then
                # candidate for replacement; output configuration default
                fout.print(config.default)
            else
                # no override; output original default
                fout.print(@orig_status)
            end
        end
        fout.print(" ")
        fout.print($TYPE_SPACES[@type])     # indentation
        fout.print(@uid)
        if @attrs.has_key?("desc") then
            fout.print(" \"")
            fout.print(@attrs["desc"])
            fout.print("\"")
        end
        fout.print("\n")
        if not prec_line.nil? then
            fout.puts prec_line
        end
        @children.each do |pt|
            pt.output_all(config, fout)
        end
    end

    def output(config, fout, overload, overload_prec)
        prec_line = nil
        fout.print("^")
        if type == $TYPE_INSTRUCTION then
            flag = @orig_status
            if @orig_status == $STATUS_CANDIDATE then
                # candidate for replacement; check to see if we should
                if not overload.nil? then
                    # status overridden by something higher up the tree
                    flag = overload
                    if overload == $STATUS_RPREC then
                        prec_line = "#{type}_#{@id}_precision=#{overload_prec}"
                    end
                else
                    if config.exceptions.has_key?(@uid) then
                        # status overridden by current config
                        flag = config.exceptions[@uid]
                        if config.exceptions[@uid] == $STATUS_RPREC then
                            prec_line = "#{type}_#{@id}_precision=#{config.precisions[@uid]}"
                        end
                    else
                        # no override; output configuration default
                        flag = config.default
                    end
                end
            end
            if $mixed_use_rprec then
                if flag == $STATUS_SINGLE then
                    flag = $STATUS_RPREC
                    prec_line = "#{type}_#{@id}_precision=#{$rprec_split_threshold}"
                elsif flag == $STATUS_DOUBLE then
                    flag = $STATUS_RPREC
                    prec_line = "#{type}_#{@id}_precision=52"
                end
            end
            fout.print(flag)
        else
            if overload.nil? then
                if config.exceptions.has_key?(@uid) then
                    # children status overridden by current config
                    overload = config.exceptions[@uid]
                    if overload == $STATUS_RPREC then
                        overload_prec = config.precisions[@uid]
                    end
                elsif not @orig_status == $STATUS_NONE then
                    # children status overridden by original config
                    overload = @orig_status
                end
            end
            fout.print(" ")
        end
        fout.print(" ")
        fout.print($TYPE_SPACES[@type])     # indentation
        fout.print(@uid)
        if @attrs.has_key?("desc") then
            fout.print(" \"")
            fout.print(@attrs["desc"])
            fout.print("\"")
        end
        fout.print("\n")
        if not prec_line.nil? then
            fout.puts prec_line
        end
        @children.each do |pt|
            pt.output(config, fout, overload, overload_prec)
        end
    end

    def output_typeforge (config, fout, overload=nil, func_name="")
        if @type == $TYPE_VARIABLE then
            flag = @orig_status
            if flag == $STATUS_CANDIDATE then
                if not overload.nil? then
                    flag = overload
                elsif config.exceptions.has_key?(@uid) then
                    flag = config.exceptions[@uid]
                end
                if flag == $STATUS_SINGLE then
                    fout.puts "change_var_type;#{func_name};#{@attrs["desc"]};float"
                end
            end
        else
            if config.exceptions.has_key?(@uid) then
                overload = config.exceptions[@uid]
            end
            if @type == $TYPE_FUNCTION then
                func_name = @attrs["desc"]
            end
            @children.each do |c|
                c.output_typeforge(config, fout, overload, func_name)
            end
        end
    end

    def output_json (config, actions, overload=nil)
        if @type == $TYPE_VARIABLE then
            flag = @orig_status
            if flag == $STATUS_CANDIDATE then
                if not overload.nil? then
                    flag = overload
                elsif config.exceptions.has_key?(@uid) then
                    flag = config.exceptions[@uid]
                end
                if flag == $STATUS_SINGLE then
                    a = Hash.new
                    a["uid"] = @id.to_s
                    a["action"] = "change_var_basetype"
                    a["handle"] = @attrs["addr"]
                    a["name"] = @attrs["desc"]
                    a["scope"] = @attrs["scope"] if @attrs.has_key?("scope")
                    a["source_info"] = @attrs["source_info"] if @attrs.has_key?("source_info")
                    a["to_type"] = "float"
                    a["labels"] = @attrs["labels"] if @attrs.has_key?("labels")
                    actions << a
                end
            end
        else
            if config.exceptions.has_key?(@uid) then
                overload = config.exceptions[@uid]
            end
            @children.each do |c|
                c.output_json(config, actions, overload)
            end
        end
    end
end # }}}

# AppConfig
#
# Represents a particular replacement configuration. To save space, an
# exception-based scheme is used; it keeps a "default" replacement status for
# all instructions that are not covered by an exception. Any exceptions at a
# level higher than an instruction (module, function, etc.) propogate to
# lower levels.
#
# {{{ AppConfig
class AppConfig
    attr_accessor :cuid             # unique identifier (string)
    attr_accessor :label            # human-readable description; e.g. "MODULE #1: boxes.c; MODULE #2: spheres.c"
    attr_accessor :default          # status
    attr_accessor :exceptions       # cuid -> status
    attr_accessor :attrs            # string => string

    attr_accessor :defprec          # default precision (int)
    attr_accessor :precisions       # cuid -> int

    MAX_SHORTLABEL_LENGTH=64

    def initialize (cuid, label, default)
        @cuid = cuid
        @label = label
        @default = default
        @exceptions = Hash.new
        @attrs = Hash.new
        @defprec = 52
        @precisions = Hash.new
    end

    def <=> (c2)
        result = nil

        # ascending level rank -> descending executed %
        if @attrs.has_key?("level") and c2.attrs.has_key?("level") then
            l1 = @attrs["level"]
            l2 = c2.attrs["level"]
            if l1 == l2 then
                if @attrs.has_key?("pct_cinst") and c2.attrs.has_key?("pct_cinst") then
                    oc1 = @attrs["pct_cinst"].to_f
                    oc2 = c2.attrs["pct_cinst"].to_f
                    if oc1 == oc2 then
                        result = -1
                    else
                        result = oc2 <=> oc1
                    end
                else
                    result = -1
                end
            else
                result = $TYPE_RANK[l1] <=> $TYPE_RANK[l2]
            end
        else
            result = -1
        end

        # descending instrumented % + executed %
        if @attrs.has_key?("pct_icount") and c2.attrs.has_key?("pct_icount") and
           @attrs.has_key?("pct_cinst") and c2.attrs.has_key?("pct_cinst") then
            oi1 = @attrs["pct_icount"].to_f
            oi2 = c2.attrs["pct_icount"].to_f
            oc1 = @attrs["pct_cinst"].to_f
            oc2 = c2.attrs["pct_cinst"].to_f
            result = (oi2+oc2) <=> (oi1+oc1)
        end

        return result
    end

    def filename (include_ext = true)
        # sanitized cuid for uniqueness
        fn = ""
        @cuid.each_char do |c|
            if c =~ /[0-9a-zA-Z_]/ then
                fn << c.downcase
            elsif c == ' ' then
                fn << "_"
            end
        end
        # add sanitized description for readability (in some cases)
        if @attrs.has_key?("desc") and (@type == $TYPE_MODULE or
                                        @type == $TYPE_FUNCTION or
                                        @type == $TYPE_VARIABLE) then
            fn << "_"
            @attrs["desc"].each_char do |c|
                if c =~ /[0-9a-zA-Z_]/ then
                    fn << c.downcase
                elsif c == ' ' then
                    fn << "_"
                end
            end
        else
            fn << "_"
            @label.each_char do |c|
                if c =~ /[0-9a-zA-Z_]/ then
                    fn << c.downcase
                elsif c == ' ' then
                    fn << "_"
                end
            end
        end
        # if file size is too long, use hash instead
        if fn.size > 32 then
            fn = "#{fn.hash.abs.to_s(16)}"
        end
        fn << ($variable_mode ? ".json" : ".cfg") if include_ext
        return fn
    end

    def shortlabel
        return format_text(@label, MAX_SHORTLABEL_LENGTH)
    end

    def add_pt_info(pt)
        if pt.attrs.has_key?("cinst") then
            @attrs["cinst"] = pt.attrs["cinst"]
        else
            @attrs["cinst"] = 0
        end
        @attrs["labels"] = pt.attrs["labels"] if pt.attrs.has_key?("labels")
    end

end # }}}

