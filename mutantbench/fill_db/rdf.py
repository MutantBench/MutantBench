import hashlib
from sqlalchemy.sql.expression import func
from mutantbench import session, db
from rdflib import Graph, Literal, RDF, URIRef, RDFS
from rdflib.namespace import FOAF, XSD, Namespace
import rdflib
from collections import defaultdict
from mutantbench.db import Type, Languages, Operation, Class


SCHEMA = Namespace('http://schema.org/')


class MutantBenchRDF(object):
    def __init__(
        self,
        namespace_uri='https://github.com/Larspolo/MutantBench/tree/main/mutantbench/mutantbench.ttl',
        prefix='mb',
        load_from_file='mutants.ttl'
    ):
        self.prefix = prefix
        self.namespace_uri = namespace_uri
        self.from_file = load_from_file

        self.graph = rdflib.Graph()
        if self.from_file:
            self.graph.parse(self.from_file, format='turtle')
        self.namespace = Namespace(namespace_uri)
        self.graph.bind(self.prefix, self.namespace)
        self.graph.bind('schema', SCHEMA)
        self.session = session
        self.rdf_cache = defaultdict(dict)

    def get_from(self, subject, predicate):
        if not isinstance(predicate, URIRef):
            return self.get_from(subject, self.namespace[predicate]) or self.get_from(subject, SCHEMA[predicate])

        return self.graph.value(subject, predicate)

    def get_full_uri(self, name, type_):
        return URIRef(f'{self.prefix}:{type_}#{name}')

    def get_or_create(self, name, mb_type, type_, predicate_object_pairs=[]):
        cache_hash = hash((str(name), str(type_), str(mb_type)))

        if cache_hash in self.rdf_cache:
            # if mb_type:
            #     print(f'{mb_type}:{name} found in cache')
            # else:
            #     print(f'{uri_type}:{name} found in cache')

            return self.rdf_cache[cache_hash]

        uri = URIRef(f'{self.prefix}:{mb_type}#{name}')
        type_ = URIRef(type_)
        if (uri, None, None) in self.graph:
            self.rdf_cache[cache_hash] = uri
            # print(f'{mb_type}:{name} found in GRAPH')
            return uri
        import time
        time.sleep(0.1)
        self.graph.add((uri, RDF.type, type_))
        for p, o in predicate_object_pairs:
            self.graph.add((uri, p, o))

        self.rdf_cache[cache_hash] = uri

        return uri

    def get_db_mutants(self):
        return session.query(db.Mutant).all()

    def _get_mutant_name(self, file_name, diff):
        return hashlib.sha1((file_name + diff).encode()).hexdigest()

    def get_mutant_name(self, mutant):
        return self._get_mutant_name(
            self.get_from(mutant['program'], 'name') + '.' + self.get_from(mutant['program'], 'extension'),
            mutant['diff'],
        )

    def get_program_name(self, program):
        return program.file_name

    def get_or_create_person(self, name):
        return self.get_or_create(
            name,
            type_=SCHEMA.person,
            mb_type='person',
            # predicate_object_pairs= TODO add predicate pairs if needed
        )

    def get_db_to_rdf_operator(self, operator):
        return self.namespace.Operator  # TODO: implement this

    def get_or_create_program(self, program, location=None):
        return self.get_or_create(
            program.file_name,
            type_=self.namespace.Program,
            mb_type='program',
            predicate_object_pairs=[
                (SCHEMA.codeRepository, Literal(location, datatype=SCHEMA.URL)),
                (SCHEMA.programmingLanguage, Literal(str(program.language).split('.')[-1])),
                (self.namespace.extension, Literal(str(program.language).split('.')[-1])),
                (SCHEMA.name, Literal('.'.join(str(program.file_name).split('.')[:-1]))),
            ]
        )

    def get_or_create_operator(self, operator):
        action_to_rdf = {
            Operation.insertion: self.namespace.InsetionOperator,
            Operation.replacement: self.namespace.ReplacementOperator,
            Operation.deletion: self.namespace.DeletionOperator,
        }
        primitive_to_rdf = {
            Type.arithmetic: self.namespace.Arithmetic,
            Type.relational: self.namespace.Relational,
            Type.conditional: self.namespace.ShortCircuitEvaluation,
            Type.shift: self.namespace.Shift,
            Type.logical: self.namespace.Logical,
            Type.statement: self.namespace.Statement,
            Type.constant: self.namespace.Constant,
            Type.variable: self.namespace.Variable,
        }
        name_to_class = {
            'ABSI': Class.predicate_analysis,
            'AORB': Class.predicate_analysis,
            'AORS': Class.predicate_analysis,
            'AOIU': Class.predicate_analysis,
            'AOIS': Class.predicate_analysis,
            'AODU': Class.predicate_analysis,
            'AODS': Class.predicate_analysis,
            'ROR': Class.predicate_analysis,
            'ROD': Class.predicate_analysis,
            'COR': Class.predicate_analysis,
            'COD': Class.predicate_analysis,
            'COI': Class.predicate_analysis,
            'SOR': Class.predicate_analysis,
            'LOR': Class.predicate_analysis,
            'LOI': Class.predicate_analysis,
            'LOD': Class.predicate_analysis,
            'ASRS': Class.predicate_analysis,
            'CDL': Class.coincidental_correctness,
            'VDL': Class.coincidental_correctness,
            'SDL': Class.stmt_analysis,
        }
        class_to_rdf = {
            Class.stmt_analysis: self.namespace.StatementAnalysis,
            Class.predicate_analysis: self.namespace.PredicateAnalysis,
            Class.coincidental_correctness: self.namespace.CoincidentalCorrectness,
        }
        return self.get_or_create(
            operator.name,
            type_=self.namespace.Operator,
            mb_type='operator',
            predicate_object_pairs=[
                (self.namespace.operatorAction, action_to_rdf[operator.operation]),
                (self.namespace.operatorDescription, Literal(operator.description)),
                (self.namespace.primitiveOperator, primitive_to_rdf[operator.type]),
                (self.namespace.operatorClass, class_to_rdf[name_to_class[operator.name]]),
                (self.namespace.operatorName, Literal(operator.long_name)),
                # TODO: name
            ]
        )

    def check_mutant_exists(self, file_name, difference):
        print(file_name)
        print(self._get_mutant_name(file_name, difference))
        uri = URIRef(f'{self.prefix}:mutant#{self._get_mutant_name(file_name, difference)}')
        return (uri, None, None) in self.graph

    def get_or_create_mutant(self, mutant):
        name = self.get_mutant_name(mutant)
        predicate_object_pairs = [
            (self.namespace.difference, Literal(mutant['diff'])),
            (SCHEMA.contributor, self.get_or_create_person(mutant['source'])),
            (self.namespace.program, mutant['program']),
            (SCHEMA.isBasedOn, mutant['program']),
        ]
        if mutant['equivalent'] is not None:
            predicate_object_pairs.append(
                (self.namespace.equivalence, Literal(mutant['equivalent'], datatype=SCHEMA.boolean))
            )
        for operator in mutant['operators']:
            predicate_object_pairs.append(
                (self.namespace.operator, self.get_or_create_operator(operator))
            )
        return self.get_or_create(
            name,
            type_=self.namespace.Mutant,
            mb_type='mutant',
            predicate_object_pairs=predicate_object_pairs,
        )

        # Program properties
        # TODO add author -> tool used (also needs to be added to the DB)
        # TODO: add poublication
        # TODO: add publisher
        # Mutant properties
        # TODO: metadata that kills the mutant

    def gen_db_mutants(self):
        for mutant in self.get_db_mutants():
            self.get_or_create_mutant(mutant)

    def export(self, destination=None, **kwargs):
        if not destination:
            destination = self.from_file
        return self.graph.serialize(destination=destination, format='turtle', **kwargs)

    def __str__(self):
        return self.graph.serialize(format='turtle').decode('utf-8')

    def get_mutants(self, program=None, equivalencies=None, operators=None):
        if equivalencies:
            equivalencies = [Literal(e, datatype=SCHEMA.boolean) for e in equivalencies]
        if operators:
            operators = [o if isinstance(o, URIRef) else self.get_full_uri(o, 'operator') for o in operators]

        for mutant in self.graph.subjects(RDF.type, self.namespace.Mutant):
            if program is not None and not (mutant, SCHEMA.isBasedOn, program) in self.graph:
                continue

            if equivalencies is not None and \
               not any((mutant, self.namespace.equivalence, e) in self.graph for e in equivalencies):
                continue

            if operators is not None and \
               not any((mutant, self.namespace.operator, o) in self.graph for o in operators):
                continue

            yield mutant

    def get_programs(self, programs=None, languages=None):
        if programs is not None:
            programs = [str(p) for p in programs]
        if languages is not None:
            languages = [Literal(l) for l in languages]
        for program in self.graph.subjects(RDF.type, self.namespace.Program):
            if languages is not None and \
               not any((program, SCHEMA.programmingLanguage, o) in self.graph for o in languages):
                continue

            file_name = self.get_from(program, 'name') + '.'
            file_name += self.get_from(program, 'extension')
            if programs is None or str(file_name) in programs:
                yield program

    def get_operators(self):
        return self.graph.subjects(RDF.type, self.namespace.Operator)

    def get_program_from_mutant(self, mutant):
        return next(self.graph.objects(mutant, SCHEMA.isBasedOn))

    def fix_up_programs(self):
        for db_program in session.query(db.Program).all():
            for program in self.graph.subjects(RDF.type, self.namespace.Program):
                # print(db_program, program)
                if db_program.file_name not in program:
                    continue
                self.graph.add((program, SCHEMA.codeRepository, Literal(db_program.path, datatype=SCHEMA.URL)))
                self.graph.add((program, SCHEMA.programmingLanguage, Literal(str(db_program.language).split('.')[-1])))
                self.graph.add((program, self.namespace.extension, Literal(str(db_program.language).split('.')[-1])))
                self.graph.add((program, SCHEMA.name, Literal('.'.join(str(db_program.file_name).split('.')[:-1]))))

        self.export()

    # def fix_up_operators(self):
    #     for mutant in self.graph.subjects(RDF.type, self.namespace.Mutant):
    #         for operator_name in self.graph.objects(mutant, self.namespace.operator):
    #             operator = self.get_or_create(str(operator_name), type_=self.namespace.Operator, mb_type='operator')
    #             self.graph.add((mutant, self.namespace.operator, operator))
    #             self.graph.remove((mutant, self.namespace.operator, operator_name))

    #     self.export()

    # def fix_up_mutants(self):
    #     for mutant in self.graph.subjects(RDF.type, self.namespace.Mutant):
    #         new_name = self._get_mutant_name(
    #             self.get_from(self.get_from(mutant, 'program'), 'name'),
    #             self.get_from(mutant, 'difference')
    #         )
    #         subject = URIRef(f'{self.prefix}:mutant#{new_name}')
    #         for p, o in self.graph.predicate_objects(mutant):
    #             self.graph.add((subject, p, o))
    #             self.graph.remove((mutant, p, o))
    #     self.export()

if __name__ == '__main__':
    mbrdf = MutantBenchRDF()
    # mbrdf.gen_db_mutants()
    # mbrdf.export()
    # mbrdf.fix_up_mutants()

    print(mbrdf)
# # loop through each triple in the graph (subj, pred, obj)
# for subj, pred, obj in mutantbench:
#     # check if there is at least one triple in the Graph
#     if (subj, pred, obj) not in mutantbench:
#        raise Exception('It better be!')

# # print the number of 'triples' in the Graph
# # print('graph has {} statements.'.format(len(g)))
# # prints graph has 86 statements.

# # print out the entire Graph in the RDF Turtle format
# # print(mutantbench.serialize(format='turtle').decode('utf-8'))
# # mutantbench.add((mutantbench, RDF.type, ))
# print(mutantbench.serialize(format='turtle').decode('utf-8'))
# # print(len(mutantbench))
# print("--- printing mboxes ---")
# for subject, predicate, obj in mutantbench:
#     print(subject)
#     print(predicate)
#     print(obj)
#     print('\n\n\n\n\n')
#     print('\n\n\n\n\n\n\n\n\nPERSON', person)
#     for mbox in mutantbench.objects(person):
#         print(mbox)
