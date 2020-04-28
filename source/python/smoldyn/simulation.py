# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

from . import _smoldyn


class Simulation(object):
    """Simulation class.
    
    for Simulation
    """

    def __init__(self, arg):
        super(Simulation, self).__init__()
        self.arg = arg
    

