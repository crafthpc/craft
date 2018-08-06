#!/usr/bin/env ruby
#

# paths
$WIZARD_ROOT    = File.absolute_path("./.craft")
$WIZARD_SANITY  = "#{$WIZARD_ROOT}/sanity"
$WIZARD_ACQUIRE = "#{$WIZARD_ROOT}/wizard_acquire"
$WIZARD_BUILD   = "#{$WIZARD_ROOT}/wizard_build"
$WIZARD_RUN     = "#{$WIZARD_ROOT}/wizard_run"
$WIZARD_VERIFY  = "#{$WIZARD_ROOT}/wizard_verify"

#{{{ input routines

# read a boolean (yes/no) from standard input
def input_boolean(prompt, default)
    if not default.nil? then
        prompt += " [default='#{default ? "y" : "n"}'] "
    end
    print prompt
    r = gets.chomp.downcase
    r = (default ? "y" : "n") if r == "" and not default.nil?
    while not "yn".chars.include?(r)
        puts "Invalid response: #{r}"
        print prompt
        r = gets.chomp.downcase
        r = (default ? "y" : "n") if r == "" and not default.nil?
    end
    return r
end

# read a single-letter option from standard input
def input_option(prompt, valid_opts)
    print prompt
    opt = gets.chomp.downcase
    while not valid_opts.chars.include?(opt)
        puts "Invalid option '#{opt}'"
        print prompt
        opt = gets.chomp.downcase
    end
    return opt
end

# read a path from standard input (optionally check for existence)
def input_path(prompt, default, must_exist=true)
    if not default.nil? then
        prompt += " [default='#{default}'] "
    end
    print prompt
    path = gets.chomp
    ok = false
    while not ok
        ok = true
        if path == "" then
            if default.nil? then
                puts "ERROR: must enter a path"
                ok = false
            else
                path = default
            end
        end
        if must_exist and not Dir.exist?(path) then
            puts "ERROR: #{path} does not exist (or is a regular file)"
            ok = false
        end
        if not ok then
            print prompt
            path = gets.chomp
        end
    end
    return path
end

# }}}

# {{{ exec_cmd - run a command and optionally echo output
def exec_cmd(cmd, echo_stdout=true, echo_stderr=false)
    Open3.popen3(cmd) do |io_in, io_out, io_err|
        io_out.each_line { |line| puts line } if echo_stdout
        io_err.each_line { |line| puts line } if echo_stderr
    end
end # }}}

# {{{ run_wizard - main driver routine
def run_wizard

    puts ""
    puts "== CRAFT Wizard =="
    puts ""
    puts "Welcome to the CRAFT source tuning wizard."
    puts ""

    # make sure configuration folder exists
    if not File.exist?($WIZARD_ROOT) then
        Dir.mkdir($WIZARD_ROOT)
    elsif not Dir.exist?($WIZARD_ROOT) then
        puts "ERROR: #{$WIZARD_ROOT} already exists as a regular file"
        exit
    end

    # print intro text and generate acquisition script
    if not File.exist?($WIZARD_ACQUIRE) then
        puts "To search in parallel automatically, the system must be able to:"
        puts "  1) acquire a copy of your code,"
        puts "  2) build your code using generic CC/CXX variables,"
        puts "  3) run your program using representative inputs, and"
        puts "  4) verify that the output is acceptable."
        puts ""
        puts "How would you like to acquire a copy of your code?"
        puts "  a) Recursive copy from a local folder"
        puts "  b) Clone a git repository"
        opt = input_option("Choose an option above: ", "ab")
        case opt
        when "a"
            path = input_path("Enter project root path: ", ".", true)
            cmd = "cp -rL #{File.absolute_path(path)}/* ."
        when "b"
            print "Enter repository URL: "
            cmd = "git clone #{gets.chomp}"
        end
        File.open($WIZARD_ACQUIRE, 'w') do |f|
            f.puts "#/usr/bin/bash"
            f.puts cmd
        end
        File.chmod(0700, $WIZARD_ACQUIRE)
        puts "Acquisition script created: #{$WIZARD_ACQUIRE}"
        puts ""
    end

    # generate build script
    if not File.exist?($WIZARD_BUILD) then
        puts "How is your project built?"
        puts "  a) \"make\""
        puts "  b) \"./configure && make\""
        puts "  c) \"cmake .\""
        opt = input_option("Choose an option above: ", "abc")
        case opt
        when "a"
            cmd = "make"
        when "b"
            cmd = "./configure && make"
        when "c"
            cmd = "cmake ."
        end
        File.open($WIZARD_BUILD, 'w') do |f|
            f.puts "#/usr/bin/bash"
            f.puts cmd
        end
        File.chmod(0700, $WIZARD_BUILD)
        puts "Build script created: #{$WIZARD_BUILD}"
        puts ""
    end

    # generate run script
    if not File.exist?($WIZARD_RUN) then
        puts "Enter command(s) to run your program with representative input: (empty line to finish)"
        script = []
        line = gets.chomp
        while line != ""
            script << line
            line = gets.chomp
        end
        File.open($WIZARD_RUN, 'w') do |f|
            f.puts "#/usr/bin/bash"
            script.each { |line| f.puts line }
        end
        File.chmod(0700, $WIZARD_RUN)
        puts "Run script created: #{$WIZARD_RUN}"
        puts ""
    end

    # TODO: smooth over run -> verify transition (stdout/stdin?)

    # generate verification script
    if not File.exist?($WIZARD_VERIFY) then
        puts "How should the output be verified?"
        puts "  a) Exact match with original"
        puts "  b) Contains a line matching a regex"
        puts "  c) Contains no lines matching a regex"
        puts "  d) Custom script"
        opt = input_option("Choose an option above: ", "abcd")
        script = []
        case opt
        when "a"
            # TODO: run original and use 'diff'
        when "b"
            # TODO: input regex and use grep/awk
        when "c"
            # TODO: input regex and use grep/awk
        when "d"
            puts "Enter Bash code to verify your program output: (empty line to finish)"
            script = []
            line = gets.chomp
            while line != ""
                script << line
                line = gets.chomp
            end
        end
        File.open($WIZARD_VERIFY, 'w') do |f|
            f.puts "#/usr/bin/bash"
            script.each { |line| f.puts line }
        end
        File.chmod(0700, $WIZARD_VERIFY)
        puts "Verify script created: #{$WIZARD_VERIFY}"
        puts ""
    end

    # run sanity check
    if input_boolean("Do you want to run a sanity check to test the generated scripts?", true) then
        FileUtils.rm_rf $WIZARD_SANITY
        Dir.mkdir $WIZARD_SANITY
        Dir.chdir $WIZARD_SANITY
        exec_cmd $WIZARD_ACQUIRE
        exec_cmd $WIZARD_BUILD
        exec_cmd $WIZARD_RUN
        exec_cmd $WIZARD_VERIFY
    end

    # TODO: build with TypeForge to generate list of variables
    # TODO: instrument and run with ADAPT
    # TODO: generate craft_builder and craft_driver from wizard scripts
    # TODO: run craft search

end # }}}
