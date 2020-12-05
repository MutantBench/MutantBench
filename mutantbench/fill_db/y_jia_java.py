from mutantbench import db
from y_jia import TranslateYJia
import os
import re


class TranslateYJiaJava(TranslateYJia):
    operator_from_filename_regex = r'\w*_([A-Z]+)\.java'

    def get_program_locations(self):
        """Get the locations of the source program."""
        return [
            root + '/' + file_name
            for root, _, files in os.walk(self.directory)
            for file_name in files
            if re.match(r'\w+\.java', file_name)
        ]

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
                program = self.session.query(db.Program).filter(
                    db.Program.file_name.contains(program_name)
                ).first()

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append(root + '/' + name)

        return program_mutants


def main():
    jiac = TranslateYJiaJava(
        language=db.Languages.java,
        source='Y.Jia.java',
        directory='/home/polo/thesis/EquivMutantDataset/Mutation-Benchmark',
        out_dir='/home/polo/thesis/MutantBench'
    )
    jiac.gen_programs()
    jiac.gen_mutants()


if __name__ == '__main__':
    main()
