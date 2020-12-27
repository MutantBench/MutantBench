from sqlalchemy import asc
from matplotlib import pyplot as plt
from mutantbench import db, session


def operators():
    bins = []
    totals = []
    equivs = []
    labels = []
    for operator in session.query(db.Operator).order_by(db.Operator.name):
        bins.append(operator.name)
        labels.append('$' + ' '.join(operator.description.split('\n')) + '$')

        totals.append(
            session.query(db.Mutant)
            .filter(db.Mutant.operators.contains(operator))
            .distinct()
            .count()
        )

        equivs.append(
            session.query(db.Mutant)
            .filter(
                db.Mutant.operators.contains(operator),
                db.Mutant.equivalent
            )
            .distinct()
            .count()
        )
    print('\\hline')
    print('Operator & Nr. of non-equivalent & Nr. of equivalent \\\\')
    print('\\hline')
    for name, total, equiv in zip(bins, totals, equivs):
        print(f'{name.replace("_", " ")} & {total - equiv} & {equiv}  \\\\ ')
    print('\\hline')
    print()
    print()
    print()


    # plt.bar(bins, totals, label='total', orientation=u'vertical', log="y")
    # plt.bar(bins, equivs, label='equivalent', orientation=u'vertical', log="y")

    # ticks_and_labels = plt.xticks(range(len(bins)), bins, rotation=0)
    # for i, label in enumerate(ticks_and_labels[1]):
    #     label.set_y(label.get_position()[1] - (i % 2) * 0.05)

    # # plt.legend(prop={'family': 'monospace'})
    # plt.legend()
    # plt.savefig('combined_dataset_operator_stats.pdf')


def programs():
    bins = []
    mutant_count = []
    equiv_count = []
    program_size = []
    language = []
    labels = []
    for program in session.query(db.Program).filter(db.Program.language == 'c').order_by(asc(db.Program.file_name)):
        bins.append(program.name)
        mutant_count.append(session.query(db.Mutant).filter(
            db.Mutant.program == program).count()
        )
        equiv_count.append(session.query(db.Mutant).filter(
            db.Mutant.program == program, db.Mutant.equivalent).count())
        program_size.append(len(open(program.path, 'r').readlines()))
        language.append(program.language)

    print('\\hline')
    print('\\multicolumn{4}{l}{\\textit{C}} \\\\ \\hline')
    for name, mutants, equiv, size in zip(bins, mutant_count, equiv_count, program_size):
        print(f'{name.replace("_", " ")} & {mutants} & {round(equiv / mutants * 100, 2)} & {size}  \\\\ ')
    bins = []
    mutant_count = []
    equiv_count = []
    program_size = []
    language = []
    labels = []
    for program in session.query(db.Program).filter(db.Program.language == 'java').order_by(asc(db.Program.file_name)):
        bins.append(program.name)
        mutant_count.append(session.query(db.Mutant).filter(
            db.Mutant.program == program).count()
        )
        equiv_count.append(session.query(db.Mutant).filter(
            db.Mutant.program == program, db.Mutant.equivalent).count())
        program_size.append(len(open(program.path, 'r').readlines()))
        language.append(program.language)

    print('\\hline')
    print('\\multicolumn{4}{l}{\\textit{Java}} \\\\ \\hline')
    for name, mutants, equiv, size in zip(bins, mutant_count, equiv_count, program_size):
        print(f'{name.replace("_", " ")} & {mutants} & {round(equiv / mutants * 100, 2)} & {size}  \\\\ ')
    print('\\hline')
    print(f' Total (C \\& Java) & {session.query(db.Mutant).count()} & {round(session.query(db.Mutant).filter(db.Mutant.equivalent).count() / session.query(db.Mutant).count() * 100, 2)} &   \\\\ ')
    print('\\hline')




if __name__ == '__main__':
    operators()
    programs()
