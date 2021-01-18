from sklearn.metrics import roc_curve
from matplotlib import pyplot as plt
import numpy as np
import pandas as pd
from rdflib import Graph, Literal, RDF, URIRef, RDFS
from fill_db.rdf import MutantBenchRDF, MB
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
        [f'patch -l -p0 -d{directory}'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        shell=True,
    )
    patch_cmd.stdin.write(str.encode(patch_stdin))
    output, error = patch_cmd.communicate()

    # TODO fix error checking
    if patch_cmd.returncode:
        print(location)
        print(patch_stdin)
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
        truth = []
        probabilities = []
        with open(out_file, 'r') as mutants_file:
            for m in mutants_file:
                if m and ', ' in m and len(m.split(', ')) == 2:
                    try:
                        mutant, probability = m.split(', ')
                        mutant = self.rdf.get_full_uri(mutant, 'mutant')
                        probabilities.append(float(probability))
                        truth.append(str(self.rdf.get_from(mutant, 'equivalence')) == 'true')
                        if float(probability) >= self.threshold:
                            mutants.append(mutant)
                    except (TypeError, ValueError):
                        pass
        with open('test.txt', 'w') as out:
            out.write(str(truth))
            out.write(str(probabilities))

        def plot_roc_curve(fpr, tpr):
            plt.plot(fpr, tpr, color='orange', label='ROC')
            plt.plot([0, 1], [0, 1], color='darkblue', linestyle='--')
            plt.xlabel('False Positive Rate')
            plt.ylabel('True Positive Rate')
            plt.title('Receiver Operating Characteristic (ROC) Curve')
            plt.legend()
            plt.show()

        fpr, tpr, thresholds = roc_curve(truth, probabilities)
        plot_roc_curve(fpr, tpr)

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
        do_not_gen_dataset=False,
    ):
        self.type = type_
        self.language = language
        self.out_dir = output
        self.equivalencies = equivalencies
        self.threshold = threshold
        self.program_names = set(f'{p}.{language}' for p in programs) if programs else None
        self.rdf = MutantBenchRDF()
        self.interface = Benchmark.INTERFACES[interface_language](
            interface,
            self.type,
            self.rdf,
            threshold=self.threshold
        )
        self.do_not_gen_dataset = do_not_gen_dataset
        self.operators = [self.rdf.get_full_uri(o, 'operator') for o in operators] if operators is not None else None

    def get_programs(self):
        return self.rdf.get_programs(programs=self.program_names, languages=[self.language])

    def get_mutants(self, program=None, equivalencies=None, operators=None):
        if program is None:
            mutants = []
            for p in self.get_programs():
                mutants += list(self.rdf.get_mutants(
                    program=p,
                    equivalencies=equivalencies if equivalencies is not None else self.equivalencies,
                    operators=operators if operators is not None else self.operators,
                ))
            return mutants
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
        # with open('examples/example_output', 'r') as f:
        #     found_mutants = [self.rdf.get_full_uri(m[:-1], 'mutant') for m in f]
        print('Running tool..')
        found_mutants = list(self.interface.benchmark(self.out_dir))
        print('Done running tool, generating metrics...')

        if self.operators is not None:
            found_mutants = [
                p
                for p in found_mutants
                if all(o in self.operators for o in self.rdf.graph.objects(p, self.rdf.namespace.operator))
            ]

        if self.type == 'AEMG':
            found_killable_mutants = found_mutants
        else:
            found_killable_mutants = [
                m
                for m in self.get_mutants()
                if m not in found_mutants
            ]

        tps, fns, fps, tns = set(), set(), set(), set()
        for mutant in self.get_mutants(equivalencies=[False]):
            if mutant in found_killable_mutants:
                tps.add(mutant)
            else:
                fns.add(mutant)
        for mutant in self.get_mutants(equivalencies=[True]):
            if mutant in found_killable_mutants:
                fps.add(mutant)
            else:
                tns.add(mutant)

        with open('classification_report.csv', 'w') as f:
            f.writelines([
                f'{mutant.split("#")[1]}, TP\n'
                for mutant in tps
            ] + [
                f'{mutant.split("#")[1]}, FN\n'
                for mutant in fns
            ] + [
                f'{mutant.split("#")[1]}, FP\n'
                for mutant in fps
            ] + [
                f'{mutant.split("#")[1]}, TN\n'
                for mutant in tns
            ])
        print('Generated classification report...')

        self.generate_generic_metrics(tps, fns, fps, tns)
        self.generate_program_metrics(tps, fns, fps, tns)
        self.generate_operator_metrics(tps, fns, fps, tns)
        self.generate_operator_action_metrics(tps, fns, fps, tns)
        self.generate_operator_class_metrics(tps, fns, fps, tns)
        self.generate_operator_primitive_metrics(tps, fns, fps, tns)
        print('Generated other metric reports. Finished!')

    def gen_barplot(self, stats, identifier):
        without_label = stats.drop('label', axis=1)
        data = np.array(without_label, dtype=float)

        # Filter data using np.isnan
        mask = ~np.isnan(data)
        filtered_data = [d[m] for d, m in zip(data.T, mask.T)]

        ticks = range(1, len(without_label.columns)+1)
        labels = list(without_label.columns)
        plt.rc('font', size=20, family='Bitstream Vera Sans')

        plt.rc('text', usetex=True)
        plt.boxplot(filtered_data)
        plt.xticks(ticks, labels)
        plt.ylim((-0.025, 1.025))

        plt.savefig(f'{os.path.basename(self.interface.name)}_{identifier}_metrics.pdf')
        plt.clf()

    def gen_latex(self, starts):
        starts = starts[['label', 'precision', 'recall', 'accuracy', '$F_1$', '$F_2$', '$F_{0.5}$']]
        print(starts.round(2).replace(np.NaN, '-').to_latex(index=False, escape=False))

    def generate_generic_metrics(self, tps, fns, fps, tns):
        def args(program):
            return {}

        generic_stats = self.get_stats(
            tps, fns, fps, tns,
            iterate_over=[1],
            get_mutants_args=args,
        )
        print(generic_stats)
        for i, row in generic_stats.iterrows():
            generic_stats.loc[i, 'label'] = str(os.path.basename(self.interface.name))
        self.gen_latex(generic_stats)
        # self.gen_barplot(generic_stats, 'all')

    def generate_program_metrics(self, tps, fns, fps, tns):
        def args(program):
            return {
                'program': program
            }

        program_stats = self.get_stats(
            tps, fns, fps, tns,
            iterate_over=self.get_programs(),
            get_mutants_args=args,
        )
        for i, row in program_stats.iterrows():
            program_stats.loc[i, 'label'] = row['label'].split('#')[1].split('.')[0]
        self.gen_latex(program_stats)
        self.gen_barplot(program_stats, 'program')

    def generate_operator_metrics(self, tps, fns, fps, tns):
        def args(operator):
            return {
                'operators': [operator]
            }

        operator_stats = self.get_stats(
            tps, fns, fps, tns,
            iterate_over=self.get_operators(),
            get_mutants_args=args,
        )
        for i, row in operator_stats.iterrows():
            operator_stats.loc[i, 'label'] = row['label'].split('#')[1]
        self.gen_latex(operator_stats)
        self.gen_barplot(operator_stats, 'operators')

    def generate_operator_action_metrics(self, tps, fns, fps, tns):
        def args(operator):
            return {
                'operators': [operator]
            }

        operators = list(self.get_operators())
        operator_stats = self.get_stats(
            tps, fns, fps, tns,
            iterate_over=operators,
            get_mutants_args=args,
        )
        self.gen_barplot(operator_stats, 'operators')

        action_grouped = {
            action: set(self.rdf.graph.subjects(MB.operatorAction, action)).intersection(set(operators))
            for action in [
                MB['InsertionOperator'],
                MB['ReplacementOperator'],
                MB['DeletionOperator'],
            ]
        }

        for action, action_operators in action_grouped.items():
            action_stats = operator_stats[operator_stats.label.isin(action_operators)]

            self.gen_barplot(action_stats, action.split('/')[-1])

    def generate_operator_class_metrics(self, tps, fns, fps, tns):
        def args(operator):
            return {
                'operators': [operator]
            }

        operators = list(self.get_operators())
        operator_stats = self.get_stats(
            tps, fns, fps, tns,
            iterate_over=operators,
            get_mutants_args=args,
        )
        self.gen_barplot(operator_stats, 'operators')

        class_grouped = {
            class_: set(self.rdf.graph.subjects(MB.operatorClass, class_)).intersection(set(operators))
            for class_ in [
                MB['CoincidentalCorrectness'],
                MB['PredicateAnalysis'],
                MB['StatementAnalysis'],
            ]
        }

        for class_, class_operators in class_grouped.items():
            class_stats = operator_stats[operator_stats.label.isin(class_operators)]

            self.gen_barplot(class_stats, class_.split('/')[-1])

    def generate_operator_primitive_metrics(self, tps, fns, fps, tns):
        def args(operator):
            return {
                'operators': [operator]
            }

        operators = list(self.get_operators())
        operator_stats = self.get_stats(
            tps, fns, fps, tns,
            iterate_over=operators,
            get_mutants_args=args,
        )

        primitive_grouped = {
            primitive: set(self.rdf.graph.subjects(MB.primitiveOperator, primitive)).intersection(set(operators))
            for primitive in [
                MB['Arithmetic'],
                MB['Relational'],
                MB['ShortCircuitEvaluation'],
                MB['Shift'],
                MB['Logical'],
                MB['ShortcutAssignment'],
                MB['ShortcutAssignment'],
                MB['Statement'],
                MB['Variable'],
                MB['Constant'],
            ]
        }

        for primitive, primitive_operators in primitive_grouped.items():
            primitive_stats = operator_stats[operator_stats.label.isin(primitive_operators)]

            self.gen_barplot(primitive_stats, primitive.split('/')[-1])

    def get_stats(self, tps, fns, fps, tns, iterate_over=[None], get_mutants_args=None, print_=False):
        stats = []

        for elem in iterate_over:
            args = get_mutants_args(elem) if elem else {}
            relevant_elements = set(self.get_mutants(equivalencies=[False], **args))
            non_relevant_elements = set(self.get_mutants(equivalencies=[True], **args))

            metrics = self.get_metrics(
                len(tps.intersection(relevant_elements)),
                len(fns.intersection(relevant_elements)),
                len(fps.intersection(non_relevant_elements)),
                len(tns.intersection(non_relevant_elements)),
                print_=print_,
            )
            stats.append(metrics)
            stats[-1]['label'] = elem

        return pd.DataFrame.from_dict(stats)

    def get_metrics(self, tp, fn, fp, tn, print_=True):
        selected = tp + fp
        relevant = tp + fn
        unrelevant = tn + fp
        correct = tp + tn
        total = tp + tn + fp + fn
        precision = calc_precision(tp, selected)
        recall = calc_recall(tp, relevant)
        accuracy = calc_accuracy(correct, total)
        F1 = calc_Fb(recall or 0, precision or 0, beta=1)
        F2 = calc_Fb(recall or 0, precision or 0, beta=2)
        F0_5 = calc_Fb(recall or 0, precision or 0, beta=0.5)
        if print_:
            print(f'Contains: {relevant} relevant, {unrelevant} unrelevant')
            print(f'Found: {tp} true positives, {fp} false positives')
            print('Precision:', precision)
            print('Recall:', recall)
            print('Fb (1,2,0.5):', F1, F2, F0_5)
            print('Accuracy:', accuracy)
        return {
            'precision': precision,
            'recall': recall,
            'accuracy': accuracy,
            '$F_1$': F1,
            '$F_2$': F2,
            '$F_{0.5}$': F0_5,
        }

    def generate_program(self, program):
        if not os.path.exists(f'{self.out_dir}/{self.get_program_name(program)}'):
            pathlib.Path(f'{self.out_dir}/{self.get_program_name(program)}/mutants')\
                .mkdir(parents=True, exist_ok=True)

        copyfile(download_program(program, self.rdf), self.get_program_location(program))

    def generate_mutant(self, mutant):
        directory = self.get_mutant_path(mutant)
        mutant_file_name = f'{mutant.split("#")[1]}.{self.rdf.get_from(self.rdf.get_from(mutant, "program"), "extension")}'
        mutant_file_location = f'{directory}/{mutant_file_name}'

        copyfile(download_program(self.rdf.get_from(mutant, 'program'), self.rdf), mutant_file_location)
        patch_mutant(self.rdf.get_from(mutant, 'difference'), mutant_file_location)

    def generate_test_dataset(self):
        if self.do_not_gen_dataset:
            print('Skipping dataset generation.')
            return
        print('Generating dataset..')

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

