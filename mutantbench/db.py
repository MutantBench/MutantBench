import enum
from sqlalchemy import create_engine
from sqlalchemy import Column, Integer, String, Enum
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, backref

engine = create_engine('sqlite:///mutants.db', echo=True)
Base = declarative_base()


class Languages(enum.Enum):
    java = 'Java'
    c = 'C'


class Type(enum.Enum):
    arithmetic = 'Arithmetic'
    logical = 'Logical'
    conditional = 'Conditional'


class Operation(enum.Enum):
    insertion = 'Insertion'
    replacement = 'Replacement'
    deletion = 'Deletion'


class Program(Base):
    __tablename__ = "program"

    id = Column(Integer, primary_key=True)
    language = Column(Enum(Languages))
    source_code = Column(String)

    def __init__(self, language, source_code):
        self.language = language
        self.source_code = source_code


class Mutant(Base):
    __tablename__ = "mutant"

    id = Column(Integer, primary_key=True)
    diff = Column(String)
    operator = relationship(
        'Operator', backref=(backref('mutants', lazy=True)))


class Operator(Base):
    __tablename__ = "operator"

    id = Column(Integer, primary_key=True)
    operator = Column(String)
    operation = Column(Enum(Operation))
    type = Column(Enum(Type))


# create tables
Base.metadata.create_all(engine)
