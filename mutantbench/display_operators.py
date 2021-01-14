from fill_db import rdf
from rdflib import Literal
import requests
import os


def download(url, out_path):
    if os.path.isfile(out_path):
        return out_path
    r = requests.get(url)
    with open(out_path, 'wb') as f:
        f.write(r.content)
    return out_path


def download_program(program):
    return download(
        mbrdf.get_from(program, 'codeRepository'),
        f'/home/polo/thesis/MutantBench/programs/{mbrdf.get_from(program, "fileName")}'
    )


def operators():
    table_data = []
    for operator in mbrdf.get_operators():
        table_data.append({
            'name': operator.split('#')[1],
            'all_count': len(list(mbrdf.get_mutants(operators=[operator]))),
            'equiv_count': len(list(mbrdf.get_mutants(operators=[operator], equivalencies=[True]))),
        })
    print('\\toprule')
    print('Operator & Nr. of mutants & \\% equivalent \\\\')
    print('\\midrule')
    for row_data in [r for r in sorted(table_data, key=lambda r: r['name'])]:
        print(f'{row_data["name"].replace("_", " ")} & {row_data["all_count"]} & {round(row_data["equiv_count"] / row_data["all_count"] * 100, 2) if row_data["all_count"] else 0.0} \\\\ ')

    print('\\bottomrule')
    print()
    print()
    print()


def programs():
    table_data = []
    for program in mbrdf.get_programs():
        table_data.append({
            'name': mbrdf.get_from(program, 'name'),
            'all_count': len(list(mbrdf.get_mutants(program=program))),
            'equiv_count': len(list(mbrdf.get_mutants(program=program, equivalencies=[True]))),
            'size': len(open(download_program(program), 'r').readlines()),
            'language': mbrdf.get_from(program, 'programmingLanguage'),
        })
    print(table_data)

    print('\\midrule')
    print('\\multicolumn{4}{l}{\\textit{C}} \\\\ \\midrule')
    for row_data in [r for r in sorted(table_data, key=lambda r: r['size']) if r['language'] == Literal('c')]:
        print(f'{row_data["name"].replace("_", " ")} & {row_data["all_count"]} & {round(row_data["equiv_count"] / row_data["all_count"] * 100, 2) if row_data["all_count"] else 0.0} & {row_data["size"]}  \\\\ ')

    print('\\midrule')
    print('\\multicolumn{4}{l}{\\textit{Java}} \\\\ \\midrule')
    for row_data in [r for r in sorted(table_data, key=lambda r: r['size']) if r['language'] == Literal('java')]:
        print(f'{row_data["name"].replace("_", " ")} & {row_data["all_count"]} & {round(row_data["equiv_count"] / row_data["all_count"] * 100, 2) if row_data["all_count"] else 0.0} & {row_data["size"]}  \\\\ ')
    print('\\midrule')
    print(
        f' Total (C \\& Java) & {len(list(mbrdf.get_mutants()))} & {round(len(list(mbrdf.get_mutants(equivalencies=[True]))) / len(list(mbrdf.get_mutants())) * 100, 2)} &  {sum([r["size"] for r in table_data])} (Avg. {int(sum([r["size"] for r in table_data]) / len(table_data))}) \\\\ ')
    print('\\bottomrule')

#(jia c) 1 + (jia c stubborn) 4


if __name__ == '__main__':
    mbrdf = rdf.MutantBenchRDF()
    # mbrdf.fix_up_operators()
    operators()
    programs()
