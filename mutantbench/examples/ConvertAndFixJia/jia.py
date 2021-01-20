from translate import TranslateDataset
import os


class TranslateYJiaC(TranslateDataset):
    def line_has_mutation_comment(self, line):
        return any(
            m in line for m in [
                '//mutated statement',
                '//mutated statemen',
                '//mutated statment',
                '// mutated statement',
            ]
        )

    def check_output(self, output, program_location, mutant_location):
        mins, plusses, mutated = 0, 0, 0

        for line in output.split('\n'):
            if line.startswith('-'):
                mins += 1
            if line.startswith('+'):
                plusses += 1
                if self.line_has_mutation_comment(line):
                    mutated += 1
        if plusses != mutated:
            return False

        if mins != plusses:
            return False

        return True

    def fix_output(self, output, program_location, mutant_location):
        split_output = output.split('\n')
        from_ = -1
        to_ = -1
        for i, line in enumerate(split_output):
            if line.startswith('@@'):
                from_ = i
            elif '//mutated statement' in line:
                to_ = i + 1
                break

        if from_ != -1 and to_ != -1:
            split_output = split_output[from_:to_]
            return '\n'.join(split_output)

    def get_program_locations(self):
        return [
            self.directory + '/original/' + file_name
            for file_name in os.listdir(self.directory + '/original')
            if file_name.endswith('.c')
        ]

    def get_mutant_locations(self):
        program_mutants = {}

        for root, _, files in os.walk(self.directory, topdown=False):
            if 'original' in root:
                continue

            for name in files:
                if not name.endswith('.c'):
                    continue

                program_name = name.split('_')[0].split('-')[-1]
                program = self.get_program_from_name(program_name)

                if program not in program_mutants:
                    program_mutants[program] = []
                program_mutants[program].append((root + '/' + name, True))

        return program_mutants


def main():
    jia_c = TranslateYJiaC(
        language='c',
        source='yao2015study',
        directory='/path/to/kintis/dataset/',
        out_dir='/path/to/program/storage/'
    )
    jia_c.gen_programs()
    jia_c.gen_mutants()


if __name__ == '__main__':
    main()
