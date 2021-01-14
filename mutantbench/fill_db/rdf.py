import hashlib
from rdflib import Graph, Literal, RDF, URIRef
from rdflib.namespace import Namespace
from collections import defaultdict


SCHEMA = Namespace('http://schema.org/')
MB = Namespace('https://github.com/MutantBench/MutantBench/tree/main/mutantbench/mutantbench.ttl/')


class MutantBenchRDF(object):
    def __init__(
        self,
        namespace=MB,
        prefix='mb',
        load_from_file='mutants.ttl'
    ):
        self.prefix = prefix
        self.from_file = load_from_file

        self.graph = Graph()
        if self.from_file:
            self.graph.parse(self.from_file, format='turtle')
        self.namespace = namespace
        self.graph.bind(self.prefix, self.namespace)
        self.graph.bind('schema', SCHEMA)
        self.rdf_cache = defaultdict(dict)

    def get_from(self, subject, predicate):
        if not isinstance(predicate, URIRef):
            return self.get_from(subject, self.namespace[predicate]) or self.get_from(subject, SCHEMA[predicate])

        return self.graph.value(subject, predicate)

    def get_full_uri(self, name, type_):
        return URIRef(f'{self.prefix}:{type_}#{name}')

    def get_or_create(self, name, type_, predicate_object_pairs=[]):
        cache_hash = hash((str(name), str(type_)))

        if cache_hash in self.rdf_cache:
            return self.rdf_cache[cache_hash]

        if (name, None, None) in self.graph:
            self.rdf_cache[cache_hash] = name
            return name

        self.graph.add((name, RDF.type, type_))
        for p, o in predicate_object_pairs:
            self.graph.add((name, p, o))

        self.rdf_cache[cache_hash] = name

        return name

    def get_mutant_hash(self, file_name, diff):
        return hashlib.sha1((file_name + diff).encode()).hexdigest()

    def get_program_name(self, program):
        return program.file_name

    def check_mutant_exists(self, file_name, difference):
        uri = URIRef(f'mb:mutant#{self.get_mutant_hash(file_name, difference)}')
        return (uri, None, None) in self.graph

    def export(self, destination=None, **kwargs):
        if not destination:
            destination = self.from_file
        return self.graph.serialize(destination=destination, format='turtle', **kwargs)

    def get_mutants(self, program=None, equivalencies=None, operators=None):
        if equivalencies:
            equivalencies = [Literal(e, datatype=SCHEMA.boolean) for e in equivalencies]
        if operators:
            operators = [o if isinstance(o, URIRef) else self.get_full_uri(o, 'operator') for o in operators]

        for mutant in self.graph.subjects(RDF.type, self.namespace.Mutant):
            if program is not None and not (mutant, self.namespace.program, program) in self.graph:
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
            languages = [Literal(lang) for lang in languages]
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

    def fix_mutants(self):
        for mutant in self.graph.subjects(RDF.type, self.namespace.Mutant):
            diff = self.get_from(mutant, 'difference')
            self.graph.remove((mutant, self.namespace.difference, Literal(diff)))
            diff = diff.replace('\r', '').rstrip()
            self.graph.add((mutant, self.namespace.difference, Literal(diff)))
            file_name = self.get_from(self.get_from(mutant, 'program'), "fileName")
            new_mutant = URIRef(f'mb:mutant#{self.get_mutant_hash(file_name, diff)}')
            for p, o in self.graph.predicate_objects(mutant):
                self.graph.remove((mutant, p, o))
                self.graph.add((new_mutant, p, o))

    def __str__(self):
        return self.graph.serialize(format='turtle').decode('utf-8')


if __name__ == '__main__':
    mbrdf = MutantBenchRDF()
    mbrdf.fix_mutants()
    mbrdf.export()

    print(mbrdf)
