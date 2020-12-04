from matplotlib import pyplot as plt
import re
import sys
import os
from sqlalchemy.orm import sessionmaker, Query
from sqlalchemy import create_engine
from mutantbench import db
from sqlalchemy import func


class BaseQuery(Query):
    def count_star(self):
        count_query = self.statement.with_only_columns([func.count()]).order_by(None)
        return self.session.execute(count_query).scalar()


def main():
    engine = create_engine('sqlite:///mutants.db', echo=False)
    Session = sessionmaker()
    Session.configure(bind=engine)
    session = Session()
    for operator in session.query(db.Operator):
        print(operator.name)
        print(session.query(db.Mutant).filter(db.Mutant.operators.contains(operator)).count())



if __name__ == '__main__':
    main()
