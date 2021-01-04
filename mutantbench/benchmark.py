import subprocess
import pathlib
import os
from shutil import copyfile
import ctypes
from py4j.java_gateway import JavaGateway
from mutantbench import session, db
import subprocess
import shutil
from mutantbench.utils import patch_mutant



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
        if error:
            raise OSError(error)

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

    def get_programs(self):
        program_qry = session.query(db.Program)\
            .filter(db.Program.language == self.language)
        if self.program_names:
            program_qry = program_qry.filter(
                db.Program.file_name.in_(self.program_names))
        return program_qry.order_by(db.Program.file_name)

    def get_mutants(self, program):
        mutant_qry = (
            session.query(db.Mutant)
            .join(db.Program)
            .filter(db.Program.language == self.language)
            .filter(db.Program.id == program.id)
        )
        if self.equivalencies is not None:
            mutant_qry = mutant_qry.filter(
                db.Mutant.equivalent.in_(self.equivalencies)
            )
        if self.operators is not None:
            mutant_qry = mutant_qry.filter(
                db.Mutant.operators.any(db.Operator.in_(self.operators))
            )
        return mutant_qry

    def get_program_path(self, program):
        return f'{self.out_dir}/{program.name}'

    def get_program_location(self, program):
        return f'{self.get_program_path(program)}/original.{program.extension}'

    def get_mutant_path(self, mutant):
        return f'{self.get_program_path(mutant.program)}/mutants'

    def run(self):
        result = self.interface.detect_mutants(self.out_dir)
        found_equivs, found_non_equivs = [], []
        for mutant_id, probability in result.items():
            if probability > self.threshold:
                found_equivs.append(mutant_id)
            else:
                found_non_equivs.append(mutant_id)

        for program in self.get_programs():
            mutant_qry = self.get_mutants(program)

            relevant_elements = mutant_qry.filter(db.Mutant.equivalent)
            n_relevant_elements = relevant_elements.count()
            non_relevant_elements = mutant_qry.filter(
                db.Mutant.equivalent is False)
            n_non_relevant_elements = non_relevant_elements.count()
            n_unknown = mutant_qry.filter(db.Mutant.equivalent is None).count()

            n_true_positives = relevant_elements.filter(
                db.Mutant.id.in_(found_equivs)).count()
            n_false_positives = non_relevant_elements.filter(
                db.Mutant.id.in_(found_equivs)).count()
            n_false_negatives = n_non_relevant_elements - n_false_positives
            n_correct = n_false_negatives + n_true_positives

            n_selected_elements = n_true_positives + n_false_positives

            precision = calc_precision(n_true_positives, n_selected_elements)
            recall = calc_recall(n_true_positives, n_relevant_elements)
            accuracy = calc_accuracy(n_correct, mutant_qry.count())
            F1 = calc_Fb(recall, precision, beta=1)
            F2 = calc_Fb(recall, precision, beta=2)
            F0_5 = calc_Fb(recall, precision, beta=0.5)
            print('Program:', program.name)
            print(f'Contains: {n_relevant_elements} equivalent, {n_non_relevant_elements} non equivalent, {n_unknown} unknown')
            print('Precision:', precision)
            print('Recall:', recall)
            print('Fb (1,2,0.5):', F1, F2, F0_5)
            print('Accuracy:', accuracy)

    def generate_program(self, program):
        if not os.path.exists(f'{self.out_dir}/{program.name}'):
            pathlib.Path(f'{self.out_dir}/{program.name}/mutants')\
                .mkdir(parents=True, exist_ok=True)

        copyfile(program.path, self.get_program_location(program))

    def generate_mutant(self, mutant):
        directory = self.get_mutant_path(mutant)
        mutant_file_name = f'{mutant.id}.{mutant.program.extension}'
        mutant_file_location = f'{directory}/{mutant_file_name}'

        copyfile(mutant.program.path, mutant_file_location)
        patch_mutant(mutant.diff, mutant_file_location)

    def generate_test_dataset(self):
        if os.path.exists(self.out_dir):
            shutil.rmtree(self.out_dir)
        pathlib.Path(self.out_dir).mkdir(parents=True, exist_ok=True)

        for program in self.get_programs():
            mutants = self.get_mutants(program)

            if mutants.count() == 0:
                continue

            self.generate_program(program)

            for mutant in mutants:
                self.generate_mutant(mutant)

