from mutantbench import db
from y_jia import TranslateYJia
import os


class TranslateYJiaC(TranslateYJia):
    operator_from_filename_regex = r'[0-9\.]+-([A-Za-z]+)'

    def get_program_locations(self):
        """Get the locations of the source program."""
        return [
            self.directory + '/original/' + file_name
            for file_name in os.listdir(self.directory + '/original')
            if file_name.endswith('.c')
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
            if 'original' in root:
                continue

            for name in files:
                if not name.endswith('.c'):
                    continue

                program_name = name.split(' ')[0].split('-')[-1]
                program = self.get_program_from_name(program_name)

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append((root + '/' + name, True))

        return program_mutants


def main():
    jia_c = TranslateYJiaC(
        language=db.Languages.c,
        source='Y.Jia.c',
        directory='/home/polo/thesis/EquivMutantDataset/Y.Jia/www0.cs.ucl.ac.uk/staff/Y.Jia/projects/equivalent_mutants/AllEQ/',
        out_dir='/home/polo/thesis/MutantBench'
    )
    jia_c.gen_programs()
    jia_c.gen_mutants()


if __name__ == '__main__':
    main()
