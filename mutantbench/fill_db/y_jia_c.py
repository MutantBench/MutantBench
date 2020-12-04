from shutil import copyfile
import subprocess
import re
import sys
import os
from sqlalchemy.orm import sessionmaker
from sqlalchemy import create_engine
from mutantbench import db


def get_filename(path):
    return os.path.basename(path)


class OperatorNotFound(Exception):
    def __init__(self, operator=None):
        self.message = 'Operator not found'
        if operator:
            self.message = f'{self.message}: {operator}'


class TranslateDataset(object):
    def __init__(self, language, directory, source, out_dir):
        # Initialize passed through variables
        self.language = language
        self.source = source
        self.directory = directory
        self.out_dir = out_dir

        # Create DB session
        engine = create_engine('sqlite:///mutants.db', echo=False)
        Session = sessionmaker()
        Session.configure(bind=engine)
        self.session = Session()

        # Initialize other variables
        self.programs = None

    def gen_programs(self, enforce_gen=False):
        """Generate the programs from this dataset and add them to the database.

        Does not generate programs that already exists, although it does return
        them. Generation of duplicates can be enforced by setting [enforce_gen]
        to True.

        NOTE: requires the own implementation of `get_program_locations`
        """
        programs_to_add = []
        programs_not_to_add = []
        for program_location in self.get_program_locations():
            file_name = get_filename(program_location)
            path = f'{self.out_dir}/programs/{self.source}/{file_name}'

            if enforce_gen or not os.path.isfile(path):
                copyfile(program_location, path)

            existing_programs = self.session.query(db.Program).filter(
                db.Program.file_name == file_name)
            if enforce_gen or existing_programs.count() == 0:
                programs_to_add.append(db.Program(
                    language=self.language,
                    file_name=file_name,
                    path=path,
                    source=self.source,
                ))
            else:
                programs_not_to_add.append(existing_programs.first())
        self.session.add_all(programs_to_add)
        self.programs = programs_to_add + programs_not_to_add
        return self.programs

    def gen_diff(self, program_location, mutant_location):
        """Generate the difference between two files."""
        # TODO: clear whitespace and remove //mutated statement
        print(' '.join(['diff', '-u0', f'"{program_location}"', f'"{mutant_location}"']))
        diff = subprocess.Popen(
            ['diff', '-u0', f'"{program_location}"', f'"{mutant_location}"'],
            stdout=subprocess.PIPE
        )
        tail = subprocess.Popen(
            'tail -n +3'.split(),
            stdin=diff.stdout,
            stdout=subprocess.PIPE,
        )
        output, error = tail.communicate()
        if error:
            raise OSError(error)
        return output

    def gen_mutants(self, enforce_gen=False):
        """Generate mutants and add them to the database.

        Does not generate mutants that already exists, although it does return
        them. Generation of duplicates can be enforced by setting [enforce_gen]
        to True.

        NOTE: requires the own implementation of `get_mutant_locations` and
        `get_operators_from_mutant_location`"""
        if not self.programs:
            self.gen_programs()

        mutants = []
        for program, mutant_locations in self.get_mutant_locations().items():
            for mutant_location in mutant_locations:
                print(mutant_location)
                diff = self.gen_diff(program.path, mutant_location)

                operators = self.get_operators_from_mutant_location(
                    mutant_location,
                    diff=diff,
                )

                mutants.append(db.Mutant(
                    diff=diff,
                    operators=operators,
                    program=program,
                ))

    def get_operators_from_mutant_location(self, mutant_location, diff=None):
        """Returns a list of operators that the mutant used."""
        raise NotImplementedError

    def get_program_locations(self):
        """Get the locations of the source program."""
        raise NotImplementedError

    def get_mutant_locations(self):
        """Return a dictionary as: {
            [program]: [list of mutant locations belonging to the program]
        }."""
        raise NotImplementedError


class TranslateOperatorInFilename(TranslateDataset):
    @property
    def operator_from_filename_regex(self):
        raise NotImplementedError

    @classmethod
    def map_operators(cls, operator, diff=None):
        raise NotImplementedError

    def get_operators_from_mutant_location(self, mutant_location, diff=None):
        """Returns a list of operators that the mutant used."""
        file_name = get_filename(mutant_location)

        operator_re = re.findall(self.operator_from_filename_regex, file_name)
        print(file_name)
        if not operator_re:
            raise OperatorNotFound(operator_re)

        operator_name = self.map_operators(operator_re[0])
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
        raise NotImplementedError

    def get_mutant_locations(self):
        """Return a dictionary as: {
            [program]: [list of mutant locations belonging to the program]
        }."""
        raise NotImplementedError


class TranslateYJia(TranslateOperatorInFilename):
    @classmethod
    def map_operators(cls, operator, diff=None):
        operator_map = {
            'UOI': 'AOIU',
            'ROR': 'ROR',
            'ABS': 'ABSI',
            'AOR': cls.get_aor,
        }
        if callable(operator_map[operator]):
            return operator_map[operator](diff)
        else:
            return operator_map[operator]

    @classmethod
    def get_aor(cls, diff):
        if any(i in diff for i in ['++', '--']):
            return 'AORS'
        if 'abs' in diff:
            return 'ABSI'
        if any(i in diff for i in ['=', '>', '<', '!']):
            return 'ROR'
        return 'AORB'


class TranslateYJiaC(TranslateYJia):
    operator_from_filename_regex = r'[0-9\.]+-([A-Z]+)'

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
                program = self.session.query(db.Program).filter(
                    db.Program.file_name.contains(program_name)
                ).first()

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append(root + '/' + name)

        return program_mutants


def main(directory):
    jiac = TranslateYJiaC(
        language=db.Languages.c,
        source='Y.Jia.c',
        directory=directory,
        out_dir='/home/polo/thesis/MutantBench'
    )
    jiac.gen_programs()
    jiac.gen_mutants()

if __name__ == '__main__':
    main(sys.argv[1])
