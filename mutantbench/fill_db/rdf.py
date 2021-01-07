from sqlalchemy.sql.expression import func
from mutantbench import session, db
from rdflib import Graph, Literal, RDF, URIRef, RDFS
from rdflib.namespace import FOAF, XSD, Namespace
import rdflib
from collections import defaultdict


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
            try:
                return self.get_from(subject, self.namespace[predicate])
            except StopIteration:
                return self.get_from(subject, SCHEMA[predicate])

        return next(self.graph.objects(subject, predicate))

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
        return abs(hash((file_name, diff)))

    def get_mutant_name(self, mutant):
        return self._get_mutant_name(mutant.program.file_name, mutant.diff)

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
            type_=self.namespace.program,
            mb_type='program',
            predicate_object_pairs=[
                (SCHEMA.codeRepository, Literal(location, datatype=SCHEMA.URL)),
                (SCHEMA.programmingLanguage, Literal(program.split('#').split('.')[-1])),
                (self.namespace.extension, Literal(program.split('#').split('.')[-1])),
                (SCHEMA.name, Literal('.'.join(program.split('#').split('.')[:-1]))),
            ]
        )

    def check_mutant_exists(self, file_name, difference):
        uri = URIRef(f'{self.prefix}:mutant#{self._get_mutant_name(file_name, difference)}')
        return (uri, None, None) in self.graph

    def get_or_create_mutant(self, mutant):
        name = self.get_mutant_name(mutant)
        predicate_object_pairs = [
            (self.namespace.difference, Literal(mutant.diff)),
            (SCHEMA.contributor, self.get_or_create_person(mutant.program.source)),
            (self.namespace.program, self.get_or_create_program(mutant.program)),
            (SCHEMA.isBasedOn, self.get_or_create_program(mutant.program)),
        ]
        if mutant.equivalent is not None:
            predicate_object_pairs.append(
                (self.namespace.equivalence, Literal(mutant.equivalent, datatype=SCHEMA.boolean))
            )
        for operator in mutant.operators:
            predicate_object_pairs.append(
                (self.namespace.operator, Literal(operator.name))
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

    def get_programs(self, programs=None):
        programs = [str(p) for p in programs]
        for program in self.graph.subjects(RDF.type, self.namespace.Program):
            file_name = self.get_from(program, 'name') + '.' + self.get_from(program, 'extension')
            if programs is None or str(file_name) in programs:
                yield program

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

if __name__ == '__main__':
    mbrdf = MutantBenchRDF()
    # mbrdf.gen_db_mutants()
    # mbrdf.export()
    # mbrdf.fix_up_programs()

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
