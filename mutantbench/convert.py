import sys
import argparse
from rdf import MutantBenchRDF, SCHEMA, MB
from rdflib import Literal, URIRef
import os
import re
from shutil import copyfile
import subprocess
import difflib
import requests


PATCH_FORMAT = """--- {from_file}
+++ {to_file}
{diff}
"""


def patch_mutant(difference, location):
    directory = os.path.dirname(location)
    file_name = os.path.basename(location)
    patch_stdin = PATCH_FORMAT.format(
        from_file=file_name,
        to_file=file_name,
        diff=difference,
    )
    patch_cmd = subprocess.Popen(
        [f'patch -p0 -d{directory}'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        shell=True,
    )
    patch_cmd.stdin.write(str.encode(patch_stdin))
    output, error = patch_cmd.communicate()

    if patch_cmd.returncode:
        print(location)
        print(patch_stdin)
        raise Exception(output, error)

    return output


def download(url, out_path):
    r = requests.get(url)
    with open(out_path, 'wb') as f:
        f.write(r.content)
    return out_path


class OperatorNotFound(Exception):
    def __init__(self, operator=None):
        self.message = 'Operator not found'
        if operator:
            self.message = f'{self.message}: {operator}'


class ConvertDataset(object):
    def __init__(self, language, directory, source, out_dir):
        # Initialize passed through variables
        self.language = language
        self.source = URIRef(f'mb:paper#{source}')
        self.directory = directory
        self.out_dir = out_dir
        self.out_url = f'https://raw.githubusercontent.com/MutantBench/MutantBench/main/mutantbench/programs/'
        self.rdf = MutantBenchRDF()

        # Create program directory
        if not os.path.exists(self.out_dir):
            os.makedirs(self.out_dir)

        # Initialize other variables
        self.programs = []
        self.mutants = []

    def get_program_filename(self, path):
        return str(os.path.basename(path))

    def gen_programs(self, enforce_gen=False):
        """Generate the programs from this dataset and add them to the database.

        Does not generate programs that already exists, although it does return
        them. Generation of duplicates can be enforced by setting [enforce_gen]
        to True.

        NOTE: requires the own implementation of `get_program_locations`
        """
        try:
            for program_location in self.get_program_locations():
                file_name = self.get_program_filename(program_location)
                path = f'{self.out_dir}/{file_name}'
                url = f'{self.out_url}/{file_name}'
                print(path)

                copyfile(program_location, path)

                self.programs.append(self.rdf.get_or_create(
                    name=URIRef(f'mb:program#{file_name}'),
                    type_=MB.Program,
                    predicate_object_pairs=[
                        (SCHEMA.codeRepository, Literal(url, datatype=SCHEMA.URL)),
                        (SCHEMA.programmingLanguage, Literal(self.language)),
                        (MB.extension, Literal(file_name.split('.')[-1])),
                        (SCHEMA.name, Literal('.'.join(file_name.split('.')[:-1]))),
                        (MB.filename, Literal('.'.join(file_name.split('.')[:-1]))),
                    ]
                ))

        except KeyboardInterrupt as e:
            if input('do you want to export? (n for no)'):
                raise e
            else:
                self.rdf.export()
                exit(0)

        self.rdf.export()
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
        if tail.returncode:
            raise OSError(output, error)

        output = output.decode("utf-8")
        if output and not self.check_output(output, program_location, mutant_location):
            print(output)
            print(program_location, mutant_location)
            if input('Output was not correct, does this need to be fixed? (n if not)') == 'n':
                return output

            fixed_output = self.fix_output(output, program_location, mutant_location)
            print(fixed_output)
            if input('Would you like to apply this fixed output? (n if not)') == 'n':
                return output

            output = fixed_output

            if input('Would you also like to change the original mutant to this output? (n if not)') == 'n':
                return output

            copyfile(program_location, mutant_location)
            patch_mutant(output, mutant_location)

        return output

    def gen_ast_diff(self, program_location, mutant_location):
        """Generate the difference between two files."""
        return [
            line
            for line in subprocess.run(
                ['gumtree/gumtree-2.1.2/bin/gumtree', 'diff', program_location, mutant_location],
                capture_output=True
            ).stdout.decode('utf-8').split('\n')
            if not line.startswith('Match ')
        ]

    def get_program_path(self, program):
        path = f'{self.out_dir}/{self.rdf.get_from(program, "fileName")}'
        if not os.path.isfile(path):
            url = f'{self.out_url}/{self.rdf.get_from(program, "fileName")}'
            download(url, path)
        return path

    def gen_mutants(self, enforce_gen=False):
        """Generate mutants and add them to the database.

        Does not generate mutants that already exists, although it does return
        them. Generation of duplicates can be enforced by setting [enforce_gen]
        to True.

        NOTE: requires the own implementation of `get_mutant_locations`"""
        if not self.programs:
            self.gen_programs(enforce_gen=enforce_gen)

        try:
            for program, mutant_locations in self.get_mutant_locations().items():
                for (mutant_location, equivalency) in mutant_locations:
                    path = self.get_program_path(program)
                    diff = self.gen_diff(path, mutant_location)
                    # Skip programs that do not actually contain any diff
                    if not diff:
                        print('Mutant is empty, skipping')
                        print(mutant_location)
                        continue

                    if self.rdf.check_mutant_exists(self.rdf.get_from(program, 'fileName'), diff):
                        print('Mutant already in databset, skipping')
                        continue

                    char_diff = self.get_char_diff(diff)
                    # Skip programs that do not actually contain any diff
                    if not char_diff:
                        print('Mutant is empty, skipping')
                        print(mutant_location)
                        continue

                    operators = self.get_operators_from_mutant_location(
                        self.rdf.get_from(program, 'codeRepository'),
                        mutant_location,
                    )

                    predicate_object_pairs = [
                        (MB.difference, Literal(diff)),
                        (SCHEMA.contributor, self.source),
                        (MB.program, program),
                    ]
                    if equivalency is not None:
                        predicate_object_pairs.append(
                            (MB.equivalence, Literal(equivalency, datatype=SCHEMA.boolean))
                        )
                    predicate_object_pairs += [
                        (MB.operator, operator)
                        for operator in operators
                    ]
                    self.mutants.append(self.rdf.get_or_create(
                        URIRef(f'mb:mutant#{self.rdf.get_mutant_hash(self.rdf.get_from(program, "fileName"), diff)}'),
                        type_=MB.Mutant,
                        predicate_object_pairs=predicate_object_pairs
                    ))

        except KeyboardInterrupt as e:
            if input('do you want to export? (n for no)'):
                raise e
            else:
                self.rdf.export()
                exit(0)
        self.rdf.export()
        return self.mutants

    def get_program_from_name(self, program_name):
        return URIRef(f'mb:program#{program_name}')

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

    def get_operators_from_mutant_location(self, program_location, mutant_location):
        """Returns a list of operators that the mutant used."""
        operators = list()
        mapping = [
            (r'Insert (MethodInvocation|FunCall)\(\d+\)', 'ABSI1'),
            (r'Insert (SimpleName|GenericString): abs\(\d+\)', 'ABSI2'),
            (r'Update (InfixExpression|GenericString): [+\-*/%]\(\d+\) to [+\-*/%]', 'AORB'),
            (r'Insert (InfixExpression): [+\-*/%]\(\d+\) into (InfixExpression): [+\-*/%]\(\d+\)', 'AORB'),
            (r'Update ((Pre|Post)fixExpression|GenericString): (\+\+|--)\(\d+\) to (\+\+|--)', 'AORS'),
            (r'Insert (PrefixExpression|GenericString): [+\-]\(\d+\)', 'AOIU'),
            (r'Insert ((Pre|Post)fixExpression|GenericString): (\+\+|--)\(\d+\)', 'AOIS'),

            (r'Delete (InfixExpression|GenericString): [+\-*/%]\(\d+\)', 'AODB'),

            (r'Delete PrefixExpression: [+\-]\(\d+\)', 'AODU')
            if self.language == 'java' else
            (r'Delete Unary\(\d+\)', 'AODU'),

            (r'Delete ((Pre|Post)fixExpression|GenericString): (\+\+|--)\(\d+\)', 'AODS'),

            (r'Update (InfixExpression|GenericString): (>=|<=|>|<|!=|==)\(\d+\) to (>=|<=|>|<|!=|==)', 'ROR'),
            (r'Update GenericString: (>=|<=|>|<|!=|==)\(\d+\) to (>=|<=|>|<|!=|==)', 'ROR'),
            # (r'Insert BooleanLiteral: (true|false)\(\d+\)', 'ROD+'),
            (r'Delete (InfixExpression|GenericString): (>=|<=|>|<|!=|==)\(\d+\)', 'ROD'),
            (r'Delete GenericString: (>=|<=|>|<|!=|==)\(\d+\)', 'ROD'),

            (r'Update (InfixExpression|GenericString): (\|\||&&)\(\d+\) to (\|\||&&)', 'SEOR'),
            (r'Delete (InfixExpression|GenericString): (\|\||&&)\(\d+\)', 'SEOD'),
            (r'Delete (PrefixExpression|GenericString): \!\(\d+\)', 'SEOD'),
            (r'Insert (PrefixExpression|GenericString): \!\(\d+\)', 'SEOI'),

            (r'Update (InfixExpression|GenericString): (>>|<<|>>>)\(\d+\) to (>>|<<|>>>)', 'SOR'),

            (r'Update (InfixExpression|GenericString): (&|\||\^)\(\d+\) to (&|\||\^)', 'LOR'),
            (r'Insert (PrefixExpression|GenericString): ~\(\d+\)', 'LOI'),
            (r'Delete (PrefixExpression|GenericString): ~\(\d+\)', 'LOD'),

            (r'Update Assignment: (\+=|\-=|\*=|%=|\/=|&=|\^=|>>=|<<=|>>>=)\(\d+\) to (\+=|\-=|\*=|%=|\/=|&=|\^=|>>=|<<=|>>>=)', 'ASRS'),

            (r'Update Assignment: (\+=|\-=|\*=|%=|\/=|&=|\^=|>>=|<<=|>>>=)\(\d+\) to =', 'VDL'),

            (r'Delete ((Simple|Qualified)Name|GenericString): \w[\w.\d]*\(\d+\)', 'VDL'),
            (r'Insert ((Simple|Qualified)Name|GenericString): \w[\w.\d]*\(\d+\)', '!VDL'),

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
        operator_counts['AORS'] += min(operator_counts['AOIS'], operator_counts['AODS'])
        operator_counts['AODS'] -= operator_counts['AORS']
        operator_counts['AOIS'] -= operator_counts['AORS']

        operator_counts['ABSI'] = min(operator_counts.pop('ABSI1'), operator_counts.pop('ABSI2'))
        operator_counts['AODB'] -= operator_counts['AODU']
        operator_counts['AORB'] += operator_counts.pop('AODB')

        operator_counts['AOIU'] -= operator_counts['ABSI']

        if operator_counts['SDL'] > 0:
            operators = ['SDL']
        else:
            for operator_name, count in operator_counts.items():
                operators += [operator_name] * count

        return [
            URIRef(f'mb:operator#{o}')
            for o in operators
        ]

    def check_output(self, output, program_location, mutant_location):
        """Check if the output is to be expected for the program and mutant

        Default returns True"""
        return True

    def fix_output(self, output, program_location, mutant_location):
        """Fix the output when [check_output] returns False"""
        return NotImplementedError

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


def get_argument_parser():
    parser = argparse.ArgumentParser(
        description='Converter tool for converting datasets to a FAIR data standard')
    arguments = {
        'language': {
            'nargs': 1,
            'type': str,
            'choices': ['java', 'c'],
            'help': 'the language of the dataset',
        },
        'source': {
            'nargs': 1,
            'type': str,
            'help': 'a reference to the paper where the mutants originate from',
        },
        'directory': {
            'nargs': 1,
            'type': str,
            'help': 'the directory of the dataset',
        },
        'out_dir': {
            'nargs': 1,
            'type': str,
            'help': 'the directory you want the programs to be generated in',
        },
    }
    for args, kwargs in arguments.items():
        parser.add_argument(*args.split(), **kwargs)
    return parser


if __name__ == '__main__':
    args = get_argument_parser().parse_args(sys.argv[1:])
    converter = ConvertDataset(
        language=args.language[0],
        source=args.name[0],
        directory=args.directory[0],
        out_dir=args.out_dir[0],
    )
    converter.gen_programs()
    converter.gen_mutants()
