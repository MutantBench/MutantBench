import enum
from sqlalchemy import create_engine, ForeignKey, Column, \
    Integer, String, Enum, Table
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, sessionmaker

engine = create_engine('sqlite:///mutants.db', echo=False)
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
    relational = 'Rel'
    conditional = 'Con'
    shift = 'Shi'
    logical = 'Log'
    statement = 'Sta'
    literal = 'Lit'
    unary = 'Una'
    TODO = 'TODO'


class Operation(enum.Enum):
    insertion = '⌦'
    replacement = '±'
    deletion = '⎀'
    TODO = 'TODO'


class Class(enum.Enum):
    stmt_analysis = 'SAL'
    predicate_analysis = 'PDA'
    coincidental_correctness = 'CCA'
    TODO = 'TODO'


class Program(Base):
    __tablename__ = 'program'

    id = Column(Integer, primary_key=True)
    language = Column(Enum(Languages))
    path = Column(String)
    source = Column(String)
    file_name = Column(String)
    mutants = relationship('Mutant', back_populates='program')


class Operator(Base):
    __tablename__ = 'operator'

    id = Column(Integer, primary_key=True)
    name = Column(String, unique=True)
    description = Column(String, unique=True)
    operator = Column(String)
    operation = Column(Enum(Operation))
    clss = Column(Enum(Class))
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
    operators = [
        Operator(
            name='ABSI',
            description='\\{(v,abs(v)),(v,-abs(v))\\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            name='AORB',
            description='\\{(op_1,op_2) \\mid op_1, op_2 \\in \\{+,-,*,/,%\\} \\land op_1 \\neq op_2\\}',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.predicate_analysis,
        ),
        Operator(
            name='AORS',
            description='\\{(op_1,op_2) \\mid op_1, op_2 \\in \\{++,--\\} \\land op_1 \\neq op_2\\}',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.predicate_analysis,
        ),
        Operator(
            name='AOIU',
            description='\\{(v,-v),(v,+v)\\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            name='AOIS',
            description='\\{(v,--v),(v,v--),(v,++v),(v,v++)\\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            name='AODU',
            description='\\{(-v,v),(+v,v)\\}',
            operation=Operation.deletion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            name='AODS',
            description='\\{(--v,v),(v--,v),(++v,v),(v++,v)\\}',
            operation=Operation.deletion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            name='ROR',
            description='''\\{
                (a op b), false),
                (a op b), false),
                (op_1, op_2) \mid op_1, op_2 \in \\{>, >=, <, <=, ==, != \\}, \\land op_1 \\neq op_2
            \\}''',
            operation=Operation.replacement,
            type=Type.relational,
            clss=Class.TODO,
        ),
        Operator(
            name='COR',
            description='''\\{(op_1, op_2) \mid op_1, op_2 \in \\{\\&\\&, ||, \^\\}, \\land op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            name='COD',
            description='''\\{(!cond, cond)\\}''',
            operation=Operation.deletion,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            name='COI',
            description='''\\{(cond, !cond)\\}''',
            operation=Operation.insertion,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            name='SOR',
            description='''\\{(op_1, op_2) \mid op_1, op_2 \in \\{>>,>>>,<<}, \\land op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.shift,
            clss=Class.TODO,
        ),
        Operator(
            name='LOR',
            description='''\\{(op_1, op_2) \mid op_1, op_2 \in \\{\\&, |, \^\\}, \\land op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            name='LOI',
            description='''\\{(v, ~v)\\}''',
            operation=Operation.insertion,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            name='LOD',
            description='''\\{(~v, v)\\}''',
            operation=Operation.deletion,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            name='ASRS',
            description='''\\{(op_1, op_2) \mid op_1, op_2 \in \\{
                +=,-=,*=,/=,%=,&=,^=,>>=,>>>=,<<=\\}, \\land op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
    ]
    for operator in operators:
        session.add(operator)
    session.commit()
