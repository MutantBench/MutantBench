
TOOL_HOME="/home/polo/thesis/major_test"

function generate {
    cd $TOOL_HOME/scripts
    $TOOL_HOME/major/bin/ant -f $TOOL_HOME/build.xml -Dmutation="=mml/all.mml.bin" clean compile.major || exit 1
    cd -
}


function MBAvoidEquivalentMutants {
    path=$1
    for program_dir in $path/*; do
        rm -r $TOOL_HOME/src/*
        program=`ls $program_dir | grep ".java"`
        cp $program_dir/$program $TOOL_HOME/src
        generate || exit 1
        mkdir -p $program_dir/mutants
        for mutant_path in $TOOL_HOME/mutants/*; do
            mutant_name=`basename $mutant_path`
            mp=`ls -d $mutant_path/* | head -n 1`
            mutant="$mp/$program"
            cp $mutant $program_dir/mutants/$mutant_name.java
        done;

        # ls $TOOL_HOME/mutant
    done

    echo $path
}

"$@" || exit 1