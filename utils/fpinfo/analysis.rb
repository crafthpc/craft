#!/usr/bin/env ruby
#
# analysis.rb - fpinfo analysis script
#
# This script analyses the output of a rewritten mutatee created by the 'fpinfo'
# utility. It takes the raw instruction count information produced by that
# mutatee, performs analytics on the information, and outputs various
# statistics and aggregation data.
#
# Originally written by Mike Lam
# May 2014
#


# =====================================================================
#
# InsnInfo - holds info for a single instruction
#

class InsnInfo

    attr_accessor :addr
    attr_accessor :func
    attr_accessor :disas
    attr_accessor :opcode
    attr_accessor :tags
    attr_accessor :rawcount
    attr_accessor :count

    def to_s
        return "%15s %d %5.1f"%[@opcode,@rawcount,@count.pct_dyn]
    end

end


# =====================================================================
#
# InsnCount - holds a set of counts (static and dynamic, raw and percentage)
#             and can collapse other counts into itself; also contains a
#             hold-anything hashtable (currently used to store opcodes for
#             aggregation counts)
#

class InsnCount

    attr_accessor :raw_sta
    attr_accessor :raw_dyn
    attr_accessor :pct_sta
    attr_accessor :pct_dyn
    attr_accessor :misc

    def initialize(sta, dyn)
        @raw_sta = sta
        @raw_dyn = dyn
        @pct_sta = 0.0
        @pct_dyn = 0.0
        @misc = Hash.new
    end

    def calc_pcts(view)
        @pct_sta = @raw_sta.to_f / view.total_sta.to_f * 100.0
        @pct_dyn = @raw_dyn.to_f / view.total_dyn.to_f * 100.0
    end

    def add(count)
        @raw_sta += count.raw_sta
        @raw_dyn += count.raw_dyn
    end

    def add_w_misc(count,key,value)
        add(count)
        @misc[key] = value
    end

    def self.hdr
        return "%15s   %5s %15s   %5s"%["STA","STA%","DYN","DYN%"]
    end

    def to_s(view = nil)
        if not view.nil? then
            calc_pcts(view)
        end
        return "%15d   %5.1f %15d   %5.1f"%[@raw_sta,@pct_sta,@raw_dyn,@pct_dyn]
    end

end


# =====================================================================
#
# InsnView - represents a view or filter of count data, providing aggregation
#            structures and running totals
#

class InsnView

    attr_accessor :insns
    attr_accessor :label
    attr_accessor :tag
    attr_accessor :predicate
    attr_accessor :total_sta
    attr_accessor :total_dyn

    attr_accessor :by_opcode
    attr_accessor :by_func
    attr_accessor :by_itype
    attr_accessor :by_dtype
    attr_accessor :by_memacc

    def initialize(tag, label, pred)
        @tag = tag
        @label = label
        @predicate = pred
        @total_sta = 0
        @total_dyn = 0
        @by_opcode = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
        @by_func   = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
        @by_itype  = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
        @by_dtype  = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
        @by_memacc = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
    end

    def add(insn)
        if @predicate.call(insn) then
            @insns << insn
        end
    end

    def report(data,rtag,rlabel,header,misc = false)

        # set up graph files
        pdata = ""
        names = ""
        pfn = "#{tag}-#{rtag}.plot"
        gfn = "#{tag}-#{rtag}.png"
        
        # output data
        puts "\n\n  ==  #{rlabel.upcase}  =="
        puts ""
        puts "#{"%50s"%[header.upcase]} #{InsnCount.hdr}"
        data.keys.sort.each do |k|
            puts "#{"%50s"%[k]} #{data[k].to_s(self)}"
            pdata += "#{pdata == "" ? "" : ","}#{data[k].pct_dyn}"
            names += "#{names == "" ? "" : ","}\"#{k}\""
        end
        if (misc) then
            puts ""
            data.keys.sort.each do |k|
                puts "#{"%10s"%[k]} #{data[k].misc.keys.sort.join(", ")}"
            end
        end

        # generate graphs (currently using R)
        fplot = File.open(pfn, "w")
        fplot.puts "data <- c(#{pdata})"
        fplot.puts "lbls <- c(#{names})"
        fplot.puts "png(\"#{gfn}\", height=600, width=600, bg=\"white\")"
        fplot.puts "par(mai=c(0.75,1.25,0.75,0.25))"
        fplot.puts "barplot(data, names.arg=lbls)"
        fplot.puts "title(ylab=\"DYN %\")"
        fplot.puts "title(main=\"#{@label} - #{rlabel}\")"
        fplot.close
        system("Rscript #{pfn}")    # run R
    end

end


# =====================================================================
#
# Global structures and startup code
#
#

# hard-coded instruction type prefixes
$itype_prefixes = Hash.new
$itype_prefixes["arith"] = "add,sub,mul,div,imul,idiv,min,max"
$itype_prefixes["transcend"] = "sqrt,sin,cos,tan"
$itype_prefixes["bitwise"] = "and,or,xor,sal,sar,shl,shr,test"
$itype_prefixes["move"] = "mov,cmov,lea"
$itype_prefixes["compare"] = "cmp,com,ucom"
$itype_prefixes["convert"] = "cvt,cbw,cwde,cdqe,cwd,cdq,cqo,set"
$itype_prefixes["stack"] = "push,pop"
$itype_prefixes["control"] = "call,j,ret"

# big bag o' instructions
$all_insns = Array.new

# global options
$clean_funcs = true

# open SQL output file
sqlf = File.open("analysis.sql", "w")
sqlf.puts "CREATE TABLE Insns\n(\n" +
          "  Addr   varchar(16),\n" + 
          "  Raw    int,\n" +
          "  Func   varchar(100),\n" +
          "  Opcode varchar(100),\n" +
          "  Disas  varchar(255)\n" +
          ");"

# create filtered views
$all_views = Array.new
$top_view = InsnView.new("all", "All Instructions",
                         lambda { |i| true })
$all_views << $top_view
$all_views << InsnView.new("fp", "FPInst Instructions",
                           lambda { |i| i.tags.include?("fp") })




# =====================================================================
#
# PASS 1: read raw counts from input file
#         O(i)
#
#

ARGF.each do |line|
    if line =~ /^\s*([0-9a-fA-F]+): (\d+)\s*(.*)\s*"(.*)" \{(.*)\}$/ then

        # basic instruction info
        insn = InsnInfo.new
        insn.addr = $1.strip
        insn.rawcount = $2.to_i
        insn.func = $3.strip
        insn.disas = $4.strip
        insn.tags = $5.split(",")

        # truncate function name to 50 characters (STL is ugly)
        if $clean_funcs and insn.func =~ /([^:]+)$/ then
            insn.func = $1
        end
        if insn.func.size > 50 then
            insn.func = insn.func[insn.func.size-50,50]
        end

        # extract opcode from disassembly
        if insn.disas =~ /^([^\s]+)/ then
            insn.opcode = $1
        end

        # create insn-specific count object
        insn.count = InsnCount.new(1, insn.rawcount)
        $all_insns << insn

        # SQL output
        sqlf.puts "INSERT INTO Insns VALUES\n" +
                  "(\"#{insn.addr}\", #{insn.rawcount}, \"#{insn.func}\", " +
                  "\"#{insn.opcode}\", \"#{insn.disas}\");"
    end
end



# =====================================================================
#
# PASS 2: calculate instruction-level percentages
#         and populate aggregation structures
#         O(iv)
#
#

$all_insns.each do |insn|
    $all_views.each do |view|
        if view.predicate.call(insn) then

            view.total_sta += insn.count.raw_sta
            view.total_dyn += insn.count.raw_dyn

            view.by_opcode[insn.opcode].add(insn.count)
            view.by_func[insn.func].add(insn.count)

            typed = false
            $itype_prefixes.each_pair do |t,p|
                if !typed then
                    p.split(",").each do |prefix|
                        if insn.opcode =~ Regexp.new("^#{prefix}") then
                            view.by_itype[t].add_w_misc(insn.count, insn.opcode, 1)
                            typed = true
                        end
                    end
                end
            end
            if !typed then
                view.by_itype["zmisc"].add_w_misc(insn.count, insn.opcode, 1)
            end

            if insn.opcode =~ /ss$/ or insn.opcode =~ /ps$/ then
                view.by_dtype["fp32"].add_w_misc(insn.count, insn.opcode, 1)
            elsif insn.opcode =~ /sd$/ or insn.opcode =~ /pd$/ then
                view.by_dtype["fp64"].add_w_misc(insn.count, insn.opcode, 1)
            elsif insn.disas =~ /XMM/ then
                view.by_dtype["ixmm"].add_w_misc(insn.count, insn.opcode, 1)
            else
                view.by_dtype["else"].add_w_misc(insn.count, insn.opcode, 1)
            end

            if insn.tags.include?("mr") and insn.tags.include?("mw") then
                view.by_memacc["both"].add_w_misc(insn.count, insn.opcode, 1)
            elsif insn.tags.include?("mr") then
                view.by_memacc["read"].add_w_misc(insn.count, insn.opcode, 1)
            elsif insn.tags.include?("mw") then
                view.by_memacc["write"].add_w_misc(insn.count, insn.opcode, 1)
            else
                view.by_memacc["none"].add_w_misc(insn.count, insn.opcode, 1)
            end

        end
    end
end



# =====================================================================
#
# PASS 3: output statistics (should NOT need to iterate all insns)
#         O(v)
#
#
#

# SQL version
sqlf.puts "SELECT * FROM Insns;"
sqlf.puts "SELECT Opcode, " +
          "COUNT(Raw), COUNT(Raw)*100.0/(SELECT COUNT(Raw) FROM Insns), " +
          "SUM(Raw), SUM(Raw)*100.0/(SELECT SUM(Raw) FROM Insns) FROM Insns " +
          "GROUP BY Opcode;"
sqlf.puts "SELECT Func, " +
          "COUNT(Raw), COUNT(Raw)*100.0/(SELECT COUNT(Raw) FROM Insns), " +
          "SUM(Raw), SUM(Raw)*100.0/(SELECT SUM(Raw) FROM Insns) FROM Insns " +
          "GROUP BY Func;"


$all_views.each do |view|

    puts "\n\n=== #{view.label.upcase} ==="
    puts ""
    puts "TOTAL STA: %-15d %5.1f"%[view.total_sta, view.total_sta.to_f / $top_view.total_sta.to_f * 100.0]
    puts "TOTAL DYN: %-15d %5.1f"%[view.total_dyn, view.total_dyn.to_f / $top_view.total_dyn.to_f * 100.0]

    # all instructions; currently disabled because it is rather lengthy
    #puts "\n\n  ==  ALL  =="
    #puts ""
    #puts "#{"%20s"%["ADDRESS"]} #{InsnCount.hdr}   DISASSEMBLY"
    #$all_insns.each do |insn|
        #if view.predicate.call(insn) then
            #puts "#{"%20s"%[insn.addr]} #{insn.count.to_s(view)}   #{insn.disas}"
        #end
    #end

    view.report(view.by_opcode, "opcode", "By Opcode",           "Opcode",      false)
    view.report(view.by_func,   "func",   "By Function",         "Function",    false)
    view.report(view.by_itype,  "itype",  "By Instruction Type", "Insn Type",   true)
    view.report(view.by_dtype,  "dtype",  "By Data Type",        "Data Type",   true)
    view.report(view.by_memacc, "memacc", "By Mem Access Type",  "Access Type", true)

end

sqlf.close

