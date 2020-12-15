import os
import re
from shutil import copyfile
import subprocess
from mutantbench import db, session
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
        self.session = session

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
        diff = subprocess.Popen(
            [f'/usr/bin/diff -u0 --ignore-all-space --ignore-blank-lines "{program_location}" "{mutant_location}"'],
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

    def gen_ast_diff(self, program_location, mutant_location):
        """Generate the difference between two files."""
        return [
            line
            for line in subprocess.run(
                ['/home/polo/thesis/MutantBench/verification/gumtree-2.1.2/bin/gumtree', 'diff', program_location, mutant_location],
                capture_output=True
            ).stdout.decode('utf-8').split('\n')
            if not line.startswith('Match ')
        ]

    def gen_mutants(self, enforce_gen=False):
        """Generate mutants and add them to the database.

        Does not generate mutants that already exists, although it does return
        them. Generation of duplicates can be enforced by setting [enforce_gen]
        to True.

        NOTE: requires the own implementation of `get_mutant_locations`"""
        if not self.programs:
            self.gen_programs()

        mutants_to_add = []
        mutants_not_to_add = []

        for program, mutant_locations in self.get_mutant_locations().items():
            for (mutant_location, equivalency) in mutant_locations:
                print(program.path, mutant_location, equivalency)
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
                    program.path,
                    mutant_location,
                )

                existing_mutants = self.session.query(db.Mutant).filter(
                    db.Mutant.diff == diff)

                if enforce_gen or existing_mutants.count() == 0:
                    mutants_to_add.append(db.Mutant(
                        diff=diff,
                        operators=operators,
                        program=program,
                        equivalent=equivalency,
                        old_path=mutant_location,
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

    def get_operator_from_name(self, operator_name):
        operator = self.session.query(db.Operator).filter(
            db.Operator.name == operator_name
        ).first()
        if not operator:
            print(operator_name)
            raise NotImplementedError
        return operator

    def get_operators_from_mutant_location(self, program_location, mutant_location):
        """Returns a list of operators that the mutant used."""
        with open(f'errors_{self.source}.txt', 'a') as errors:
            operators = list()
            mapping = [
                (r'Insert (MethodInvocation|FunCall)\(\d+\)', 'ABSI1'),
                (r'Insert (SimpleName|GenericString): abs\(\d+\)', 'ABSI2'),
                (r'Update (InfixExpression|GenericString): [+\-*/%]\(\d+\) to [+\-*/%]', 'AORB'),
                # (r'Insert (InfixExpression|GenericString): [+\-*/%]\(\d+\) into (InfixExpression|GenericString): [+\-*/%]\(\d+\)', 'AOIB'),
                (r'Update ((Pre|Post)fixExpression|GenericString): (\+\+|--)\(\d+\) to (\+\+|--)', 'AORS'),
                (r'Insert (PrefixExpression|GenericString): [+\-]\(\d+\)', 'AOIU'),
                (r'Insert ((Pre|Post)fixExpression|GenericString): (\+\+|--)\(\d+\)', 'AOIS'),

                (r'Delete (InfixExpression|GenericString): [+\-*/%]\(\d+\)', 'AODB'),

                (r'Delete PrefixExpression: [+\-]\(\d+\)', 'AODU')
                if self.language == db.Languages.java else
                (r'Delete Unary\(\d+\)', 'AODU'),

                (r'Delete ((Pre|Post)fixExpression|GenericString): (\+\+|--)\(\d+\)', 'AODS'),

                (r'Update (InfixExpression|GenericString): (>=|<=|>|<|!=|==)\(\d+\) to (>=|<=|>|<|!=|==)', 'ROR'),
                (r'Update GenericString: (>=|<=|>|<|!=|==)\(\d+\) to (>=|<=|>|<|!=|==)', 'ROR'),
                (r'Insert BooleanLiteral: (true|false)\(\d+\)', 'ROR+'),
                (r'Delete (InfixExpression|GenericString): (>=|<=|>|<|!=|==)\(\d+\)', 'ROD'),
                (r'Delete GenericString: (>=|<=|>|<|!=|==)\(\d+\)', 'ROD'),

                (r'Update (InfixExpression|GenericString): (\|\||&&|\^)\(\d+\) to (\|\||&&|\^)', 'COR'),
                (r'Delete (InfixExpression|GenericString): (\|\||&&|\^)\(\d+\)', 'COD'),
                (r'Delete (PrefixExpression|GenericString): \!\(\d+\)', 'COD'),
                (r'Insert (PrefixExpression|GenericString): \!\(\d+\)', 'COI'),

                (r'Update (InfixExpression|GenericString): (>>|<<|>>>)\(\d+\) to (>>|<<|>>>)', 'SOR'),

                (r'Update (InfixExpression|GenericString): (&|\||\^)\(\d+\) to (&|\||\^)', 'LOR'),
                (r'Insert (PrefixExpression|GenericString): ~\(\d+\)', 'LOI'),
                (r'Delete (PrefixExpression|GenericString): ~\(\d+\)', 'LOD'),

                (r'Update Assignment: (\+=|\-=|\*=|%=|\/=|&=|\^=|>>=|<<=|>>>=)\(\d+\) to (\+=|\-=|\*=|%=|\/=|&=|\^=|>>=|<<=|>>>=)', 'ASRS'),

                (r'Update Assignment: (\+=|\-=|\*=|%=|\/=|&=|\^=|>>=|<<=|>>>=)\(\d+\) to =', 'VDL'),

                (r'Delete ((Simple|Qualified)Name|GenericString): \w[\w.\d]+\(\d+\)', 'VDL'),
                (r'Insert ((Simple|Qualified)Name|GenericString): \w[\w.\d]+\(\d+\)', '!VDL'),

                (r'Delete (NumberLiteral|Constant): [\d.]+\(\d+\)', 'CDL'),
                (r'Insert (NumberLiteral|Constant): [\d.]+\(\d+\)', '!CDL'),

                (r'Delete \w+Statement\(\d+\)', 'SDL'),
            ]
            ast_diff = self.gen_ast_diff(program_location, mutant_location)

            operator_counts = {
                operator_name: 0
                for _, operator_name in mapping
            }
            for line in ast_diff:
                for regex, operator_name in mapping:
                    if re.match(regex, line):
                        operator_counts[operator_name] += 1
                        break
            operator_counts['CDL'] -= operator_counts.pop('!CDL')
            operator_counts['VDL'] -= operator_counts.pop('!VDL')
            # operator_counts['VDL'] -= 2 * operator_counts['ROR+']
            # if operator_counts['VDL'] < 0:
            #     operator_counts['CDL'] += operator_counts['VDL']
            #     operator_counts['VDL'] = 0
            operator_counts['ROR'] += operator_counts.pop('ROR+')
            operator_counts['ROD'] -= operator_counts['ROR']
            operator_counts['AORS'] += min(operator_counts['AOIS'], operator_counts['AODS'])
            operator_counts['AODS'] -= operator_counts['AORS']
            operator_counts['AOIS'] -= operator_counts['AORS']

            operator_counts['ABSI'] = min(operator_counts.pop('ABSI1'), operator_counts.pop('ABSI2'))
            operator_counts['AODB'] -= operator_counts['AODU']
            operator_counts['AORB'] += operator_counts.pop('AODB')

            operator_counts['AOIU'] -= operator_counts['ABSI']


            if operator_counts['SDL'] > 0:
                operators = [self.get_operator_from_name('SDL')]
            else:
                for operator_name, count in operator_counts.items():
                    operators += [self.get_operator_from_name(operator_name)] * count

            # print(ast_diff, '\n', operator_counts, [o.name for o in operators])

            if len(operators) >= 2:
                if any(
                    {o.name for o in operators} == a
                    for a in [
                        {'ROR', 'VDL', 'CDL'},
                        {'ROR', 'VDL'},
                        {'ROR', 'CDL'},
                        {'ROD', 'VDL', 'CDL'},
                        {'ROD', 'VDL'},
                        {'ROD', 'CDL'},
                        {'AODS', 'VDL'},
                        {'COD', 'ROD', 'VDL', 'CDL'},
                        {'COD', 'ROD', 'VDL'},
                        {'COD', 'ROD', 'CDL'},
                        {'ROR', 'COD', 'ROD', 'VDL', 'CDL'},
                        {'ROR', 'COD', 'ROD', 'VDL'},
                        {'ROR', 'COD', 'ROD', 'CDL'},
                        {'ROR', 'COD', 'VDL', 'CDL'},
                        {'ROR', 'COD', 'VDL'},
                        {'ROR', 'COD', 'CDL'},
                    ]
                ):
                    pass
                if 'Tcas' in program_location and {o.name for o in operators} == {'ROR'}:
                    pass
                elif 'Flex' in program_location and {o.name for o in operators} == {'ABSI'}:
                    pass
                elif 'Mid' in program_location and {o.name for o in operators} == {'AOIS'}:
                    pass
                elif ('Hashmap' in program_location or 'Replace' in program_location) and len(ast_diff) > 10:
                    pass
                else:
                    errors.write(program_location + ' ' + mutant_location + '\n')
                    errors.write(','.join([o.name for o in operators]) + '\n')
                    errors.write('\n'.join(ast_diff[:20]) + '\n')
                    if len(ast_diff) > 20:
                        errors.write(f'AND {len(ast_diff)} MORE LINES OF AST DIFF')
                    errors.write(self.gen_diff(program_location, mutant_location) + '\n')
            elif len(operators) == 1:
                if operators[0].name in mutant_location:
                    pass
                elif any(
                    operators[0].name == a and b in mutant_location
                    for a, b in [
                        ('AOIS', 'UOI'),
                        ('ROR', 'ROR'),
                        ('ABSI', 'ABS'),
                        ('AORS', 'AOR'),
                        ('ABSI', 'AOR'),
                        ('ROR', 'AOR'),
                        ('AORB', 'AOR'),
                        ('ROR', 'ror'),
                        ('COR', 'LCR'),
                        ('VDL', 'ODL'),
                        ('CDL', 'ODL'),
                        ('COD', 'ODL'),
                        ('AODS', 'ODL'),
                        ('VDL', 'SDL'),
                        ('AODU', 'ODL')
                    ]
                ):
                    pass
                else:
                    errors.write(program_location + ' ' + mutant_location + '\n')
                    errors.write(','.join([o.name for o in operators]) + '\n')
                    errors.write('\n'.join(ast_diff[:20]) + '\n')
                    if len(ast_diff) > 20:
                        errors.write(f'AND {len(ast_diff)} MORE LINES OF AST DIFF')
                    errors.write(self.gen_diff(program_location, mutant_location) + '\n')
            else:
                errors.write(program_location + ' ' + mutant_location + '\n')
                errors.write(','.join([o.name for o in operators]) + '\n')
                errors.write('\n'.join(ast_diff[:20]) + '\n')
                if len(ast_diff) > 20:
                    errors.write(f'AND {len(ast_diff)} MORE LINES OF AST DIFF')
                errors.write(self.gen_diff(program_location, mutant_location) + '\n')

            return operators

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
