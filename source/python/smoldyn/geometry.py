# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh <dilawars@ncbs.res.in>"

"""
Geometry related classes.
"""

import functools
import operator
from typing import List, Union
from smoldyn import _smoldyn
from smoldyn._smoldyn import PanelShape, PanelFace, SrfAction, DrawMode, ErrorCode
from smoldyn.kinetics import Species
from smoldyn.config import __logger__
from dataclasses import dataclass, field

__all__ = [
    "Surface",
    "Port",
    "Panel",
    "Sphere",
    "Rectangle",
    "Triangle",
    "Hemisphere",
    "Cylinder",
    "Disk",
    "Compartment",
    "Boundaries",
]


class Compartment:
    """Compartment
    """

    def __init__(self, arg):
        super().__init__()
        self.arg = arg


class Panel(object):
    """Panels are required to construct a surface. Following types of Panel are
    supported. 

    :py:class:`~.Rectangle`, :py:class:`~.Triangle`, :py:class:`~.Sphere`,
    :py:class:`~.Hemisphere`, :py:class:`~.Cylinder`, :py:class:`~.Disk`
    """

    def __init__(self, panelshape: PanelShape = PanelShape.none, name=""):
        self.name = name
        self.ctype: PanelShape = panelshape
        self.pts: List[float] = []

    def _axisIndex(self, axisname):
        axisDict = dict(x=0, y=1, z=2)
        return int(axisDict.get(axisname.lower(), axisname))

    def addToSurface(self, surface):
        print("[INFO] Adding pts to surface")

    def toText(self):
        return f"panel {self.ctype} {self.axisstr} {' '.join(map(str,self.pts))} {self.name}".strip()


class Rectangle(Panel):
    def __init__(
        self, corner: List[float], dimensions: List[float], axis: str, name=""
    ):
        """Rectangle (Panel)

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
        super(Rectangle, self).__init__(PanelShape.rect, name=name)
        self.corner = corner
        self.dimensions = dimensions
        assert axis[0] in "+-", "axis must precede by '+' or '-'"
        assert axis[1].lower() in "012xyz"
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
    def __init__(self, vertices: List[List[float]] = [[]], name=""):
        """Triangle Panel.

        Parameters
        ----------
        vertices : List[List[float]]
            vertices of triangle.
        name :
            name
        """
        super(Triangle, self).__init__(PanelShape.tri, name)
        self.axisstr = ""
        self.vertices = vertices
        self.pts = functools.reduce(operator.iconcat, vertices, [])


class Sphere(Panel):
    def __init__(self, center: List[float], radius: float, name=""):
        """Sphere 

        Parameters
        ----------
        center : List[float]
            The center of the sphere.
        radius : float
            The radius of the sphere.
        name : optional
            name
        """
        super(Sphere, self).__init__(PanelShape.sph, name=name)
        self.center = center
        self.radius = radius
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius]


class Hemisphere(Panel):
    def __init__(
        self,
        center: List[float],
        radius: float,
        vector: List[float],
        slices: int,
        stacks: int,
        name: str = "",
    ):
        """Hemisphere 

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
        self.ctype = PanelShape.hemi
        self.center = center
        self.radius = radius
        self.vector = vector
        self.slices = slices
        self.stacks = stacks
        # Smoldyn Panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, *self.vector, self.slices, self.stack]


class Cylinder(Panel):
    def __init__(
        self,
        start: List[float],
        end: List[float],
        radius: float,
        slices: int,
        stacks: int,
        name="",
    ):
        """Cylinder

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
        super(Cylinder, self).__init__(PanelShape.cyl, name=name)
        self.start = start
        self.end = end
        self.slices = slices
        self.stacks = stacks
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.start, *self.end, self.slices, self.stacks]


class Disk(Panel):
    def __init__(
        self, center: List[float], radius: float, vector: List[float], name=""
    ):
        """Disk

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
        super(Disk, self).__init__(PanelShape.disk, name=name)
        self.center = center
        self.radius = radius
        self.vector = vector
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, *self.vector]


class Surface(object):
    """Surfaces are infinitesimally thin structures that can be used to
    represent cell membranes, obstructions, system boundaries, or other things.

    Note
    -----
    They are 2D structures in 3D simulations, or 1D lines or
    curves in 2D simulations (or 0D points in 1D simulations).Each surface
    has a “front” and a “back” face, so molecules can interact differently
    with the two sides of a surface.Each surface is composed of one or more
    “:py:class:`~.Panels`”, where each panels can be a rectangle, triangle, sphere,
    hemisphere, cylinder, or a disk. Surfaces can be disjoint, with separate
    non-connected portions.  However, all portions of a given surface type
    are displayed in the same way and interact with molecules in the same
    way.
    """

    def __init__(self, name: str, panels: List[Panel]):
        """
        Parameters
        ----------
        panels : List[Panel]
            A surface consists of one or more panels
        name : str 
            name of the surface

        Examples:
        >>> r1 = S.Rectangle(corner=[100,0,0], dimensions=[100,100], axis='-x')
        >>> r2 = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='+y')
        >>> s = smoldyn.Surface("walls", panels=["r1", "r2"])
        """
        self.panels = panels
        self.name = name
        _smoldyn.addSurface(self.name)
        self._addToSmoldyn()

    def _addToSmoldyn(self):
        # add panels
        assert self.name, "Surface name is missing"
        for panel in self.panels:
            k = _smoldyn.addPanel(
                self.name, panel.ctype, panel.name, panel.axisstr, panel.pts
            )
            assert k == ErrorCode.ok, f"Failed to add panel to surface {self.name}"

    def setStyle(
        self,
        panelface: str,
        drawmode: str,
        color="black",
        thickness: float = 1.0,
        stipplefactor: int = 1,
        stipplepattern: int = 0,
        shininess: int = 1,
    ):
        """Set drawing style for this surface.

        Parameters
        ----------
        panelface : str
            face of the panel. `face` can be “front”, “back”, or “both”.
        drawmode : str
            `drawmode` may be “none”, “vertex”, “edge”, “face”, or combinations 
            of ‘v’, ‘e’, or ‘f’ for multiple renderings of vertices, edges,
            and/or faces.  2-D spheres and hemispheres are either filled or are
            outlined depending on the polygon frontcharacter. If multiple
            renderings are chosen in 3D, then panel faces are shown in the
            requested color and other renderings are shown in black.
        thickness : float
            Boldness of the surface in pixels for drawing purposes.  This is
            only relevant for 1-D and 2-D simulations, and for 3-D simulations
            in which surfaces are drawn with just vertices or edges and not
            faces.
        color : str, tuple(R, G, B, A)
            color e.g. 'black', 'red' etc.
        stipplefactor : int
            stipplefactor is the repeat distance for the entire stippling
            pattern (1 is a good choice).  Stippling of the surface edges, for
            drawing purposes.  This is only relevant for 3-D simulations in
            which surfaces are drawn with just edges and not faces, and with
            `opengl_good` or `opengl_better` display method.
        stipplepattern : int
            stipplepattern a hexidecimal integer, enter the stippling pattern between
            0x0000 (0) and 0xFFFF (65536).  0x00FF (256) has long dashes,
            0x0F0F (3855) has medium dashes, 0x5555 (21845)  has dots, etc.
            Turn stippling off with 0xFFFF (65536).
        shininess : float
            Shininess of the surface for drawing purposes.  This value can
            range from 0 for visually flat surfaces to 128 for very shiny
            surfaces.  This is only relevant for some simulations.
        """
        assert self.name, "Surface must have a valid name"
        assert panelface in ("front", "back", "both")
        # TODO: Check on drawmode
        k = _smoldyn.setSurfaceStyle(
            self.name,
            PanelFace.__members__[panelface],
            DrawMode.__members__[drawmode],
            thickness,
            color,
            stipplefactor,
            stipplepattern,
            shininess,
        )
        assert k == ErrorCode.ok, "Failed to set drawing style"

    def addAction(
        self, species: Union[Species, str], face: str, action: str, new_spec=None
    ):
        """The behavior of molecules named species when they collide with the
        `face` of this surface.

        Parameters
        ----------
        species : Union[Species, str]
            Species. If `"all"`, then this action applies to all molecules.
        face : str
            `face` can be “front”, “back”, or “both”.  
        action : str
            The action can be  “reflect”, “absorb”, “transmit”, “jump”, “port”,
            or “periodic”.
        new_spec: str
            If `new_spec` is entered, then the molecule changes to this new
            species upon surface collision. In addition, it’s permissible to
            enter the action as “multiple,” in which case the rates need to be
            set with rate; alternatively, just setting the rates will
            automatically set the action to “multiple.” The default is
            transmission for all molecules.
        """
        assert action in ["reflect", "absorb", "transmit", "jump", "port", "periodic"]
        if isinstance(species, Species):
            sname, sstate = species.name, species.state
        else:
            sname, sstate = species, _smoldyn.MolecState.soln

        # TODO:
        if new_spec:
            raise NotImplementedError("This feature is not implemented.")

        k = _smoldyn.setSurfaceAction(
            self.name, PanelFace.__members__[face]
            , sname, sstate, SrfAction.__members__[action]
        )
        assert k == ErrorCode.ok

class Port(object):
    """Ports are data structures that are used for importing and
    exporting molecules between a Smoldyn simulation and another simulation.  In
    particular, they are designed for the incorporation of Smoldyn into MOOSE,
    but they could also be used to connect multiple Smoldyn simulations or for
    other connections.

    A port is essentially a surface and a buffer. Smoldyn
    molecules that hit the porting surface are removed from the Smoldyn
    simulation and are put into the buffer for export.  Once exported, they are
    removed from the buffer.  Also, molecules may be added to the Smoldyn
    simulation at the porting surface by other programs.
    """

    def __init__(self, name:str, surface:Union[Surface,str], panel:str):
        """
        Parameters
        ----------
        name : str
            name of the port
        surface : Union[Surface,str]
            Porting surface (must be created before).
        panel : str
            Face of the which surface active for porting: "front" or "back".
        """
        self.name : str = name
        self.surfname = surface.name if isinstance(surface, Surface) else surface
        assert panel in ["front", "back"]
        self.panel = PanelFace.__members__[panel]
        k = _smoldyn.addPort(self.name, self.surfname, self.panel)
        assert k == ErrorCode.ok

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
