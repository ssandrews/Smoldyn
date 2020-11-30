"""
Types
"""

from typing import Union, Tuple, Dict, List
from smoldyn.utils import color2RGBA

# Either a string such as 'black', 'red', 'orange' or a tuple of 4 values such
# as [1,1,0,1] specifying RGBA value.
ColorType = Union[str, Tuple[float, float, float], Tuple[float, float, float, float]]

"""Diffusion coefficient can be a single numerical value (applies to all states
of molecules), or it could be a dictionary of molecule state and correponding
diffusion coefficient. 
"""
DiffConst = Union[float, Dict[str, float]]

"""Boundary type e.g. 'r' or 'rrr' or ['r', 'r', 'r']. If a single value is
given, it applies to all dimentions."""
BoundType = Union[str, List[str]]

class Color:
    """Color class.
    """
    def __init__(self, color):
        assert not isinstance(color, dict)
        self.name = color if not isinstance(color, Color) else color.name
        self.rgba = self._toRGBA()

    def _toRGBA(self):
        if isinstance(self.name, str):
            return color2RGBA(self.name) if self.name else None
        if len(self.name) == 3:
            return (*self.name, 1)

    def __str__(self):
        return str(self.name)

