PATH=:/home/polo/cgum:$PATH

sqlite3 ../mutantbench/mutants.db "
select
    old_path ||
    ' /home/polo/thesis/MutantBench/mutantbench/generated/' || trim(program.file_name, '.java') || '/' || mutant.id || '.java'
from mutant join program
where
    program.id = mutant.program_id and
    program.language = 'java'
" | while read -r mutants; do
    echo $mutants
    dif= diff -u0 $mutants | tr --delete '\n\t'
    echo $dif
    if [ -n "$dif" ]; then
        /home/polo/thesis/MutantBench/verification/gumtree-2.1.2/bin/gumtree diff $mutants | grep -v 'Match '
        if [ $? -eq 0 ]; then
            exit 1
        fi
    fi
done

sqlite3 ../mutantbench/mutants.db "
select
    old_path ||
    ' /home/polo/thesis/MutantBench/mutantbench/generated/' || trim(program.file_name, '.c') || '/' || mutant.id || '.c'
from mutant join program
where
    program.id = mutant.program_id and
    program.language = 'c'
" | while read -r mutants; do
    echo $mutants
    dif= diff -u0 $mutants | tr --delete '\n\t'
    if [ -n "$dif" ]; then
        /home/polo/thesis/MutantBench/verification/gumtree-2.1.2/bin/gumtree diff $mutants | grep -v 'Match '
        if [ $? -eq 0 ]; then
            exit 1
        fi
    fi
done
