from typing import Union, Tuple, Dict, List
from smoldyn.utils import color2RGBA

# Either a string such as 'black', 'red', 'orange' or a tuple of 4 values such
# as [1,1,0,1] specifying RGBA value.
ColorType = Union[str, Tuple[float, float, float], Tuple[float, float, float, float]]
RGBA = Tuple[float, float, float, float]

"""Diffusion coefficient can be a single numerical value (applies to all states
of molecules), or it could be a dictionary of molecule state and correponding
diffusion coefficient. 
"""
DiffConst = Union[float, Dict[str, float]]

"""Boundary type e.g. 'r' or 'rrr' or ['r', 'r', 'r']. If a single value is
given, it applies to all dimentions."""
BoundType = Union[str, List[str]]


class Color:
    """Color class."""

    def __init__(self, color: str | None | ColorType | "Color"):
        if color:
            assert not isinstance(color, dict)
            self._inner: ColorType | str = (
                color if not isinstance(color, Color) else color._inner
            )
        else:
            # default value in c library.
            self._inner = (0.0, 0.0, 0.0, 1.0)
        self.rgba: RGBA = self._toRGBA()

    def _toRGBA(self) -> RGBA:
        if isinstance(self._inner, str) and self._inner:
            # convert color name e.g. black, yellow to rgba.
            return color2RGBA(self._inner)

        # Otherwise, we have 3 integers e.g. 1 0 1.
        value = [float(x) for x in self._inner]
        return (value[0], value[1], value[2], 1.0)

    def __str__(self) -> str:
        return str(self._inner)
