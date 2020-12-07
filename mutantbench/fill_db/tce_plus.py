from mutantbench import db
from translate import TranslateOperatorInFilename, OperatorNotFound
import os
import re


class TranslateTCEPlus(TranslateOperatorInFilename):
    operator_from_filename_regex = r'\/([A-Z]*)_\d*\/'

    @classmethod
    def map_operators(cls, operator, diff=None):
        # TCE PLUS does not include bit shits, or the a -> +a operator
        operator_map = {
            'AODS': 'AODS',
            'AODU': 'AODU',
            'AOIS': 'AOIS',
            'AOIU': 'AOIU',
            'AORB': 'AORB',
            'AORS': 'AORS',
            'ASRS': 'ASRS',
            'CDL': 'CDL',
            'COD': 'COD',
            'COI': 'COI',
            'COR': 'COR',
            'LOI': 'LOI',
            'ODL': cls.get_odl,
            'ROR': 'ROR',
            'SDL': 'STMTD',
            'VDL': cls.get_vdl,
        }
        if callable(operator_map[operator]):
            return operator_map[operator](diff)
        else:
            return operator_map[operator]

    @classmethod
    def get_odl(cls, diff):
        if any(i in diff for i in ['++', '--']):
            return 'AODS'
        if any(i in diff for i in ['=', '>', '<', '!']):
            return 'VROD'
        return 'VAOD'

    @classmethod
    def get_vdl(cls, diff):
        if any(i in diff for i in ['++', '--']):
            return 'AODS'
        if any(i in diff for i in ['=', '>', '<', '!']):
            return 'VROD'
        return 'VAOD'

    def get_operators_from_mutant_location(self, mutant_location, diff=None):
        """Returns a list of operators that the mutant used."""
        print(mutant_location)
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

                program_name = name.split('.')[0]
                program = self.get_program_from_name(program_name)

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append(root + '/' + name)

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
