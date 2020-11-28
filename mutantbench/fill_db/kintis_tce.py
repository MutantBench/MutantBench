
import sys
import os
from sqlalchemy.orm import sessionmaker
from sqlalchemy import create_engine
from mutantbench import db


def main(directory):
    print(directory)
    engine = create_engine('sqlite:///mutants.db', echo=True)
    Session = sessionmaker()
    Session.configure(bind=engine)
    session = Session()

    for project_name in os.listdir(directory):
        project_dir = directory + '/' + project_name
        mutants = []
        if os.path.isdir(project_dir) and \
           not project_name.startswith('.'):
            for mutant in os.listdir(project_dir):
                # Mutants have an _ in the name to indicate the type
                if '_' not in mutant:
                    program = db.Program(
                        language=db.Languages.java,
                        source_code='test',  # TODO
                        source='kintis'
                    )
                    continue
                operator = session.query(
                    db.Operator).first()
                print(operator, mutants)
                mutants.append(db.Mutant(
                    diff='a',  # TODO
                    operators=[operator],  # TODO
                ))
        session.add(program)
        for mutant in mutants:
            mutant.program = program
            session.add(mutant)
        session.commit()


if __name__ == '__main__':
    main(sys.argv[1])
