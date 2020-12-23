import enum
from sqlalchemy import create_engine, ForeignKey, Column, \
    Integer, String, Enum, Table, Boolean
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, sessionmaker
from sympy import preview
from mutantbench import session, engine

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
    arithmetic = 'arithmetic'
    relational = 'relational'
    conditional = 'conditional'
    shift = 'shift'
    logical = 'logical'
    statement = 'statement'
    constant = 'constant'
    unary = 'unary'
    variable = 'variable'
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

    @property
    def name(self):
        # Remove extension from filename
        return '.'.join(self.file_name.split('.')[:-1])

    @property
    def extension(self):
        return self.file_name.split('.')[-1]


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
    old_path = Column(String)


if __name__ == '__main__':
    # create tables
    Base.metadata.create_all(engine)

    # v: variable (can be constant)
    # c: constant
    # op: operator
    operators = [
        Operator(
            long_name='Absolute operator insertion',
            name='ABSI',
            description=r'\{(v,abs(v)),(v,-abs(v))\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator replacement binary',
            name='AORB',
            description=r'\{(op_1,op_2) \mid op_1, op_2 \in \{+,-,*,/,\%\} \wedge op_1 \neq op_2\}',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.predicate_analysis,
        ),
        Operator(
            long_name='Arithmetic operator replacement short-cut',
            name='AORS',
            description=r'\{(op_1,op_2) \mid op_1, op_2 \in \{++,--\} \wedge op_1 \neq op_2\}',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.predicate_analysis,
        ),
        Operator(
            long_name='Arithmetic operator insertion unary',
            name='AOIU',
            description=r'\{(v,-v),(v,+v)\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator insertion short-cut',
            name='AOIS',
            description=r'\{(v,--v),(v,v--),(v,++v),(v,v++)\}',
            operation=Operation.insertion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator deletion unary',
            name='AODU',
            description=r'\{(-v,v),(+v,v)\}',
            operation=Operation.deletion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Arithmetic operator deletion short-cut',
            name='AODS',
            description=r'\{(--v,v),(v--,v),(++v,v),(v++,v)\}',
            operation=Operation.deletion,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Relational operator replacement',
            name='ROR',
            description=r'''\{
                (op_1, op_2) \mid op_1, op_2 \in \{>, >=, <, <=, ==, != \} \wedge op_1 \neq op_2
            \}''',
            operation=Operation.replacement,
            type=Type.relational,
            clss=Class.TODO,
        ),
        # TODO fix regex
        Operator(
            long_name='Relational operator deletion',
            name='ROD',
            description=r'''\{
                ((e\text{ }op\text{ }e), false),
                ((e\text{ }op\text{ }e), true),
                (e\text{ }op_1\text{ }e, e \text{ } e) \mid op \in \{>, >=, <, <=, ==, != \}
            \}''',
            operation=Operation.deletion,
            type=Type.relational,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Conditional operator replacement',
            name='COR',
            description=r'''\{(op_1, op_2) \mid op_1, op_2 \in \{\&\&, || \} \wedge op_1 \neq op_2\}''',
            operation=Operation.replacement,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        # TODO: expand regex to include infix expressions
        Operator(
            long_name='Conditional operator deletion',
            name='COD',
            description=r'''\{(!e, e), \{(e_1\text{ }op\text{ }e_2, e_1), \{(e_1\text{ }op\text{ }e_2, e_2) \mid op \in \{\&\&, || \}\}''',
            operation=Operation.deletion,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Conditional operator insertion',
            name='COI',
            description=r'''\{(cond, !cond)\}''',
            operation=Operation.insertion,
            type=Type.conditional,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Shift operator replacement',
            name='SOR',
            description=r'''\{(op_1, op_2) \mid op_1, op_2 \in \{>>,>>>,<<\} \wedge op_1 \neq op_2\}''',
            operation=Operation.replacement,
            type=Type.shift,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Logical operator replacement',
            name='LOR',
            description=r'''
                \{(op_1, op_2) \mid
                op_1, op_2 \in \{\&, |, \wedge \} \wedge
                op_1 \neq op_2\}''',
            operation=Operation.replacement,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Logical operator insertion',
            name='LOI',
            description=r'''\{(v, \sim v)\}''',
            operation=Operation.insertion,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Logical operator deletion',
            name='LOD',
            description=r'''\{(\sim v, v)\}''',
            operation=Operation.deletion,
            type=Type.logical,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Short-cut assignment operator replacement',
            name='ASRS',
            description=r'''\{(op_1, op_2) \mid op_1, op_2 \in \{
                +=,-=,*=,/=,\%=,\&=,\wedge =,>>=,>>>=,<<=\} \wedge op_1 \neq op_2\}''',
            operation=Operation.replacement,
            type=Type.arithmetic,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Constant deletion',
            name='CDL',
            description=r'''
                \{
                    (v\text{ }op\text{ }c,v),
                    (c\text{ }op\text{ }v,v) \mid
                    op \in \{+,-,*,/,\%\,>,>=,<,<=,==,!=\}
                \}''',
            operation=Operation.replacement,
            type=Type.constant,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Variable deletion',
            name='VDL',
            description=r'''
                \{
                    (v_1\text{ }op\text{ }v_2,v_1),
                    (v_2\text{ }op\text{ }v_1,v_2) \mid
                    op \in \{+,-,*,/,\%\,>,>=,<,<=,==,!=\}
                \}''',
            operation=Operation.replacement,
            type=Type.variable,
            clss=Class.TODO,
        ),
        Operator(
            long_name='Statement deletion',
            name='SDL',
            description=r'''
                \{
                    (s,)
                \}''',
            operation=Operation.deletion,
            type=Type.statement,
            clss=Class.TODO,
        )
    ]
    for operator in operators:
        operator.description = '\\\\'.join(operator.description.split('\n'))
        print(operator.name)
        preview(
            '\[' + operator.description + '\]',
            viewer='file',
            filename=f'visualizer/static/{operator.name}.png',
            euler=False,
        )
        session.add(operator)
    session.commit()
