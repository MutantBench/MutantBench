from mutantbench import db
from translate import TranslateOperatorInFilename, OperatorNotFound
import os
import re
import pandas as pd


class TranslateTCEPlus(TranslateOperatorInFilename):
    operator_from_filename_regex = r'\/([A-Z]*)_\d*\/'

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.equivalent_mutants = pd.read_csv(f'{self.directory}/EMs_list.csv')

    @classmethod
    def map_operators(cls, operator, diff=None):
        # TCE PLUS does not include bit shits, or the a -> +a operator
        operator_map = {
            'ODL': cls.get_odl,
            'SDL': 'STMTD',
            'VDL': cls.get_vdl,
        }
        if operator not in operator_map:
            return operator
        if callable(operator_map[operator]):
            return operator_map[operator](diff)
        else:
            return operator_map[operator]

    @classmethod
    def get_odl(cls, diff):
        if any(i in diff for i in ['++', '--']):
            return 'AODS'
        if any(i in diff for i in ['&&', '||', '^']):
            return 'VCOD'
        if any(i in diff for i in '=<>!'):
            return 'VROD'
        if any(i in diff for i in '+-*/%'):
            return 'VAOD'
        raise OperatorNotFound(diff + '\n get_odl')

    @classmethod
    def get_vdl(cls, diff):
        if any(i in diff for i in ['++', '--']):
            return 'AODS'
        if any(i in diff for i in ['&&', '||', '^']):
            return 'VCOD'
        if any(i in diff for i in '=<>!'):
            return 'VROD'
        if any(i in diff for i in '+-*/%'):
            return 'VAOD'
        raise OperatorNotFound(diff + '\n get_vdl')

    def get_operators_from_mutant_location(self, mutant_location, diff=None):
        """Returns a list of operators that the mutant used."""
        operator_re = re.findall(
            self.operator_from_filename_regex,
            mutant_location,
        )
        if not operator_re:
            raise OperatorNotFound(operator_re)

        operator_name = self.map_operators(operator_re[0], diff=diff)
        if not operator_name:
            raise OperatorNotFound(operator_name)

        matching_operator = self.session.query(db.Operator).filter(
            db.Operator.name == operator_name
        ).first()

        if not matching_operator:
            raise OperatorNotFound(operator_name)

        return [matching_operator]

    def get_program_locations(self):
        """Get the locations of the source program."""
        return [
            root + '/' + file_name
            for root, _, files in os.walk(self.directory)
            for file_name in files
            if re.match(r'\w+\.java', file_name) and 'original' in root
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
    jiac = TranslateTCEPlus(
        language=db.Languages.java,
        source='TCE.plus.java',
        directory='/home/polo/thesis/EquivMutantDataset/TCE_PLUS/Mutants_java',
        out_dir='/home/polo/thesis/MutantBench'
    )
    jiac.gen_programs()
    jiac.gen_mutants()


if __name__ == '__main__':
    main()
