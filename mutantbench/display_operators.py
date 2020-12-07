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
    bins = []
    totals = []
    equivs = []
    labels = []
    for operator in session.query(db.Operator):
        bins.append(operator.name)
        labels.append('$' + ' '.join(operator.description.split('\n')) + '$')

        totals.append((session.query(db.Mutant).filter(
            db.Mutant.operators.contains(operator)).count()
        ))

        equivs.append((session.query(db.Mutant).filter(
            db.Mutant.operators.contains(operator),
            db.Mutant.equivalent
        ).count()))

    plt.bar(bins, totals, label='total')
    plt.bar(bins, equivs, label='equivalent')

    # plt.legend(prop={'family': 'monospace'})
    plt.legend()
    plt.show()


if __name__ == '__main__':
    main()
