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
        calc_pcts
        @misc = Hash.new
    end

    def calc_pcts
        @pct_sta = @raw_sta.to_f / $total_sta.to_f * 100.0
        @pct_dyn = @raw_dyn.to_f / $total_dyn.to_f * 100.0
    end

    def add(count)
        @raw_sta += count.raw_sta
        @raw_dyn += count.raw_dyn
        @pct_sta = @raw_sta.to_f / $total_sta.to_f * 100.0
        @pct_dyn = @raw_dyn.to_f / $total_dyn.to_f * 100.0
    end

    def add_w_misc(count,key,value)
        add(count)
        @misc[key] = value
    end

    def self.hdr
        return "%15s   %5s %15s   %5s"%["STA","STA%","DYN","DYN%"]
    end

    def to_s
        return "%15d   %5.1f %15d   %5.1f"%[@raw_sta,@pct_sta,@raw_dyn,@pct_dyn]
    end
end


# =====================================================================
#
# Global structures and startup code
#
#

# hard-coded instruction type prefixes
itype_prefixes = Hash.new
itype_prefixes["arith"] = "add,sub,mul,div,imul,idiv,min,max"
itype_prefixes["transcend"] = "sqrt,sin,cos,tan"
itype_prefixes["bitwise"] = "and,or,xor,sal,sar,shl,shr,test"
itype_prefixes["move"] = "mov,cmov,lea"
itype_prefixes["compare"] = "cmp,com,ucom"
itype_prefixes["convert"] = "cvt,cbw,cwde,cdqe,cwd,cdq,cqo,set"
itype_prefixes["stack"] = "push,pop"
itype_prefixes["control"] = "call,j,ret"

# big bag o' instructions
all_insns = Array.new

# total # of instructions and total # of insn executions
$total_sta = 0
$total_dyn = 0

# open SQL output file
sqlf = File.open("analysis.sql", "w")
sqlf.puts "CREATE TABLE Insns\n(\n" +
          "  Addr   varchar(16),\n" + 
          "  Raw    int,\n" +
          "  Func   varchar(100),\n" +
          "  Opcode varchar(100),\n" +
          "  Disas  varchar(255)\n" +
          ");"



# =====================================================================
#
# PASS 1: read raw counts from input file
#         and calculate program-wide totals
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
        if insn.func.size > 50 then
            insn.func = insn.func[insn.func.size-50,50]
        end

        # extract opcode from disassembly
        if insn.disas =~ /^([^\s]+)/ then
            insn.opcode = $1
        end

        # create insn-specific count object and update global totals
        insn.count = InsnCount.new(1, insn.rawcount)
        all_insns << insn
        $total_sta += 1
        $total_dyn += insn.rawcount

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
#
#

by_opcode = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
by_func   = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
by_itype  = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
by_dtype  = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }
by_memacc = Hash.new { |h,k| h[k] = InsnCount.new(0,0) }

all_insns.each do |insn|
    insn.count.calc_pcts

    by_opcode[insn.opcode].add(insn.count)
    by_func[insn.func].add(insn.count)

    typed = false
    itype_prefixes.each_pair do |t,p|
        if !typed then
            p.split(",").each do |prefix|
                if insn.opcode =~ Regexp.new("^#{prefix}") then
                    by_itype[t].add_w_misc(insn.count, insn.opcode, 1)
                    typed = true
                end
            end
        end
    end
    if !typed then
        by_itype["zmisc"].add_w_misc(insn.count, insn.opcode, 1)
    end

    if insn.opcode =~ /ss$/ or insn.opcode =~ /ps$/ then
        by_dtype["fp32"].add_w_misc(insn.count, insn.opcode, 1)
    elsif insn.opcode =~ /sd$/ or insn.opcode =~ /pd$/ then
        by_dtype["fp64"].add_w_misc(insn.count, insn.opcode, 1)
    elsif insn.disas =~ /XMM/ then
        by_dtype["ixmm"].add_w_misc(insn.count, insn.opcode, 1)
    else
        by_dtype["else"].add_w_misc(insn.count, insn.opcode, 1)
    end

    if insn.tags.include?("mr") and insn.tags.include?("mw") then
        by_memacc["both"].add_w_misc(insn.count, insn.opcode, 1)
    elsif insn.tags.include?("mr") then
        by_memacc["read"].add_w_misc(insn.count, insn.opcode, 1)
    elsif insn.tags.include?("mw") then
        by_memacc["write"].add_w_misc(insn.count, insn.opcode, 1)
    else
        by_memacc["none"].add_w_misc(insn.count, insn.opcode, 1)
    end

end



# =====================================================================
#
# PASS 3+: output statistics (should NOT need to iterate all insns)
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

# all instructions; currently disabled because it is rather lengthy
#puts "\n\n===  ALL  ==="
#puts ""
#puts "#{"%20s"%["ADDRESS"]} #{InsnCount.hdr}   DISASSEMBLY"
#all_insns.each do |insn|
    #puts "#{"%20s"%[insn.addr]} #{insn.count.to_s}   #{insn.disas}"
#end

puts "\n\n===  BY OPCODE  ===="
puts ""
puts "#{"%50s"%["OPCODE"]} #{InsnCount.hdr}"
by_opcode.keys.sort.each do |k|
    puts "#{"%50s"%[k]} #{by_opcode[k].to_s}"
end

puts "\n\n===  BY FUNCTION  ==="
puts ""
puts "#{"%50s"%["FUNCTION"]} #{InsnCount.hdr}"
by_func.keys.sort.each do |k|
    puts "#{"%50s"%[k]} #{by_func[k].to_s}"
end

puts "\n\n===  BY INSTRUCTION TYPE  ==="
puts ""
puts "#{"%50s"%["INSN TYPE"]} #{InsnCount.hdr}"
by_itype.keys.sort.each do |k|
    puts "#{"%50s"%[k]} #{by_itype[k].to_s}"
end
puts ""
by_itype.keys.sort.each do |k|
    puts "#{"%10s"%[k]} #{by_itype[k].misc.keys.sort.join(", ")}"
end

puts "\n\n===  BY DATATYPE  ==="
puts ""
puts "#{"%50s"%["DATA TYPE"]} #{InsnCount.hdr}"
by_dtype.keys.sort.each do |k|
    puts "#{"%50s"%[k]} #{by_dtype[k].to_s}"
end
puts ""
by_dtype.keys.sort.each do |k|
    puts "#{"%10s"%[k]} #{by_dtype[k].misc.keys.sort.join(", ")}"
end

puts "\n\n===  BY MEM ACCESS  ==="
puts ""
puts "#{"%50s"%["ACCESS TYPE"]} #{InsnCount.hdr}"
by_memacc.keys.sort.each do |k|
    puts "#{"%50s"%[k]} #{by_memacc[k].to_s}"
end
puts ""
by_memacc.keys.sort.each do |k|
    puts "#{"%10s"%[k]} #{by_memacc[k].misc.keys.sort.join(", ")}"
end

puts ""
puts "TOTAL STA: #{$total_sta}"
puts "TOTAL DYN: #{$total_dyn}"
puts ""

sqlf.close

