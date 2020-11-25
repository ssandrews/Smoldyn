# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

from smoldyn import _smoldyn                  # type: ignore
from typing import Tuple

__matplotlib_found__ = False
try:
    from matplotlib import colors           # type: ignore
    __matplotlib_found__ = True
except ImportError as e:
    pass

def load_model(path, args=''):
    _smoldyn.load_model(path, args)

def color2RGBA(colorname:str) -> Tuple[float,float,float,float]:
    """Convert color name to RGBA value.

    If `matplotlib` is installed, its `colors` submodule is used else in-built
    color parser is used which has limited number of colors.

    Parameters
    ----------
    colorname : str
        Name of the color

    Return:
    ------
        A tuple of 4 values between 0 and 1.0 (red, green, blue, alpha)
    """
    if __matplotlib_found__:
        return colors.to_rgba(colorname)
    return _smoldyn.color2RGBA(colorname)
