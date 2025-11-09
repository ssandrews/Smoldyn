import smoldyn._smoldyn  # type: ignore
import typing as T


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
    try:
        import matplotlib.colors as colors  # type: ignore

        clr: T.Tuple[float, float, float, float] = colors.to_rgba(colorname)
    except Exception:
        clr = smoldyn._smoldyn.color2RGBA(colorname)

    return clr
