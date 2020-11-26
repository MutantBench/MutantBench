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
        'language': {
            'nargs': 1,
            'type': str,
            'choices': ['java', 'c'],
            'help': 'the language the interface is in',
        },
        '--mutants -m': {
            'nargs': '*',
            'type': str,
            'help': 'the mutants you would like to test (supports regex)',
            'default': '*',
        },
        '--RIP': {
            'nargs': '?',
            'type': str,
            'choices': ['weak', 'strong'],
            'help': 'only test the [RIP] mutants',
        },
        '--operators': {
            'nargs': '?',
            'type': str,
            'choices': ['ABS', 'AOR', 'LCR', 'ROR', 'UOI'],
            'help': 'only test the specified operators',
        },
        '--only-equivalent': {
            'nargs': '?',
            'type': lambda b: b == 'true',
            'choices': ['true', 'false'],
            'default': 'false',
            'help': 'only test the equivalent mutants'
        },
    }
    for args, kwargs in arguments.items():
        parser.add_argument(*args.split(), **kwargs)
    return parser


if __name__ == '__main__':
    args = get_argument_parser().parse_args(sys.argv[1:])
    benchmark = Benchmark(args.mutants, args.language[0], args.interface[0])
