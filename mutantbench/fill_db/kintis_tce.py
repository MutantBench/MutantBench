import difflib
import re
import sys
import os
from sqlalchemy.orm import sessionmaker
from sqlalchemy import create_engine
from mutantbench import db


def get_aor(x):
    if any(i in x for i in ['++', '--']):
        return 'AORS'
    if 'abs' in x:
        return 'ABSI'
    if any(i in x for i in ['=', '>', '<', '!']):
        return 'ROR'
    return 'ABSI'


def get_operator(x):
    if '++' in x or '--' in x:
        return 'AORS'
    if 'abs' in x:
        return 'ABSI'
    if '=' in x:
        return 'AOIU'


operator_map = {
    'UOI': lambda x: 'AOIU',
    'ROR': lambda x: 'ROR',
    'ABS': lambda x: 'ABSI',
    'AOR': get_aor,
}


def get_edits_string(old, new):
    old = old.replace('//mutated statement', '')
    new = new.replace('//mutated statement', '')
    old = ' '.join(old.split())
    new = ' '.join(new.split())
    result = ""
    codes = difflib.SequenceMatcher(a=old, b=new).get_opcodes()
    for code in codes:
        o = old[code[1]:code[2]].strip()
        n = new[code[3]:code[4]].strip()
        if not o and not n or o == n:
            continue
        if code[0] == "delete":
            result += f'{db.Operation.deletion.value}» {o} «'
        elif code[0] == "insert":
            result += f'{db.Operation.insertion.value}» {n} «'
        elif code[0] == "replace":
            result += f'{db.Operation.replacement.value}» {o} ↦ {n} «'
    return result


def get_diff(program, mutant_source_code):
    a = get_edits_string(program.source_code, mutant_source_code)
    return a


def gen_mutant(file_name, project_dir, program):
    with open(f'{project_dir}/{file_name}', 'r') as f:
        source_code = f.read()

    diff = get_diff(program, source_code)

    operator_re = re.findall(r'\w*_([A-Z]+)\.java', file_name)
    # TODO make it based on content
    operator_name = operator_map[operator_re[0]](diff) if operator_re else None
    if operator_name:
        matching_operator = session.query(db.Operator).filter(db.Operator.name==operator_name).first()
        if matching_operator:
            operators = [matching_operator]
        else:
            print('NOTHING', operator_name)
    else:
        operators = []

    print(file_name, operator_name, diff)
    return db.Mutant(
        diff=diff,
        operators=operators,  # TODO
        program=program,
    )


def gen_program(file_name, project_dir):
    with open(f'{project_dir}/{file_name}', 'r') as f:
        source_code = f.read()
    return db.Program(
        language=db.Languages.java,
        source_code=source_code,
        source='kintis'
    )


def gen_program_with_mutants(project_dir):
    mutants = []
    program = None
    for file_name in os.listdir(project_dir):
        # Mutants have an _ in the name to indicate the type
        if re.match(r'\w+\.java', file_name):
            program = gen_program(file_name, project_dir)
            break
    else:
        print(f'No program was found in {project_dir}')
        return program, mutants

    for file_name in os.listdir(project_dir):
        if not re.match(r'\w+\.java', file_name):
            mutants.append(gen_mutant(file_name, project_dir, program))

    return program, mutants


def main(directory):
    for project_name in os.listdir(directory):
        project_dir = directory + '/' + project_name
        if os.path.isdir(project_dir) and not project_name.startswith('.'):
            program, mutants = gen_program_with_mutants(project_dir)
            session.add(program)
            session.add_all(mutants)
    session.commit()


if __name__ == '__main__':
    engine = create_engine('sqlite:///mutants.db', echo=False)
    Session = sessionmaker()
    Session.configure(bind=engine)
    session = Session()
    main(sys.argv[1])
