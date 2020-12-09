from matplotlib import pyplot as plt
from mutantbench import db, session


def main():
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
