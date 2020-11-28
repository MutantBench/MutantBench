import enum
from sqlalchemy import create_engine, ForeignKey, Column, \
    Integer, String, Enum, Table
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, sessionmaker

engine = create_engine('sqlite:///mutants.db', echo=True)
Base = declarative_base()


association_table = Table(
    'mutant_operators', Base.metadata,
    Column('operator_id', Integer, ForeignKey('operator.id')),
    Column('mutant_id', Integer, ForeignKey('mutant.id'))
)


class Languages(enum.Enum):
    java = 'Java'
    c = 'C'


class Type(enum.Enum):
    arithmetic = 'Ari'
    logical = 'Log'
    conditional = 'Con'
    relational = 'Rel'
    statement = 'Sta'
    literal = 'Lit'


class Operation(enum.Enum):
    insertion = '+'
    replacement = '~'
    deletion = '-'


class Program(Base):
    __tablename__ = 'program'

    id = Column(Integer, primary_key=True)
    language = Column(Enum(Languages))
    source_code = Column(String)
    source = Column(String)
    mutants = relationship('Mutant', back_populates='program')


class Operator(Base):
    __tablename__ = 'operator'

    id = Column(Integer, primary_key=True)
    operator = Column(String)
    operation = Column(Enum(Operation))
    type = Column(Enum(Type))
    mutants = relationship(
        'Mutant',
        secondary=association_table,
        back_populates='operators',
    )


class Mutant(Base):
    __tablename__ = 'mutant'

    id = Column(Integer, primary_key=True)
    diff = Column(String)
    # operator_id = Column(Integer, ForeignKey('operator.id'))
    operators = relationship(
        'Operator',
        secondary=association_table,
        back_populates='mutants',
    )
    program_id = Column(Integer, ForeignKey('program.id'))
    program = relationship('Program', back_populates='mutants')


if __name__ == '__main__':
    # create tables
    Base.metadata.create_all(engine)

    Session = sessionmaker()
    Session.configure(bind=engine)
    session = Session()
    for operation in Operation:
        for t in Type:
            if not session.query(Operator).filter_by(operator=operation, type=t).first():
                session.add(Operator(
                    operation=operation,
                    type=t,
                ))
    session.commit()
