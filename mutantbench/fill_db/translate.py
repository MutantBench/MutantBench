import os
from sqlalchemy.orm import sessionmaker
from sqlalchemy import create_engine
import re
from shutil import copyfile
import subprocess
from mutantbench import db
import difflib


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
        self.out_dir = f'{out_dir}/programs/{self.source}'

        # Create DB session
        engine = create_engine('sqlite:///mutants.db', echo=False)
        Session = sessionmaker()
        Session.configure(bind=engine)
        self.session = Session()

        # Create program directory
        if not os.path.exists(self.out_dir):
            os.makedirs(self.out_dir)

        # Initialize other variables
        self.programs = None
        self.mutants = None

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
            path = f'{self.out_dir}/{file_name}'

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
        self.session.commit()
        self.programs = programs_to_add + programs_not_to_add
        return self.programs

    def gen_diff(self, program_location, mutant_location):
        """Generate the difference between two files."""
        # TODO: clear whitespace and remove //mutated statement
        diff = subprocess.Popen(
            [f'/usr/bin/diff -u0 "{program_location}" "{mutant_location}"'],
            stdout=subprocess.PIPE,
            shell=True
        )
        tail = subprocess.Popen(
            'tail -n +3'.split(),
            stdin=diff.stdout,
            stdout=subprocess.PIPE,
        )
        output, error = tail.communicate()
        if error:
            raise OSError(error)

        output = output.decode("utf-8")
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

        mutants_to_add = []
        mutants_not_to_add = []
        for program, mutant_locations in self.get_mutant_locations().items():
            for (mutant_location, equivalency) in mutant_locations:
                # print(mutant_location)
                diff = self.gen_diff(program.path, mutant_location)
                # print(diff)

                char_diff = self.get_char_diff(diff)
                # print(char_diff)

                # Skip programs that do not actually contain any diff
                if not char_diff:
                    print("SKIPPED ONE")
                    print(mutant_location)
                    continue

                operators = self.get_operators_from_mutant_location(
                    mutant_location,
                    diff=char_diff,
                )

                existing_mutants = self.session.query(db.Mutant).filter(
                    db.Mutant.diff == diff)

                if enforce_gen or existing_mutants.count() == 0:
                    mutants_to_add.append(db.Mutant(
                        diff=diff,
                        operators=operators,
                        program=program,
                        equivalent=equivalency,
                    ))

        self.session.add_all(mutants_to_add)
        self.session.commit()
        self.mutants = mutants_to_add + mutants_not_to_add
        return self.mutants

    def get_program_from_name(self, program_name):
        return self.session.query(db.Program).filter(
            db.Program.file_name.contains(program_name),
            db.Program.language == self.language,
            db.Program.source == self.source,
        ).first()

    def get_char_diff(self, diff):
        diff_lines = diff.split('\n')
        old = '\n'.join([
            line[1:]  # Remove diff syntax
            for line in diff_lines
            if line.startswith('-')  # Get all the old lines
        ])
        new = '\n'.join([
            line[1:]  # Remove diff syntax
            for line in diff_lines
            if line.startswith('+')  # Get all the new lines
        ])
        old = old.replace('//mutated statement', '')
        new = new.replace('//mutated statement', '')
        old = ' '.join(old.split())
        new = ' '.join(new.split())
        result = ""
        codes = difflib.SequenceMatcher(a=old, b=new).get_opcodes()
        for code in codes:
            o = old[code[1]:code[2]].strip()
            n = new[code[3]:code[4]].strip()
            if not o and not n or o == n:
                continue
            if code[0] == "delete":
                result += f'» {o} «'
            elif code[0] == "insert":
                result += f'» {n} «'
            elif code[0] == "replace":
                result += f'» {o} ↦ {n} «'
        return result

    def get_operators_from_mutant_location(self, mutant_location, diff=None):
        """Returns a list of operators that the mutant used."""
        raise NotImplementedError

    def get_program_locations(self):
        """Get the locations of the source program."""
        raise NotImplementedError

    def get_mutant_locations(self):
        """Return a dictionary as: {
            [program]: [list of sets as follows:
                (
                    mutant locations belonging to the program,
                    wether the mutant is equivalent or not,
                )
            ]
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
        file_name = get_filename(mutant_location)

        operator_re = re.findall(self.operator_from_filename_regex, file_name)
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
        raise NotImplementedError

    def get_mutant_locations(self):
        raise NotImplementedError
