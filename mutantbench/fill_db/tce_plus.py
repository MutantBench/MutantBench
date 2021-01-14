from mutantbench import db
from translate import TranslateDataset
import os
import re
import pandas as pd


class TranslateTCEPlus(TranslateDataset):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.equivalent_mutants = pd.read_csv(f'{self.directory}/../EMs_list.csv')

    def get_program_locations(self):
        """Get the locations of the source program."""
        return [
            root + '/' + file_name
            for root, _, files in os.walk(self.directory)
            for file_name in files
            if re.match(r'\w+\.java', file_name) and 'original' in root
        ]

    def check_output(self, output, program_location, mutant_location):
        mins, plusses = 0, 0

        for line in output.split('\n'):
            if line.startswith('-'):
                mins += 1
            if line.startswith('+'):
                plusses += 1

        if 'SDL_' in mutant_location and plusses == 0:
            return True
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
                if 'original' in root:
                    continue
                location = root + '/' + name

                program_name = name.split('.')[0]
                program = self.get_program_from_name(program_name)

                equivalent = any(
                    equiv_mutant in location
                    for equiv_mutant in self.equivalent_mutants[program_name]
                    .dropna().tolist()
                )

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append((location, equivalent))

        return program_mutants


def main():
    tce_plus = TranslateTCEPlus(
        language='java',
        source='houshmand2017tce',
        directory='/home/polo/thesis/EquivMutantDataset/TCE_PLUS/Mutants_java',
        out_dir='/home/polo/thesis/MutantBench'
    )
    tce_plus.gen_programs()
    tce_plus.gen_mutants()


if __name__ == '__main__':
    main()
