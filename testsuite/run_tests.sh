#!/bin/bash

# change this to set any global mutator flags
FPINST=fpinst

# change this to set which optimization levels are tested
opts="-O0 -O1 -O2 -O3"
#opts="-O0"

# change this to set which instrumentation modes are tested
#modes="dcancel svinp_single svinp_double"
#modes="svinp"
modes="cinst dcancel dnan trange svinp svinp_double"
#modes="svinp svinp_double"

# change this to set which tests are run
#tests="pushpop"    # old x87-based test
#tests="lulesh"     # lulesh won't accept -mno-80387 with -O1 or higher
tests="arith calllib compare convert gauss intcopy mgbarsky mixed modes packed powloop"
#tests="modes"

echo ""
echo "===  FPANALYSIS TEST SUITE  ==="
echo "Optimization levels: $opts"
echo "Analysis modes: $modes"
echo "Mutatees: $tests"

# make sure there aren't any old rewritten libraries hanging around
rm -f libc.so.6
rm -f libm.so.6

# clear out old results files
for t in $tests
do
    dir=results/$t
    for m in $modes
    do
        rm -f $dir/$m*
    done
done

START=$(date +%s)

# for each optimization level
# (must recompile mutatees each time)
for opt in $opts
do

    # set optimization level (read by Makefile)
    export CFLAGS=$opt
    echo ""
    echo "== OPTIMIZATION LEVEL $opt"

    # sanity test: rebuild and check unmodified mutatees
    make clean
    for t in $tests
    do
        make $t &>/dev/null
        if [ $? -ne 0 ]
        then
            make $t
            exit
        fi
        ./$t &>/dev/null
        if [ $? -ne 0 ]
        then
            echo "ERROR: Unmodified mutatee $t (opt: $opt) failed!"
            ./$t
            exit
        fi
    done
    echo "All unmodified tests passed."

    # for each analysis mode
    for m in $modes
    do
        # for each mutatee
        for t in $tests
        do

            # misc. test variables
            dir="results/$t"
            mkdir -p $dir
            tag="$m$opt"
            mutant="$dir/$tag"
            
            # analysis mode command-line options
            if [ "$m" = "cinst" ]
            then modeopt="--cinst"
            elif [ "$m" = "dcancel" ]
            then modeopt="--dcancel"
            elif [ "$m" = "dnan" ]
            then modeopt="--dnan"
            elif [ "$m" = "trange" ]
            then modeopt="--trange"
            elif [ "$m" = "svinp" ]
            then modeopt="--svinp single"
            elif [ "$m" = "svinp_double" ]
            then modeopt="--svinp double"
            else
                echo "Invalid test mode: $m"
                exit
            fi

            # build mutant
            fpinst -i $modeopt -o $mutant $t &>$dir/$tag.fpout

            if [ $? -ne 0 ]
            then
                echo "Cannot build $m mutant for $t!"
            else

                # run test
                $mutant >$dir/$tag.out

                # clean up any rewritten libraries
                if [ -e libm.so.6 ]
                then
                    mv libm.so.6 $dir/$tag.libm.so.6
                fi
                if [ -e libc.so.6 ]
                then
                    mv libc.so.6 $dir/$tag.libc.so.6
                fi

                # clean up log files
                mv fpinst.log $dir/$tag.fplog
                mv $t-*.log $dir/$tag.log

                # print results
                status=`grep "==.*:" $dir/$tag.out`
                echo "$m - ${status:3}"

            fi
        done
    done
done

END=$(date +%s)
DIFF=$(( $END - $START ))

echo ""
echo "Total Time: $DIFF sec"

