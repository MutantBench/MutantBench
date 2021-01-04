from sqlalchemy.sql.expression import func
from mutantbench import session, db
from rdflib import Graph, Literal, RDF, URIRef, RDFS
from rdflib.namespace import FOAF, XSD, Namespace
import rdflib
from collections import defaultdict


SCHEMA = Namespace('http://schema.org/')


class MutantBenchRDF(object):
    def __init__(self, namespace_uri='http://larspolo.github.com/mutantbench/', prefix='mb'):
        self.prefix = prefix
        self.namespace_uri = namespace_uri

        self.graph = rdflib.Graph()
        self.namespace = Namespace(namespace_uri)
        self.graph.bind(self.prefix, self.namespace)
        self.graph.bind('schema', SCHEMA)
        self.session = session
        self.rdf_cache = defaultdict(dict)

    def get_or_create_uri(self, name, type_):
        if name in self.rdf_cache[type_]:
            return self.rdf_cache[type_][name]

        uri = URIRef(f'{self.prefix}:{type_}#{name}')
        self.graph.add((uri, RDF.type, SCHEMA.person))
        self.graph.add((uri, SCHEMA.name, Literal(name)))
        self.rdf_cache[type_][name] = uri
        return uri

    def get_db_mutants(self):
        return session.query(db.Mutant).filter(
            func.length(db.Mutant.diff) < 300
        )

    def get_mutant_uri(self, mutant):
        return self.get_or_create_uri(
            abs(hash((mutant.program.file_name, mutant.diff))),
            'mutant'
        )

    def get_program_uri(self, program):
        return self.get_or_create_uri(
            program.file_name,
            'program'
        )

    def get_db_to_rdf_operator(self, operator):
        return self.namespace.Operator  # TODO: implement this

    def add_mutant(self, mutant):
        uri = self.get_mutant_uri(mutant)
        self.graph.add((uri, RDF.type, self.namespace.Mutant))
        # Mutant properties
        self.graph.add((uri, self.namespace.difference, Literal(mutant.diff)))
        if mutant.equivalent is not None:
            self.graph.add((uri, self.namespace.equivalence, Literal(mutant.equivalent, datatype=SCHEMA.boolean)))
        for operator in mutant.operators:
            self.graph.add((uri, self.namespace.operator, Literal(operator.name)))

        # SoftwareSourceCode properties
        self.graph.add((uri, SCHEMA.contributor, self.get_or_create_uri(mutant.program.source, 'person')))
        # self.graph.add(())  # TODO add author -> tool used (also needs to be added to the DB)
        # TODO: add poublication
        # TODO: add publisher
        # TODO: metadata that kills the mutant
        self.graph.add((uri, SCHEMA.isBasedOn, self.get_program_uri(mutant.program)))

    def gen_mutants(self):
        for mutant in self.get_db_mutants():
            self.add_mutant(mutant)

    def export(self, destination='mutants.ttl', **kwargs):
        return self.graph.serialize(destination=destination, format='turtle', **kwargs)

    def __str__(self):
        return self.graph.serialize(format='turtle').decode('utf-8')


if __name__ == '__main__':
    mbrdf = MutantBenchRDF()
    mbrdf.gen_mutants()
    mbrdf.export()

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
