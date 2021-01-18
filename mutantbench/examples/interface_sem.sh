#!/bin/bash

function sizeCompare {
    a=`wc -c $1`
    b=`wc -c $2`
    python3 -c "print(max(1 - abs(${a% *} - ${b% *}) / 200, 0))"
}

function compile  {
    mutant=$1
    # Compile
    gcc -w -O3 -c "$mutant" -o "$mutant.o" &&
    # Output as assembly
    objdump -S "$mutant.o" > "$mutant.a" &&
    # Replace line numbers from assembly code
    sed -i "s/.[^:]*//" "$mutant.a" && sed -i "s/+[^>]*//" "$mutant.a";
}

function compileDir {
    path=$1
    for mutant in $path/*.c; do
        compile $mutant
    done
}

function compare {
    original=$1
    mutant=$2
    if diff "$original" "$mutant" &> /dev/null; then
        fileName=$(basename $mutant)
        echo "${fileName%.*.*}, 1" >> /tmp/mb_gcc_out_sem.txt
    else
        fileName=$(basename $mutant)
        echo ${original%.*} ${mutant%.*}
        a=`sizeCompare "$original" "$mutant"`
        echo "${fileName%.*.*}, $a" >> /tmp/mb_gcc_out_sem.txt
    fi
}

function compareDir {
    path=$1
    programName=`basename $path`
    for mutant in $path/mutants/*.a; do
        compare "$path/${programName}.c.a" "$mutant"
    done
    echo /tmp/mb_gcc_out_sem.txt
}

function MBSuggestMutants {
    rm -f /tmp/mb_gcc_out_sem.txt
    touch /tmp/mb_gcc_out_sem.txt
    path=$1
    for program in $path/*; do
        programName=`basename $program`
        compile "$program/${programName}.c"
        compileDir "$program/mutants"
        compareDir "$program"
    done
    echo /tmp/mb_gcc_out_sem.txt
}

"$@"