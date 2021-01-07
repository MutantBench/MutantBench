from __future__ import absolute_import, unicode_literals

from sqlalchemy.orm import sessionmaker
from sqlalchemy import create_engine

engine = create_engine(
    'mysql+pymysql://root:pass@localhost/mutantbench',
    # connect_args={'check_same_thread': False}
)

Session = sessionmaker()
Session.configure(bind=engine)
session = Session()
