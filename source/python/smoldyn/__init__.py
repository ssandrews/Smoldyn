"""smoldyn
Python bindings.
"""
import smoldyn._smoldyn as _smoldyn

__version__ = _smoldyn.__version__

def simulate(path, arg=''):
    """Simulate a given path

    Parameters
    ----------

    """
    return _smoldyn.simulate(path, arg)
