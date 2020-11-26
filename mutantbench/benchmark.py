import ctypes
from py4j.java_gateway import JavaGateway


class CInterface(ctypes.CDLL):
    def detec_mutants(self, locations):
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

    def detec_mutants(self, locations):
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

        self.detect_interface(['test1', 'test2'])  # TODO: remove


    def detect_interface(self, locations):
        result = self.interface.detec_mutants(locations)
        assert result >= 0, f'[ERROR] An error occured while processing the mutants with error code {result}'