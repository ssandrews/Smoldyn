"""smoldyn
Python bindings.
"""

import smoldyn._smoldyn as _smoldyn

__version__ = _smoldyn.__version__

def load_model(path, arg=''):
    """Simulate a given path

    Parameters
    ----------

    """
    return _smoldyn.load_model(path, arg)

from smoldyn._smoldyn import *
