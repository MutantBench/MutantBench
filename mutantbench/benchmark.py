import ctypes
from py4j.java_gateway import JavaGateway
from mutantbench import session, db


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

    def __init__(self, mutants, language, interface):
        self.mutants = mutants
        self.language = language
        self.interface = Benchmark.INTERFACES[language](interface)

        self.generate_mutants()  # TODO: remove

    def generate_mutants(
            self,
            equivalencies=None,
            operators=None,
            ):
        mutant_qry = session.query(db.Mutant)
        if equivalencies is not None:
            mutant_qry = mutant_qry.filter(
                db.Mutant.equivalent.in_(equivalencies)
            )
        if operators is not None:
            mutant_qry = mutant_qry.filter(
                db.Mutant.operators.any(db.Operator.x.in_(operators))
            )

        print(mutant_qry, mutant_qry.all())
        result = self.interface.detect_mutants(['generated'])
        assert result >= 0, f'[ERROR] An error occured while processing the mutants with error code {result}'
