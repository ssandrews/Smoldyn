"""smoldyn
Python bindings.
"""

from . import _smoldyn

__version__ = _smoldyn.__version__

# Instantiate object of 
__cppobj__ = _smoldyn.Smoldyn()

def obj():
    """Return initialised Smoldyn object.
    """
    global __cppobj__
    return __cppobj__

# And export other symbols as well.
from .simulation import * 
from .geometry import *
from .kinetics import *
from .utils import *
