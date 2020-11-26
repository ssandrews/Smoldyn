"""smoldyn
Python bindings.
"""

from . import _smoldyn
__version__ = _smoldyn.__version__
def version():
    return __version__

# Bring low-level c-api to the front.
from smoldyn._smoldyn import *                        # type: ignore
from smoldyn.smoldyn import *
