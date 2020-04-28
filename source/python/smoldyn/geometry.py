# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"


from . import _smoldyn


class Geometry(object):
    """Geometry in smoldyn.


    """
    def __init__(self, dim):
        super().__init__()
        self.dim = dim
    

class Surface(Geometry):
    """Surface

    """
    def __init__(self, arg):
        super().__init__()


class Compartment(Geometry):
    """Compartment
    """
    def __init__(self, arg):
        super().__init__()
        self.arg = arg
    

class Box(Geometry):
    def __init__(self, size):
        super().__init__(3)
        self.size : float = size

    def addMolecules(self, molecule, num, *args, **kwargs):
        print(f"[INFO ] Adding molecule {molecule}")
        


