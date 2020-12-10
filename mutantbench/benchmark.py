import os
from shutil import copyfile
import ctypes
from py4j.java_gateway import JavaGateway
from mutantbench import session, db
import subprocess


PATCH_FORMAT = """--- {from_file}
+++ {to_file}
{diff}
"""


class CInterface(ctypes.CDLL):
    def detect_mutants(self, locations):
        locations = ctypes.c_char_p(','.join(locations).encode('utf-8'))
        self.MBDetectMutants.restype = ctypes.c_int
        return self.MBDetectMutants(locations)

    def get_results(self):
        return self.interface.MBSetResults()


class JavaInterface(JavaGateway):
    def __init__(self, name):
        self.name = name
        self.gateway = JavaGateway()
        self.interface = self.gateway.jvm.MBInterface()

    def detect_mutants(self, locations):
        return self.interface.MBDetectMutants(','.join(locations))


class Benchmark(object):
    INTERFACES = {
        'c': CInterface,
        'java': JavaInterface,
    }
    LANGUAGES = {
        'c': db.Languages.c,
        'java': db.Languages.java,
    }

    def __init__(self, mutants, language, interface, output):
        self.mutants = mutants
        self.language = self.LANGUAGES[language]
        self.interface = Benchmark.INTERFACES[language](interface)
        self.out_dir = output

        self.generate_mutants()  # TODO: remove

    def generate_mutants(
            self,
            equivalencies=None,
            operators=None,
            ):
        for program in session.query(db.Program).all():
            mutant_qry = (
                session.query(db.Mutant)
                .join(db.Program)
                .filter(db.Program.language == self.language)
                .filter(db.Program.id == program.id)
            )
            if equivalencies is not None:
                mutant_qry = mutant_qry.filter(
                    db.Mutant.equivalent.in_(equivalencies)
                )
            if operators is not None:
                mutant_qry = mutant_qry.filter(
                    db.Mutant.operators.any(db.Operator.x.in_(operators))
                )

            if mutant_qry.count() == 0:
                continue

            # TODO remove this var
            new_program_file = f'{self.out_dir}/{program.file_name}'
            copyfile(program.path, new_program_file)

            if not os.path.exists(f'{self.out_dir}/{program.name}'):
                os.makedirs(f'{self.out_dir}/{program.name}')

            for mutant in mutant_qry.all():
                mutant_file_location = f'{self.out_dir}/{program.name}/{mutant.id}.{program.extension}'

                copyfile(program.path, mutant_file_location)
                patch_stdin = PATCH_FORMAT.format(
                    from_file=mutant_file_location,
                    to_file=mutant_file_location,
                    diff=mutant.diff,
                )
                patch_cmd = subprocess.Popen(
                    ['patch -p0'],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    shell=True,
                )
                patch_cmd.stdin.write(str.encode(patch_stdin))
                output, error = patch_cmd.communicate()

                if error:
                    raise Exception(error)


        # for mutant in mutant_qry.all():

        result = self.interface.detect_mutants(['generated'])
        assert result >= 0, f'[ERROR] An error occured while processing the mutants with error code {result}'
