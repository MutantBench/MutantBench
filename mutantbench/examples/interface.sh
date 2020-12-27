
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
        echo "${fileName%.*.*}, 1"
    else
        fileName=$(basename $mutant)
        echo "${fileName%.*.*}, 0"
    fi;
}

function compareDir {
    path=$1
    for mutant in $path/mutants/*.a; do
        compare "$path/original.c.a" "$mutant"
    done
}

function MBDetectMutants {
    path=$1
    for program in $path/*; do
        compile "$program/original.c"
        compileDir "$program/mutants"
        compareDir "$program"
    done
}

"$@"