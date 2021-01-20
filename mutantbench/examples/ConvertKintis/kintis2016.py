from mutantbench import db
from convert import ConverterDataset
import os
import re
import pandas as pd


class ConverterKintis(ConverterDataset):

    def get_program_filename(self, path):
        return path.split('/')[-2] + '.java'

    def get_program_locations(self):
        """Get the locations of the source program."""
        return [
            root + '/' + file_name
            for root, _, files in os.walk(self.directory)
            for file_name in files
            if re.match(r'\w+\.java', file_name) and 'original' in file_name
        ]

    def check_output(self, output, program_location, mutant_location):
        mins, plusses = 0, 0

        for line in output.split('\n'):
            if line.startswith('-'):
                mins += 1
            if line.startswith('+'):
                plusses += 1

        if mins != 1 or plusses != 1:
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
                if 'original' in name:
                    continue
                location = root + '/' + name

                program_name = root.split('/')[-1] + '.java'
                program = self.get_program_from_name(program_name)

                equivalent = 'EQUIV' in name

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append((location, equivalent))

        return program_mutants


def main():
    tce_plus = ConverterKintis(
        language='java',
        source='kintis2016analysing',
        directory='/path/to/kintis/dataset/',
        out_dir='/path/to/program/storage/'
    )
    tce_plus.gen_programs()
    tce_plus.gen_mutants()


if __name__ == '__main__':
    main()
