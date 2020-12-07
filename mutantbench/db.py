import enum
from sqlalchemy import create_engine, ForeignKey, Column, \
    Integer, String, Enum, Table, Boolean
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
    constant = 'Constant'
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
    long_name = Column(String, unique=True)
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
    equivalent = Column(Boolean)


if __name__ == '__main__':
    # create tables
    Base.metadata.create_all(engine)

    Session = sessionmaker()
    Session.configure(bind=engine)
    session = Session()
    # v: variable (can be constant)
    # c: constant
    # op: operator
    operators = [
        Operator(
            long_name='Absolute operator insertion',
            name='ABSI',
            description='\\{(v,abs(v)),(v,-abs(v))\\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator replacement binary',
            name='AORB',
            description='\\{(op_1,op_2) \\mid op_1, op_2 \\in \\{+,-,*,/,\\%\\} \\land op_1 \\neq op_2\\}',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.predicate_analysis,
        ),
        Operator(
            long_name='Arithmetic operator replacement short-cut',
            name='AORS',
            description='\\{(op_1,op_2) \\mid op_1, op_2 \\in \\{++,--\\} \\land op_1 \\neq op_2\\}',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.predicate_analysis,
        ),
        Operator(
            long_name='Arithmetic operator insertion unary',
            name='AOIU',
            description='\\{(v,-v),(v,+v)\\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator insertion short-cut',
            name='AOIS',
            description='\\{(v,--v),(v,v--),(v,++v),(v,v++)\\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator deletion unary',
            name='AODU',
            description='\\{(-v,v),(+v,v)\\}',
            operation=Operation.deletion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator deletion short-cut',
            name='AODS',
            description='\\{(--v,v),(v--,v),(++v,v),(v++,v)\\}',
            operation=Operation.deletion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Relational operator replacement',
            name='ROR',
            description='''\\{
                (a op b), false),
                (a op b), true),
                (op_1, op_2) \\mid op_1, op_2 \in \\{>, >=, <, <=, ==, != \\}, \\land op_1 \\neq op_2
            \\}''',
            operation=Operation.replacement,
            type=Type.relational,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Conditional operator replacement',
            name='COR',
            description='''\\{(op_1, op_2) \\mid op_1, op_2 \\in \\{\\&\\&, ||, \\^\\}, \\land op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Conditional operator deletion',
            name='COD',
            description='''\\{(!cond, cond)\\}''',
            operation=Operation.deletion,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Conditional operator insertion',
            name='COI',
            description='''\\{(cond, !cond)\\}''',
            operation=Operation.insertion,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Shift operator replacement',
            name='SOR',
            description='''\\{(op_1, op_2) \\mid op_1, op_2 \\in \\{>>,>>>,<<}, \\land op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.shift,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Logical operator replacement',
            name='LOR',
            description='''
                \\{(op_1, op_2) \\mid
                op_1, op_2 \\in \\{\\&, |, \\^\\}, \\land
                op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Logical operator insertion',
            name='LOI',
            description='''\\{(v, ~v)\\}''',
            operation=Operation.insertion,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Logical operator deletion',
            name='LOD',
            description='''\\{(~v, v)\\}''',
            operation=Operation.deletion,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Short-cut assignment operator replacement',
            name='ASRS',
            description='''\\{(op_1, op_2) \\mid op_1, op_2 \\in \\{
                +=,-=,*=,/=,\\%=,\\&=,\\^=,>>=,>>>=,<<=\\}, \\land op_1 \\neq op_2\\}''',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Constant deletion',
            name='CDL',
            description='''
                \\{
                    (v op c,v) \\mid
                    (c op v,v) \\mid
                    op \\in \\{+,-,*,/,\\%\\,>,>=,<,<=,==,!=\\}
                \\}''',
            operation=Operation.replacement,
            type=Type.constant,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Variable with relational operator deletion',
            name='VROD',
            description='''
                \\{
                    (v_1 op v_2, v_1) \\mid
                    (v_2 op v_1, v_1) \\mid
                    op \\in \\{>,>=,<,<=,==,!=\\}
                \\}''',
            operation=Operation.deletion,
            type=Type.relational,  # TODO change to variable with operator?
            clss=Class.TODO,
        ),
        Operator(
            long_name='Variable with arithmetic operator deletion',
            name='VAOD',
            description='''
                \\{
                    (v_1 op v_2, v_1) \\mid
                    (v_2 op v_1, v_1) \\mid
                    op \\in \\{+,-,*,/,\\%\\}
                \\}''',
            operation=Operation.deletion,
            type=Type.arithmetic,  # TODO change to variable with operator?
            clss=Class.TODO,
        ),
        Operator(
            long_name='Variable with conditional operator deletion',
            name='VCOD',
            description='''
                \\{
                    (v_1 op v_2, v_1) \\mid
                    (v_2 op v_1, v_1) \\mid
                    op \\in \\{\\&\\&, ||, \\^\\}
                \\}''',
            operation=Operation.deletion,
            type=Type.conditional,  # TODO change to variable with operator?
            clss=Class.TODO,
        ),
        Operator(
            long_name='Statement deletion',
            name='STMTD',
            description='''
                \\{
                    (s,)
                \\}''',
            operation=Operation.deletion,
            type=Type.statement,
            clss=Class.TODO,
        )
    ]
    for operator in operators:
        session.add(operator)
    session.commit()
