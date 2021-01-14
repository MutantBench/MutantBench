from rdflib import Graph, Literal, RDF, URIRef, RDFS
from fill_db.rdf import MutantBenchRDF
import subprocess
import pathlib
import os
from shutil import copyfile
import ctypes
from py4j.java_gateway import JavaGateway
import subprocess
import requests
import shutil
import os


def download(url, out_path):
    if os.path.isfile(out_path):
        return out_path
    r = requests.get(url)
    with open(out_path, 'wb') as f:
        f.write(r.content)
    return out_path


def download_program(program, mbrdf):
    return download(
        mbrdf.get_from(program, 'codeRepository'),
        f'/home/polo/thesis/MutantBench/programs/{mbrdf.get_from(program, "fileName")}'
    )


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
    if patch_cmd.returncode:
        print(location)
        print(difference)
        raise Exception(output, error)

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
    SEM = 'SEM'
    DEM = 'DEM'
    AEMG = 'AEMG'
    TYPES = {
        SEM: {
            'func_name': 'MBSuggestMutants',
            'processor': 'process_sem_out',
        },
        DEM: {
            'func_name': 'MBDetectMutants',
            'processor': 'process_dem_out',
        },
        AEMG: {
            'func_name': 'MBAvoidEquivalentMutants',
            'processor': 'process_aemg_out',
        },
    }

    def __init__(self, name, type_,  rdf, threshold=None):
        assert type_ != self.SEM or threshold is not None, 'For SEM tools, the threshold needs to be specified'
        self.name = name
        self.type = type_
        self.rdf = rdf
        self.threshold = threshold

    def MBDetectMutants(self, directory):
        """Detect if the mutants in the directory are equivalent.

        Returns the path to a file that contains an equivalent mutant on each row
        """
        raise NotImplementedError()

    def process_dem_out(self, out_file):
        """Takes file where each line is mutant uri that is equivalent"""
        mutants = []
        with open(out_file, 'r') as mutants_file:
            for m in mutants_file:
                mutants.append(self.rdf.get_full_uri(m[:-1], 'mutant'))
        return mutants

    def MBSuggestMutants(self, directory):
        """Suggest the equivalency of mutants in the directory.

        Returns the path to a file where each row is as follows:
        [mutant_uri], [probability]
        """
        raise NotImplementedError()

    def process_sem_out(self, out_file):
        """Takes file where each line is [mutant uri], [probability equivalent]"""
        mutants = []

        with open(out_file, 'r') as mutants_file:
            for m in mutants_file:
                if m and ', ' in m and len(m.split(', ')) == 2:
                    try:
                        mutant, probability = m.split(', ')
                        if float(probability) >= self.threshold:
                            mutants.append(self.rdf.get_full_uri(mutant[:-1], 'mutant'))
                    except (TypeError, ValueError):
                        pass
        return mutants

    def MBAvoidEquivalentMutants(self, directory):
        """Avoid equivalent mutant generation from original programs.

        Returns the path to in file where each row is as follows:
        [mutant_uri], [probability equivalent]
        """
        raise NotImplementedError()

    def process_aemg_out(self, out_dir):
        def get_diff(loc1, loc2):
            diff = subprocess.Popen(
                [f'/usr/bin/diff -u0 --ignore-all-space --ignore-blank-lines "{loc1}" "{loc2}"'],
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

            return output.decode("utf-8")
        non_equivalent_mutants = []

        for entry in os.scandir(out_dir):
            for e in os.scandir(entry):
                if e.is_file():
                    program = e
            program_uri = self.rdf.get_full_uri(program.name, 'program')
            for root, _, files in os.walk(entry):
                if not root.endswith('mutants'):
                    continue
                for mutant in files:
                    diff = get_diff(program.path, f'{root}/{mutant}')
                    found = False
                    for _, _, d in self.rdf.graph.triples((None, self.rdf.namespace.difference, None)):
                        if diff == str(d):
                            found = True
                            non_equivalent_mutants.append(
                                next(self.rdf.graph.subjects(self.rdf.namespace.difference, d)))
                            break
        return non_equivalent_mutants

    def benchmark(self, directory):
        out_path = self.execute_tool(directory)
        processed = getattr(self, self.TYPES[self.type]['processor'])(out_path)
        return processed


class CInterface(ctypes.CDLL, MBInterface):
    def execute_tool(self, directory):
        directory = ctypes.c_char_p(directory.encode('utf-8'))
        getattr(self, self.type).restype = ctypes.c_char_p
        return getattr(self, self.type)(directory)


class JavaInterface(JavaGateway, MBInterface):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.gateway = JavaGateway()
        self.interface = self.gateway.jvm.MBInterface()

    def execute_tool(self, directory):
        return getattr(self.interface, self.TYPES[self.type]["func_name"])(directory)


class BashInterface(MBInterface):
    def execute_tool(self, directory):
        """Bash interface only expects the last output to be the path of the mutants"""
        tool_output = subprocess.Popen(
            f'bash {self.name} {self.TYPES[self.type]["func_name"]} {directory}'.split(),
            stdout=subprocess.PIPE,
        )
        tail = subprocess.Popen(
            'tail -n 1'.split(),
            stdin=tool_output.stdout,
            stdout=subprocess.PIPE,
        )
        output, error = tail.communicate()
        if tail.returncode:
            print(directory)
            raise OSError(output, error)

        return output.decode("utf-8")[:-1]

class Benchmark(object):
    # TODO: remove this, user should provide the file in which the interface is
    INTERFACES = {
        'c': CInterface,
        'java': JavaInterface,
        'bash': BashInterface,
    }

    def __init__(
        self,
        interface_language,
        language,
        interface,
        output,
        type_,
        programs=[],
        equivalencies=None,
        operators=None,
        threshold=.5,
    ):
        self.type = type_
        self.language = language
        self.out_dir = output
        self.equivalencies = equivalencies
        self.threshold = threshold
        self.program_names = set(f'{p}.{language}' for p in programs) if programs else None
        self.rdf = MutantBenchRDF()
        self.interface = Benchmark.INTERFACES[interface_language](interface, self.type, self.rdf, self.threshold)
        self.operators = [self.rdf.get_full_uri(o, 'operator') for o in operators] if operators is not None else None

    def get_programs(self):
        return self.rdf.get_programs(programs=self.program_names, languages=[self.language])

    def get_mutants(self, program=None, equivalencies=None, operators=None):
        return self.rdf.get_mutants(
            program=program,
            equivalencies=equivalencies if equivalencies is not None else self.equivalencies,
            operators=operators if operators is not None else self.operators,
        )

    def get_operators(self):
        return sorted([
            o
            for o in self.rdf.get_operators()
            if self.operators is None or o in self.operators
        ])

    def get_program_name(self, program):
        return self.rdf.get_from(program, 'name')

    def get_program_path(self, program):
        return f'{self.out_dir}/{self.get_program_name(program)}'

    def get_program_location(self, program):
        return f'{self.get_program_path(program)}/{self.rdf.get_from(program, "name")}.{self.rdf.get_from(program, "extension")}'

    def get_mutant_path(self, mutant):
        return f'{self.get_program_path(self.rdf.get_from(mutant, "program"))}/mutants'

    def run(self):
        if self.type == 'AEMG':
            program_args = {
                'relevant': lambda program: {
                    'program': program,
                    'equivalencies': [False],
                },
                'non_relevant': lambda program: {
                    'program': program,
                    'equivalencies': [True],
                },
                'unknown': lambda program: {
                    'program': program,
                    'equivalencies': [None],
                },
            }
            operator_args = {
                'relevant': lambda operator: {
                    'operators': [operator],
                    'equivalencies': [False],
                },
                'non_relevant': lambda operator: {
                    'operators': [operator],
                    'equivalencies': [True],
                },
                'unknown': lambda operator: {
                    'operators': [operator],
                    'equivalencies': [None],
                },
            }
        else:
            program_args = {
                'relevant': lambda program: {
                    'program': program,
                    'equivalencies': [True],
                },
                'non_relevant': lambda program: {
                    'program': program,
                    'equivalencies': [False],
                },
                'unknown': lambda program: {
                    'program': program,
                    'equivalencies': [None],
                },
            }
            operator_args = {
                'relevant': lambda operator: {
                    'operators': [operator],
                    'equivalencies': [True],
                },
                'non_relevant': lambda operator: {
                    'operators': [operator],
                    'equivalencies': [False],
                },
                'unknown': lambda operator: {
                    'operators': [operator],
                    'equivalencies': [None],
                },
            }

        found_equiv_mutants = list(self.interface.benchmark(self.out_dir))
        print(len(found_equiv_mutants))
        if self.operators is not None:
            found_equiv_mutants = [
                p
                for p in found_equiv_mutants
                if all(o in self.operators for o in self.rdf.graph.objects(p, self.rdf.namespace.operator))
            ]
            print(len(found_equiv_mutants))
        # with open('examples/example_output', 'r') as f:
        #     found_equiv_mutants = [self.rdf.get_full_uri(m[:-1], 'mutant')
        #                            for m in f]
        #     print(found_equiv_mutants)
        program_stats = self.get_stats(
            found_equiv_mutants,
            self.get_programs(),
            get_mutants_args=program_args,
        )
        operator_stats = self.get_stats(
            found_equiv_mutants,
            self.get_operators(),
            get_mutants_args=operator_args,
        )

        program_stats.pop('labels')

        labels = []
        means = []
        std = []
        import numpy
        for label, stats in program_stats.items():
            means.append(numpy.mean([s for s in stats if s is not None], axis=0))
            std.append(numpy.std([s for s in stats if s is not None], axis=0))
            labels.append(label)
        from matplotlib import pyplot as plt
        plt.errorbar(labels, means, std, linestyle='None', marker='^')
        plt.show()

        for i, label in enumerate(operator_stats.pop('labels')):
            print(
                label.split('#')[1], ' & ',
                round(operator_stats['precision'][i], 2) if operator_stats['precision'][i] is not None else '-', ' & ',
                round(operator_stats['recall'][i], 2) if operator_stats['recall'][i] is not None else '-', ' & ',
                round(operator_stats['accuracy'][i], 2) if operator_stats['accuracy'][i] is not None else '-', ' & ',
                round(operator_stats['F1'][i], 2) if operator_stats['F1'][i] is not None else '-', ' & ',
                round(operator_stats['F2'][i], 2) if operator_stats['F2'][i] is not None else '-', ' & ',
                round(operator_stats['F0_5'][i], 2) if operator_stats['F0_5'][i] is not None else '-', '\\\\'
            )

    def get_stats(self, found_equiv_mutants, loop_elements, get_mutants_args=None, print_=False):

        stats = {
            'precision': [],
            'recall': [],
            'accuracy': [],
            'F1': [],
            'F2': [],
            'F0_5': [],
            'labels': [],
        }

        for elem in loop_elements:
            relevant_elements = list(self.get_mutants(**get_mutants_args['relevant'](elem)))
            n_relevant_elements = len(relevant_elements)
            non_relevant_elements = list(self.get_mutants(**get_mutants_args['non_relevant'](elem)))
            n_non_relevant_elements = len(non_relevant_elements)
            n_unknown = list(self.get_mutants(**get_mutants_args['unknown'](elem)))

            n_true_positives = len(list([
                mutant
                for mutant in relevant_elements
                if mutant in found_equiv_mutants
            ]))
            n_false_positives = len(list([
                mutant
                for mutant in non_relevant_elements
                if mutant in found_equiv_mutants
            ]))
            n_false_negatives = n_non_relevant_elements - n_false_positives

            for i, p in zip(self.get_metrics(
                n_true_positives,
                n_relevant_elements - n_true_positives,
                n_false_positives,
                n_false_negatives,
                n_unknown,
                print_=print_,
            ), stats.keys()):
                stats[p].append(i)
            stats['labels'].append(elem)

        return stats

    def get_metrics(self, tp, tn, fp, fn, unknown, print_=True):
        selected = tp + fp
        relevant = tp + tn
        unrelevant = fp + fn
        correct = tp + fn
        total = tp + tn + fp + fn
        precision = calc_precision(tp, selected)
        recall = calc_recall(tp, relevant)
        accuracy = calc_accuracy(correct, total)
        F1 = calc_Fb(recall or 0, precision or 0, beta=1)
        F2 = calc_Fb(recall or 0, precision or 0, beta=2)
        F0_5 = calc_Fb(recall or 0, precision or 0, beta=0.5)
        if print_:
            print(f'Contains: {relevant} relevant, {unrelevant} unrelevant, {unknown} unknown')
            print(f'Found: {tp} true positives, {fp} false positives')
            print('Precision:', precision)
            print('Recall:', recall)
            print('Fb (1,2,0.5):', F1, F2, F0_5)
            print('Accuracy:', accuracy)
        return (
            precision,
            recall,
            accuracy,
            F1,
            F2,
            F0_5,
        )

    def generate_program(self, program):
        if not os.path.exists(f'{self.out_dir}/{self.get_program_name(program)}'):
            pathlib.Path(f'{self.out_dir}/{self.get_program_name(program)}/mutants')\
                .mkdir(parents=True, exist_ok=True)

        copyfile(download_program(program, self.rdf), self.get_program_location(program))

    def generate_mutant(self, mutant):
        directory = self.get_mutant_path(mutant)
        mutant_file_name = f'{mutant.split("#")[1]}.{self.rdf.get_program_from_mutant(mutant).split(".")[-1]}'
        mutant_file_location = f'{directory}/{mutant_file_name}'

        copyfile(download_program(self.rdf.get_from(mutant, 'program'), self.rdf), mutant_file_location)
        patch_mutant(self.rdf.get_from(mutant, 'difference'), mutant_file_location)

    def generate_test_dataset(self):
        if os.path.exists(self.out_dir):
            shutil.rmtree(self.out_dir)
        pathlib.Path(self.out_dir).mkdir(parents=True, exist_ok=True)

        for program in self.get_programs():
            mutants = list(self.get_mutants(program))

            if len(mutants) == 0:
                continue

            self.generate_program(program)

            if self.type != 'AEMG':
                for mutant in mutants:
                    self.generate_mutant(mutant)

