# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

from smoldyn import _smoldyn

def load_model(path, args=''):
    _smoldyn.load_model(path, args)
