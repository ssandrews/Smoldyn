# -*- coding: utf-8 -*-
"""
File: geometry.py
Author: Dilawar Singh
Email: dilawars@ncbs.res.in
Description: 
    This file contains classes related to geometry of simulation arena.
"""

import functools
import operator
from typing import List
from smoldyn import _smoldyn
from smoldyn.config import __logger__
from dataclasses import dataclass, field


class Compartment:
    """Compartment
    """

    def __init__(self, arg):
        super().__init__()
        self.arg = arg


class Panel(object):
    """Panel: A surface is composed of one or more Panels. Following types of
    Panel are supported.
        - Rectangle
        - Triangle
        - Sphere
        - Hemisphere
        - Cylinder
        - Disk
    """

    def __init__(self, name=""):
        self.name = name
        self.axisstr: str = ""
        self.ctype: str = ""
        self.pts: List[float] = []

    def _axisIndex(self, axisname):
        axisDict = dict(x=0, y=1, z=2)
        return int(axisDict.get(axisname.lower(), axisname))

    def addToSurface(self, surface):
        print("[INFO] Adding pts to surface")

    def toText(self):
        return f"panel {self.ctype} {self.axisstr} {' '.join(map(str,self.pts))} {self.name}".strip()


class Rectangle(Panel):
    """Rectangle
    """

    def __init__(
        self, corner: List[float], dimensions: List[float], axis: str, name=""
    ):
        """__init__.

        Parameters
        ----------
        corner : List[float]
            One corner of the rectangle.
        dimensions : List[float]
            Dimensions of the rectangle e.g., for a 2D rectangle it is a list of
            values of width and height.
        height : float
            height of the rectangle
        axis : str
            axis perpendicular to the rectangle preceded by '+' if the panel
            front faces the positive axis and '-' if it faces the negative
            axis e.g., '+x', '+0', '-y' etc.
        name : optional
            name of the panel.
        """
        super(Rectangle, self).__init__(name=name)
        self.ptype = "rect"
        self.corner = corner
        self.dimensions = dimensions
        assert axis[0] in "+-", "axis must precede by '+' or '-'"
        assert axis[1].lower() in ["0", "1", "x", "y"]
        self.axis = axis.lower()
        self.axisIndex = self._axisIndex(axis[1])
        self.toGenericPanel()

    def toGenericPanel(self):
        self.axisstr = self.axis
        self.pts = [*self.corner, *self.dimensions]
        return self.axisstr, self.pts


##    def plot(self, ax = None):
##        """
##        TODO: Not tested.
##        """
##        import matplotlib.pyplot as plt
##        import matplotlib.patches as patches
##        from mpl_toolkits.mplot3d import Axes3D
##        import mpl_toolkits.mplot3d.art3d as art3d
##
##        if ax is None:
##            ax = plt.subplot(111, projection='3d')
##        rect = patches.Rectangle(self.center, self.width, self.height)
##        ax.add_patch(rect)
##        art3d.pathpatch_2d_to_3d(rect, zdir=self.axis[1])


class Triangle(Panel):
    """docstring for Triangle"""

    def __init__(self, vertices: List[List[float]] = [[]], name=""):
        """__init__.

        Parameters
        ----------
        vertices : List[List[float]]
            vertices of triangle.
        name :
            name
        """
        super(Triangle, self).__init__(name=name)
        self.ctype = "tri"
        self.axisstr = ""
        self.vertices = vertices
        self.pts = functools.reduce(operator.iconcat, vertices, [])


class Sphere(Panel):
    """docstring for Sphere"""

    def __init__(self, center: List[float], radius: float, name=""):
        """__init__.

        Parameters
        ----------
        center : List[float]
            The center of the sphere.
        radius : float
            The radius of the sphere.
        name : optional
            name
        """
        super(Sphere, self).__init__(name=name)
        self.ctype = "sph"
        self.center = center
        self.radius = radius
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius]


class Hemisphere(Panel):
    """docstring for Hemisphere"""

    def __init__(
        self,
        center: List[float],
        radius: float,
        vector: List[float],
        slices: int,
        stacks: int,
        name="",
    ):
        """__init__.

        Parameters
        ----------
        center : List[float]
            The center of hemisphere
        radius : float
            The radius of the hemisphere. Enter a positive radius to have the
            front of the surface on the outside and a negative radius for it to
            be on the inside.
        vector : List[float]
            The outward pointing vector (needs NOT be normalized)
        slices : int
            The number of slices
        stacks : int
            The number of stacks
        name : optional
            name
        """
        super(Hemisphere, self).__init__(name=name)
        self.ctype = "hemi"
        self.center = center
        self.radius = radius
        self.vector = vector
        self.slices = slices
        self.stacks = stacks
        # Smoldyn Panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, *self.vector, self.slices, self.stack]


class Cylinder(Panel):
    """docstring for Cylinder"""

    def __init__(
        self,
        start: List[float],
        end: List[float],
        radius: float,
        slices: int,
        stacks: int,
        name="",
    ):
        """__init__.

        Parameters
        ----------
        start : List[float]
            Cylinder axis start point 
        end : List[float]
            Cylinder axis's end point
        radius : float
            The radius of the cylinder. If the radius is negative then the front
            of the surface is on the inside otherwise it is on the outside.
        slices : int
            Number of slices
        stacks : int
            Number of stacks
        name : optional
            name
        """
        super(Cylinder, self).__init__(name=name)
        self.ctype = "cyl"
        self.start = start
        self.end = end
        self.slices = slices
        self.stacks = stacks
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.start, *self.end, self.slices, self.stacks]


class Disk(Panel):
    """docstring for Disk
    """

    def __init__(
        self, center: List[float], radius: float, vector: List[float], name=""
    ):
        """__init__.

        Parameters
        ----------
        center : List[float]
            center of the disk.
        radius : float
            radius of the disk.
        vector: List[float]
            A vector that points away from the front of the disk. The size of
            this vector is 2 for 2-D and 3 for 3-D.
        name : optional
            name
        """
        super(Disk, self).__init__(name=name)
        self.ctype = "disk"
        self.center = center
        self.radius = radius
        self.vector = vector
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, *self.vector]


class Surface(object):
    """Surface
    """

    def __init__(self, panels: List[Panel] = [], name: str = ""):
        self.panels = panels
        self.name = name


@dataclass
class Boundaries:
    low: List[float]
    high: List[float]
    types: List[str] = field(default_factory=lambda: ["r"])
    dim: field(init=False) = 0

    def __post_init__(self):
        assert len(self.low) == len(self.high), "Size mismatch."
        if len(self.types) == 1:
            self.types = self.types * len(self.low)
        self.dim = len(self.low)
        _smoldyn.setBoundaries(list(zip(self.low, self.high)))
        __logger__.debug("Getting boundaries", _smoldyn.getBoundaries())
        assert _smoldyn.getDim() == self.dim, (_smoldyn.getDim(), self.dim)

        k = _smoldyn.setBoundaryType(-1, -1, self.types[0])
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set boundary type: {k}"


@dataclass
class Partition:
    name: str
    value: float

    def __post_init__(self):
        k = _smoldyn.setPartitions(self.name, self.value)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set partition: {k}"


class MoleculePerBox(Partition):
    def __init__(self, size: float):
        super().__init__("molperbox", size)


class Box(Partition):
    def __init__(self, size):
        super().__init__("boxsize", size)
