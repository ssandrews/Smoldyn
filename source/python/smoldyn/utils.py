# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2019-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn._smoldyn  # type: ignore
import typing as T

__matplotlib_found__ = False
try:
    from matplotlib import colors  # type: ignore

    __matplotlib_found__ = True
except ImportError:
    __matplotlib_found__ = False


def load_model(path, *args):
    smoldyn._smoldyn.load_model(path, *args)


def color2RGBA(colorname: str) -> T.Tuple[float, float, float, float]:
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
        clr: T.Tuple[float, float, float, float] = colors.to_rgba(colorname)
    else:
        clr = smoldyn._smoldyn.color2RGBA(colorname)
    return clr
