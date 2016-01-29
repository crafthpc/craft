#!/bin/bash
#
# Runs some mixed- and reduced-precision searches and check their results for
# correctness. Meant to be run as a part of the standard test suite.
#

NPROCS=4            # number of worker threads to use for searches
RUN_MIXED=false     # run standard mixed-precision search
RUN_RPREC=true      # run standard reduced-precision search

RUN_SEARCH=true     # set to "false" to only do the checks; use for debugging this script
#RUN_SEARCH=false    # set to "false" to only do the checks; use for debugging this script

# directories
TESTDIR=`pwd`
MUTDIR=$DYNINST_ROOT/craft/demo/sum2pi_x
MUTATEE=$MUTDIR/sum2pi_x


# build the sum2pi_x mutatee
echo " "
echo "=== SUM2PI_X ==="
echo " "
echo "Building sum2pi_x:"
cd $MUTDIR && make clean && make
if [ $? -ne 0 ]; then
    echo "ERROR: Cannot build sum2pi_x!"
    exit
fi
echo "Build complete."

mkdir -p $TESTDIR/auto
cd $TESTDIR/auto


if [ "$RUN_MIXED" == true ]; then

    # run mixed-precision search
    echo " "
    echo "=== Standard mixed-precision search ==="
    echo " "
    if [ "$RUN_SEARCH" == true ]; then
        rm -rf $TESTDIR/auto/*
        cp $MUTDIR/craft_driver .
        echo "Running search..."
        craft search -j $NPROCS $MUTATEE &>search-mixed.txt
    fi
    errors=0

    if [ -e craft_final.cfg ]; then

        # verify that there are at least five single-precision instructions
        echo -n "Mixed - At least five single-precision instructions:     "
        s_count=`flattencfg craft_final.cfg | grep "^\^s" | wc -l`
        if [ "$s_count" -lt 5 ]; then
            errors=$((errors+1))
            echo "fail"
        else
            echo "pass"
        fi

        # verify that there is only one double-precision instruction
        echo -n "Mixed - Exactly one double-precision instruction:        "
        d_count=`flattencfg craft_final.cfg | grep "^\^d" | wc -l`
        if [ "$d_count" -ne 1 ]; then
            errors=$((errors+1))
            echo "fail"
        else
            echo "pass"
        fi

        # verify that the double-precision instruction is the add from line 65
        echo -n "Mixed - Double precision for accumulator:                "
        d_correct=`flattencfg craft_final.cfg | grep "^\^d" | grep "sum2pi_x.c:65"`
        if [[ -z $d_correct ]]; then
            errors=$((errors+1))
            echo "fail"
        else
            echo "pass"
        fi

    else
        echo "ERROR: No output configuration to check"
        errors=$((errors+1))
    fi



    # final check
    if [ "$errors" -gt 0 ]; then
        echo "OVERALL:                                                 fail"
    else
        echo "OVERALL:                                                 pass"
    fi
    echo " "

fi


if [ "$RUN_RPREC" == true ]; then

    # run reduced-precision search
    echo " "
    echo "=== Standard reduced-precision search ==="
    echo " "
    if [ "$RUN_SEARCH" == true ]; then
        rm -rf $TESTDIR/auto/*
        cp $MUTDIR/craft_driver .
        echo "Running search..."
        craft search -r -j $NPROCS $MUTATEE &>search-rprec.txt
    fi
    errors=0

    if [ -e craft_final.cfg ]; then

        # verify that the add in line 56 can be done with zero precision
        echo -n "Reduced - Non-zero precision for bit-shift add:          "
        line=`grep "sum2pi_x.c:56" craft_final.cfg`
        regex="INSN #([0-9]*)"
        if [[ "$line" =~ $regex ]]; then
            id=${BASH_REMATCH[1]}
            line=`grep "INSN_$id" craft_final.cfg`
            regex="precision=([0-9]*)"
            if [[ "$line" =~ $regex ]]; then
                prec=${BASH_REMATCH[1]}
                if [ "$prec" -gt 0 ]; then
                    errors=$((errors+1))
                    echo "fail"
                else
                    echo "pass"
                fi
            else
                errors=$((errors+1))
                echo "fail"
            fi
        else
            errors=$((errors+1))
            echo "fail"
        fi

        # verify that the add in line 65 requires more than 23 bits of precision
        echo -n "Reduced - More than single precision for accumulator:    "
        line=`grep "sum2pi_x.c:65" craft_final.cfg`
        regex="INSN #([0-9]*)"
        if [[ "$line" =~ $regex ]]; then
            id=${BASH_REMATCH[1]}
            line=`grep "INSN_$id" craft_final.cfg`
            regex="precision=([0-9]*)"
            if [[ "$line" =~ $regex ]]; then
                prec=${BASH_REMATCH[1]}
                if [ "$prec" -lt 24 ]; then
                    errors=$((errors+1))
                    echo "fail"
                else
                    echo "pass"
                fi
            else
                errors=$((errors+1))
                echo "fail"
            fi
        else
            errors=$((errors+1))
            echo "fail"
        fi

    else
        echo "ERROR: No output configuration to check"
        errors=$((errors+1))
    fi

    # final check
    if [ "$errors" -gt 0 ]; then
        echo "OVERALL:                                                 fail"
    else
        echo "OVERALL:                                                 pass"
    fi
    echo " "

fi

