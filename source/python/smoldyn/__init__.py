"""smoldyn
Python bindings.
"""

# Bring low-level c-api to the front.
from ._smoldyn import *
__version__ = _smoldyn.__version__

from .smoldyn import *

