

import rdflib


class URIRef(rdflib.URIRef):


class Graph(rdflib.Graph):
    def __init__(self, *args, default_namespace=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.default_namespace = default_namespace
        self.ns = self.default_namespace


