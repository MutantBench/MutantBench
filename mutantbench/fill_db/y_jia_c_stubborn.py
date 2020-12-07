import re
from mutantbench import db
from y_jia import TranslateYJia
import os


class TranslateYJiaC(TranslateYJia):
    operator_from_filename_regex = r'[A-Za-z]+[0-9]+([A-Za-z]+).c'

    def get_program_locations(self):
        return []

    def get_mutant_locations(self):
        program_mutants = {}

        for root, _, files in os.walk(self.directory, topdown=False):
            if 'original' in root:
                continue

            for name in files:
                if not name.endswith('.c'):
                    continue
                if name.startswith('.'):
                    continue

                program_name_regex = r'([A-Za-z]+)[0-9]+[A-Za-z]+.c'
                program_name = re.findall(
                    program_name_regex,
                    name,
                )[0]
                program = self.get_program_from_name(program_name)

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append((root + '/' + name, False))

        return program_mutants


def main():
    jiac = TranslateYJiaC(
        language=db.Languages.c,
        source='Y.Jia.c',
        directory='/home/polo/thesis/EquivMutantDataset/stubborn_mutants_yjia/',
        out_dir='/home/polo/thesis/MutantBench'
    )
    jiac.gen_programs()
    jiac.gen_mutants()


if __name__ == '__main__':
    main()
