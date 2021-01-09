from mutantbench import db
from y_jia_c import TranslateYJiaC
import os
import re


class TranslateYJiaJava(TranslateYJiaC):
    def get_program_locations(self):
        """Get the locations of the source program."""
        return [
            root + '/' + file_name
            for root, _, files in os.walk(self.directory)
            for file_name in files
            if re.match(r'\w+\.java', file_name)
        ]

    def check_output(self, output, program_location, mutant_location):
        mins, plusses, mutated = 0, 0, 0

        for line in output.split('\n'):
            if line.startswith('-'):
                mins += 1
            if line.startswith('+'):
                plusses += 1
                if self.line_has_mutation_comment(line):
                    mutated += 1
        if mins != plusses:
            return False

        return True

    def get_mutant_locations(self):
        """ Return a dictionary containing a list of all mutants belonging to
        each program set in [self.programs]

        Return a dictionary as: {
            [program]: [list of mutant locations belonging to the program]
        }.
        """
        program_mutants = {}

        for root, _, files in os.walk(self.directory, topdown=False):
            for name in files:
                if not name.endswith('.java'):
                    continue

                if re.match(r'\w+\.java', name):
                    continue

                program_name = name.split('_')[0]
                program = self.get_program_from_name(program_name)

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append((root + '/' + name, True))

        return program_mutants


def main():
    jia_java = TranslateYJiaJava(
        language=db.Languages.java,
        source='Y.Jia.java',
        directory='/home/polo/thesis/EquivMutantDataset/Mutation-Benchmark',
        out_dir='/home/polo/thesis/MutantBench'
    )
    jia_java.gen_programs()
    jia_java.gen_mutants()


if __name__ == '__main__':
    main()
