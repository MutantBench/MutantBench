from benchmark import Benchmark
import sys
import argparse


def get_argument_parser():
    parser = argparse.ArgumentParser(
        description='Benchmarking tool for the Equivalent Mutant Problem')
    arguments = {
        'interface': {
            'nargs': 1,
            'type': str,
            'help': 'the interface file MutantBench should communicate with',
        },
        # TODO: change to "tool" or sth alike
        'interface_language': {
            'nargs': 1,
            'type': str,
            'choices': ['java', 'c', 'bash'],
            'help': 'the language the interface is in',
        },
        'interface_type': {
            'nargs': 1,
            'type': str,
            'choices': ['DEM', 'SEM', 'AEMG'],
            'help': 'the language the interface is in',
        },
        # TODO: rename to sth better
        'output': {
            'nargs': 1,
            'type': str,
            'help': 'the directory the mutants will be generated',
        },
        'language': {
            'nargs': 1,
            'type': str,
            'choices': ['java', 'c'],
            'help': 'the language of the mutants',  # TODO: supportn multi language
        },
        '--programs -p': {
            'nargs': '*',
            'type': str,
            'help': 'the program names you would like to test',
            'default': '',
        },
        # '--RIP': {
        #     'nargs': '?',
        #     'type': str,
        #     'choices': ['weak', 'strong'],
        #     'help': 'only test the [RIP] mutants',
        # },
        '--operators': {
            'nargs': '?',
            'type': str,
            'choices': ['ABS', 'AOR', 'LCR', 'ROR', 'UOI'],
            'help': 'only test the specified operators',
        },
        '--equivalency': {
            'nargs': '*',
            'type': str,
            'choices': ['non_equivalent', 'equivalent', 'unknown', 'all'],
            'default': 'all',
            'help': 'generate equivalent mutants'
        },
        '--threshold': {
            'nargs': '?',
            'type': float,
            'default': .5,
            'help': 'Threshold when mutant should be considered equivalent'
        },
        '--do_not_gen_dataset': {
            'help': 'Do not (re)generate the dataset when benchmarking',
        }
    }
    for args, kwargs in arguments.items():
        parser.add_argument(*args.split(), **kwargs)
    return parser


def main():
    from mutantbench.fill_db import y_jia_c
    y_jia_c.main()


if __name__ == '__main__':
    args = get_argument_parser().parse_args(sys.argv[1:])
    equivalencies = None if 'all' in args.equivalency else set(
        {
            'non_equivalent': False,
            'equivalent': True,
            'unknown': None,
            'all': 'all',
        }[eq]
        for eq in args.equivalency
    )
    benchmark = Benchmark(
        programs=args.programs,
        interface_language=args.interface_language[0],
        interface=args.interface[0],
        output=args.output[0],
        language=args.language[0],
        equivalencies=equivalencies,
        operators=args.operators,
        type_=args.interface_type[0],
    )
    if not args.do_not_gen_dataset:
        benchmark.generate_test_dataset()
    benchmark.run()
