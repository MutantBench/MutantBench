from sqlalchemy.sql.expression import func
from mutantbench import session, db


for mutant in session.query(db.Mutant).filter(
    func.length(db.Mutant.diff) > 300
).order_by(db.Mutant.old_path):
    if all(n not in mutant.old_path for n in ['Tcas', 'Hashmap/2.2', 'Replace/2.2', 'Replace/3.1-UOI']):
        print('\n\n\n\n\n')
        print(mutant.old_path)
        print(mutant.diff)
