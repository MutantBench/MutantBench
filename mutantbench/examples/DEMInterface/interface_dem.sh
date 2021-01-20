
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
        echo "${fileName%.*.*}" >> /tmp/mb_gcc_out_dem.txt
    fi;
}

function compareDir {
    path=$1
    programName=`basename $path`
    for mutant in $path/mutants/*.a; do
        compare "$path/${programName}.c.a" "$mutant"
    done
    echo /tmp/mb_gcc_out_dem.txt
}

function MBDetectMutants {
    rm -f /tmp/mb_gcc_out_dem.txt
    touch /tmp/mb_gcc_out_dem.txt
    path=$1
    for program in $path/*; do
        programName=`basename $program`
        compile "$program/${programName}.c"
        compileDir "$program/mutants"
        compareDir "$program"
    done
    echo /tmp/mb_gcc_out_dem.txt
}

# Don't run: provid, make
# i.e. --programs bubble Calendar Calender Day Defroster Flex Hashmap Insert Mid Min Prime_number Replace Schedule Space Tcas Triangle

"$@"