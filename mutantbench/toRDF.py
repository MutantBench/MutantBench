

from rdflib import URIRef, Graph
from rdflib.namespace import FOAF

if __name__ == "__main__":

    g = Graph()
    g.load("foaf.n3", format="n3")

    tim = URIRef("http://www.w3.org/People/Berners-Lee/card#i")

    print("Timbl knows:")

    for o in g.objects(tim, FOAF.knows / FOAF.name):
        print(o)
