from sqlalchemy.orm import sessionmaker
from sqlalchemy import create_engine


engine = create_engine(
    'sqlite:////home/polo/thesis/MutantBench/mutantbench/mutants.db',
    connect_args={'check_same_thread': False}
)
Session = sessionmaker()
Session.configure(bind=engine)
session = Session()
