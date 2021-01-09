from fill_db.rdf import MutantBenchRDF
import subprocess
import pathlib
import os
from shutil import copyfile
import ctypes
from py4j.java_gateway import JavaGateway
from mutantbench import session, db
import subprocess
import shutil


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

    # TODO fix error checking
    if error:
        raise Exception(error)

    return output


def calc_precision(true_positives, selected_elements):
    try:
        return true_positives / selected_elements
    except ZeroDivisionError:
        return None


def calc_recall(true_positives, relevant_elements):
    try:
        return true_positives / relevant_elements
    except ZeroDivisionError:
        return None


def calc_Fb(recall, precision, beta=1):
    top = (precision * recall)
    bottom = (beta**2 * precision) + recall
    try:
        return (1 + beta**2) * (top / bottom)
    except ZeroDivisionError:
        return None


def calc_accuracy(correct, total):
    try:
        return correct / total
    except ZeroDivisionError:
        return None


class MBInterface(object):
    def detect_mutants(self, directory):
        """Detect if the mutants in the directory are equivalent.

        Returns a directory where the key is the mutant id or filename and the
        value is the probability of change that the mutant is equivalent.
        """
        raise NotImplementedError()


class CInterface(ctypes.CDLL, MBInterface):
    def detect_mutants(self, locations):
        locations = ctypes.c_char_p(','.join(locations).encode('utf-8'))
        self.MBDetectMutants.restype = ctypes.c_int
        return self.MBDetectMutants(locations)

    def get_results(self):
        return self.interface.MBSetResults()


class JavaInterface(JavaGateway, MBInterface):
    def __init__(self, name):
        self.name = name
        self.gateway = JavaGateway()
        self.interface = self.gateway.jvm.MBInterface()

    def detect_mutants(self, locations):
        return self.interface.MBDetectMutants(','.join(locations))


class BashInterface(MBInterface):
    def __init__(self, name):
        self.name = name

    def detect_mutants(self, locations):
        """Detect if the mutants in the directory are equivalent.

        Returns a directory where the key is the mutant id or filename and the
        value is the probability of change that the mutant is equivalent.
        A value of -1 will be treated as unknown probability.

        This interface expects the bash program to print the following out for
        each mutant:
        [mutant_id], [probability]
        """
        tail = subprocess.Popen(
            f'bash {self.name} MBDetectMutants {locations}'.split(),
            stdout=subprocess.PIPE,
        )
        output, error = tail.communicate()

        mutants_out = output.decode("utf-8").split('\n')
        mutants = {}
        for m in mutants_out:
            if m and ', ' in m and len(m.split(', ')) == 2:
                try:
                    mutant, probability = m.split(', ')
                    mutants[int(mutant)] = int(probability)
                except (TypeError, ValueError):
                    pass
        return mutants


class Benchmark(object):
    # TODO: remove this, user should provide the file in which the interface is
    INTERFACES = {
        'c': CInterface,
        'java': JavaInterface,
        'bash': BashInterface,
    }
    LANGUAGES = {
        'c': db.Languages.c,
        'java': db.Languages.java,
    }

    def __init__(
        self,
        interface_language,
        language,
        interface,
        output,
        programs=[],
        equivalencies=None,
        operators=None,
        threshold=.5,
    ):
        self.language = self.LANGUAGES[language]
        self.interface = Benchmark.INTERFACES[interface_language](interface)
        self.out_dir = output
        self.equivalencies = equivalencies
        self.operators = operators
        self.threshold = threshold
        self.program_names = set(f'{p}.{language}' for p in programs)
        self.rdf = MutantBenchRDF()

    def get_programs(self):
        return self.rdf.get_programs(programs=self.program_names)

    def get_mutants(self, program, equivalencies=None, operators=None):
        return self.rdf.get_mutants(
            program=program,
            equivalencies=equivalencies if equivalencies is not None else self.equivalencies,
            operators=operators if operators is not None else self.operators,
        )

    def get_program_name(self, program):
        return self.rdf.get_from(program, 'name')

    def get_program_path(self, program):
        return f'{self.out_dir}/{self.get_program_name(program)}'

    def get_program_location(self, program):
        return f'{self.get_program_path(program)}/original.{self.rdf.get_from(program, "extension")}'

    def get_mutant_path(self, mutant):
        return f'{self.get_program_path(self.rdf.get_from(mutant, "program"))}/mutants'

    def run(self):
        result = self.interface.detect_mutants(self.out_dir)
        found_equivs, found_non_equivs = [], []
        for mutant_id, probability in result.items():
            if probability > self.threshold:
                found_equivs.append(mutant_id)
            else:
                found_non_equivs.append(mutant_id)

        for program in self.get_programs():
            relevant_elements = list(self.get_mutants(program, equivalencies=[True]))
            n_relevant_elements = len(relevant_elements)
            non_relevant_elements = list(self.get_mutants(program, equivalencies=[False]))
            n_non_relevant_elements = len(non_relevant_elements)
            n_unknown = len(list(self.get_mutants(program, equivalencies=[None])))

            n_true_positives = len(list([
                mutant
                for mutant in relevant_elements
                if int(mutant.split('#')[1]) in found_equivs
            ]))
            n_false_positives = len(list([
                mutant
                for mutant in non_relevant_elements
                if int(mutant.split('#')[1]) in found_equivs
            ]))
            n_false_negatives = n_non_relevant_elements - n_false_positives
            n_correct = n_false_negatives + n_true_positives

            n_selected_elements = n_true_positives + n_false_positives

            precision = calc_precision(n_true_positives, n_selected_elements)
            recall = calc_recall(n_true_positives, n_relevant_elements)
            accuracy = calc_accuracy(n_correct, len(list(self.get_mutants(program))))
            F1 = calc_Fb(recall or 0, precision or 0, beta=1)
            F2 = calc_Fb(recall or 0, precision or 0, beta=2)
            F0_5 = calc_Fb(recall or 0, precision or 0, beta=0.5)
            print('Program:', self.get_program_name(program))
            print(f'Contains: {n_relevant_elements} equivalent, {n_non_relevant_elements} non equivalent, {n_unknown} unknown')
            print('Precision:', precision)
            print('Recall:', recall)
            print('Fb (1,2,0.5):', F1, F2, F0_5)
            print('Accuracy:', accuracy)

    def generate_program(self, program):
        if not os.path.exists(f'{self.out_dir}/{self.get_program_name(program)}'):
            pathlib.Path(f'{self.out_dir}/{self.get_program_name(program)}/mutants')\
                .mkdir(parents=True, exist_ok=True)

        copyfile(self.rdf.get_from(program, 'codeRepository'), self.get_program_location(program))

    def generate_mutant(self, mutant):
        directory = self.get_mutant_path(mutant)
        mutant_file_name = f'{mutant.split("#")[1]}.{self.rdf.get_program_from_mutant(mutant).split(".")[-1]}'
        mutant_file_location = f'{directory}/{mutant_file_name}'

        copyfile(self.rdf.get_from(self.rdf.get_from(mutant, 'program'), 'codeRepository'), mutant_file_location)
        patch_mutant(self.rdf.get_from(mutant, 'difference'), mutant_file_location)

    def generate_test_dataset(self):
        if os.path.exists(self.out_dir):
            shutil.rmtree(self.out_dir)
        pathlib.Path(self.out_dir).mkdir(parents=True, exist_ok=True)

        for program in self.get_programs():
            mutants = self.get_mutants(program)

            # if mutants.count() == 0:
            #     continue

            self.generate_program(program)

            for mutant in mutants:
                self.generate_mutant(mutant)

