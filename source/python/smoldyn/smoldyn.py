# -*- coding: utf-8 -*-
"""Smoldyn user API.

See _smoldyn.so for C-API.
"""

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2020-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import os
import warnings
import operator
import functools
from pathlib import Path
from dataclasses import dataclass, field
from smoldyn.config import __logger__
from smoldyn.utils import color2RGBA
from typing import Union, Tuple, List, Dict
from smoldyn import _smoldyn

# Color type
Color = Union[str, Tuple[float, float, float, float]]


@dataclass
class NullSpecies:
    name: str = ""
    state = _smoldyn.MolecState.__members__["all"]


class Species(object):
    """Chemical species.
    """

    def __init__(
        self,
        name: str,
        state: str = "soln",
        color: Union[str, Dict[str, Color]] = "",
        difc: Union[float, Dict[str, float]] = 0.0,
        display_size: int = 3,
        mol_list: str = "",
    ):
        """
        Parameters
        ----------
        name : str
            Name of the species.
        state : str
            State of the species. One of the following: 
                'soln', 'front', 'back', 'up', 'down', 'bsoln', 'all', 'none',
                'some'
        color: Color or dict of Colors
            If a single `Color` is given, all states of this molecule will be
            assigned this color. To assign different colors to different
            states, use a dictionary of `Color`s.
        difc : float or a dict of floats
            Diffusion coefficient. If a single value is given, all states of
            the molecule gets the same value. To assign different values to
            different states, use a dict e.g. {'soln':1, 'front':2}.
        display_size : int, optional
            display size of the molecule (default 3px).
        mol_list : str, optional
            molecule list (default '')

        See also
        --------
        Color
        """
        self.name: str = name
        assert self.name

        k = _smoldyn.addSpecies(self.name)
        assert k == _smoldyn.ErrorCode.ok, "Failed to add molecule"

        if state not in _smoldyn.MolecState.__members__:
            raise NameError(
                f"{state} is not a valid MolecState. Available "
                "states are:{', '.join(_smoldyn.MolecState.__members__.keys())}"
            )

        self.state = _smoldyn.MolecState.__members__[state]
        self._size: float = display_size

        if not isinstance(difc, dict):
            difc = dict(all=difc)
        self._difc: Dict[str, float] = difc

        if not isinstance(color, dict):
            color = dict(all=color)
        self._color = color

        self.difc = self._difc
        self.color = self._color

        self._mol_list: str = mol_list
        if mol_list:
            self.mol_list: str = mol_list

    def __repr__(self):
        return f"<Molecule: {self.name}, difc={self.difc}, state={self.state}>"

    def __setStyle(self):
        for state, color in self._color.items():
            state = _smoldyn.MolecState.__members__[state]
            k = _smoldyn.setMoleculeStyle(self.name, state, self.size, color)
            assert k == _smoldyn.ErrorCode.ok, f"Failed to set style on {self}, {k}"

    @property
    def difc(self) -> float:
        return self._difc

    @difc.setter
    def difc(self, difconst):
        if not isinstance(difconst, dict):
            self._difc = dict(all=difconst)
        for state, D in self._difc.items():
            st = _smoldyn.MolecState.__members__[state]
            k = _smoldyn.setSpeciesMobility(self.name, st, D)
            assert k == _smoldyn.ErrorCode.ok

    @property
    def mol_list(self) -> str:
        return self._mol_list

    @mol_list.setter
    def mol_list(self, val):
        k = _smoldyn.addMolList(val)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add mollist: {k}"
        k = _smoldyn.setMolList(self.name, self.state, val)
        assert (
            k == _smoldyn.ErrorCode.ok
        ), f"Failed to set mol_list={val} on {self}: {k}"
        self._mol_list = val

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, clr):
        if isinstance(clr, str):
            clr = dict(all=clr)
        self.__setStyle()

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, size: float):
        self._size = size
        self.__setStyle()

    def addToSolution(
        self, mol: float, highpos: List[float] = [], lowpos: List[float] = []
    ):
        assert (
            self.state == _smoldyn.MolecState.soln
        ), f"You can't use this function on a Species with type {self.state}"
        k = _smoldyn.addSolutionMolecules(self.name, mol, lowpos, highpos)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add to solution: {k}"


class HalfReaction:
    def __init__(self, subs: List[Species], prds: List[Species], k, rname=""):
        assert len(subs) < 3, "At most two reactants are supported."
        r1 = subs[0]
        r2 = subs[1] if len(subs) == 2 else NullSpecies()
        if not rname:
            rname = "r%d" % id(self)
        assert rname
        k = _smoldyn.addReaction(
            rname,
            r1.name,
            r1.state,
            r2.name,
            r2.state,
            [x.name for x in prds],
            [x.state for x in prds],
            k,
        )
        if k != _smoldyn.ErrorCode.ok:
            __logger__.warning(f" Reactant1 : {r1}")
            __logger__.warning(f" Reactant2 : {r2}")
            __logger__.warning(f" Subtrate  : {subs}")
            __logger__.warning(f" Products  : {prds}")
            raise RuntimeError(f"Failed to add reaction: {k}")


class Reaction(object):
    """Reaction.
    """

    def __init__(self, subs: List[Species], prds: List[Species], kf, kb=0.0):
        assert len(subs) < 3, "At most two reactants are supported"
        self.forward = HalfReaction(subs, prds, kf)
        self.backbard = None
        if kb > 0:
            self.backbard = HalfReaction(prds, subs, kb)

    def productPlacement(self, type: str, *params):
        """Placement method and parameters for the products of reaction.
        This also affects the binding radius of the reverse reaction, as
        explained in the manual. 

        Parameters
        ----------
        type : 
            Availabele values: 'irrev', 'pgem', 'pgemmax', 'pgemmaxw', 'ratio'
            , 'unbindrad', 'pgem2', 'pgemmax2', 'ratio2', 'offset', 'fixed'
        params :
            params
        """


class Panel(object):
    """Panels are required to construct a surface. Following primitives are
    available.

    :py:class:`~Rectangle`, :py:class:`~Triangle`, :py:class:`~Sphere`,
    :py:class:`~Hemisphere`, :py:class:`~Cylinder`, :py:class:`~Disk`
    """

    def __init__(
        self, panelshape: _smoldyn.PanelShape = _smoldyn.PanelShape.none, name=""
    ):
        self.name = name
        self.ctype: _smoldyn.PanelShape = panelshape
        self.pts: List[float] = []

    def _axisIndex(self, axisname):
        axisDict = dict(x=0, y=1, z=2)
        return int(axisDict.get(axisname.lower(), axisname))

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
        super(Rectangle, self).__init__(_smoldyn.PanelShape.rect, name=name)
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


class Triangle(Panel):
    def __init__(self, vertices: List[List[float]] = [[]], name=""):
        """Triangle Panel.

        Parameters
        ----------
        vertices : 
            vertices of triangle.
        name :
            name
        """
        super(Triangle, self).__init__(_smoldyn.PanelShape.tri, name)
        self.axisstr = ""
        self.vertices = vertices
        self.pts = functools.reduce(operator.iconcat, vertices, [])


class Sphere(Panel):
    def __init__(
        self, center: List[float], radius: float, slices: int, stacks: int = 0, name=""
    ):
        """Sphere 

        Parameters
        ----------
        center : 
            The center of the sphere.
        radius : float
            The radius of the sphere.
        slices: int
            Number of slices (longitudnal lines) that are used for drawing the sphere.
        stacks: int
            Number of stacks (latitude lines, default 0) that are used to drawing the sphere. `
            If a non-postive number is given,`slices` value is used.
        name : optional
            name of the panel. If omitted, 0, 1, 2 etc. will be assigned. 
        """
        super(Sphere, self).__init__(_smoldyn.PanelShape.sph, name=name)
        self.center = center
        self.radius = radius
        self.slices = slices
        assert self.slices > 0, "Positive int is required"
        self.stacks = stacks if stacks > 0 else slices
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, self.slices, self.stacks]


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
        center : 
            The center of hemisphere
        radius : float
            The radius of the hemisphere. Enter a positive radius to have the
            front of the surface on the outside and a negative radius for it to
            be on the inside.
        vector :
            The outward pointing vector (needs NOT be normalized)
        slices : int
            The number of slices
        stacks : int
            The number of stacks
        name : optional
            name
        """
        super(Hemisphere, self).__init__(name=name)
        self.ctype = _smoldyn.PanelShape.hemi
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
        start : 
            Cylinder axis start point 
        end : 
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
        super(Cylinder, self).__init__(_smoldyn.PanelShape.cyl, name=name)
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
        center : 
            center of the disk.
        radius : float
            radius of the disk.
        vector: 
            A vector that points away from the front of the disk. The size of
            this vector is 2 for 2-D and 3 for 3-D.
        name : optional
            name
        """
        super(Disk, self).__init__(_smoldyn.PanelShape.disk, name=name)
        self.center = center
        self.radius = radius
        self.vector = vector
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, *self.vector]


class SurfaceFaceCollection(object):
    """Collection of faces of a surface"""

    def __init__(self, faces: List[str], surfname):
        self.faces: str = faces
        self.surfname = surfname

    def setStyle(
        self,
        color: Color = "",
        drawmode: str = "none",
        thickness: float = 1.0,
        stipplefactor: int = -1,
        stipplepattern: int = -1,
        shininess: int = -1,
    ):
        """Set drawing style for the face of surface.

        Parameters
        ----------
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
        color : Color
            color e.g. 'black', 'red', [0.1,0.1,0.1,1] etc.
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
        for which in self.faces:
            # TODO: Check on drawmode
            k = _smoldyn.setSurfaceStyle(
                self.surfname,
                _smoldyn.PanelFace.__members__[which],
                _smoldyn.DrawMode.__members__[drawmode],
                thickness,
                color,
                stipplefactor,
                stipplepattern,
                shininess,
            )
            assert k == _smoldyn.ErrorCode.ok, f"Failed to set drawing style {k}"

    def addAction(self, species: Union[Species, str], action: str, new_spec=None):
        """The behavior of molecules named species when they collide with this
        face of this surface.

        Parameters
        ----------
        species : Union[Species, str]
            Species. If `"all"`, then this action applies to all molecules.
        action : str
            The action can be  `“reflect”`, `“absorb”`, `“transmit”`, `“jump”`,
            `“port”`, or `“periodic”`.
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

        for which in self.faces:
            k = _smoldyn.setSurfaceAction(
                self.surfname,
                _smoldyn.PanelFace.__members__[which],
                sname,
                sstate,
                _smoldyn.SrfAction.__members__[action],
            )
            assert k == _smoldyn.ErrorCode.ok


class Surface(object):
    """Surfaces are infinitesimally thin structures that can be used to
    represent cell membranes, obstructions, system boundaries, or other things.

    Note
    -----
    They are 2D structures in 3D simulations, or 1D lines or
    curves in 2D simulations (or 0D points in 1D simulations).Each surface
    has a “front” and a “back” face, so molecules can interact differently
    with the two sides of a surface.Each surface is composed of one or more
    “:py:class:`~Panels`”, where each panels can be a rectangle, triangle, sphere,
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
            List of panels. A surface must have at least one.
        name : str 
            name of the surface

        Examples
        --------
        >>> r1 = S.Rectangle(corner=[100,0,0], dimensions=[100,100], axis='-x')
        >>> r2 = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='+y')
        >>> s = smoldyn.Surface("walls", panels=["r1", "r2"])
        """
        self.panels = panels
        self.name = name
        _smoldyn.addSurface(self.name)

        self.front = SurfaceFaceCollection(["front"], name)
        self.back = SurfaceFaceCollection(["back"], name)
        self.both = SurfaceFaceCollection(["front", "back"], name)
        self._addToSmoldyn()

    def _addToSmoldyn(self):
        # add panels
        assert self.name, "Surface name is missing"
        for i, panel in enumerate(self.panels):
            panel.name = panel.name if panel.name else str(i)
            k = _smoldyn.addPanel(
                self.name, panel.ctype, panel.name, panel.axisstr, panel.pts,
            )
            assert k == _smoldyn.ErrorCode.ok, f"Failed to add panel {self.name}, {k}"

    def setStyle(self, face, *args, **kwargs):
        """See the function :py:class:`~SurfaceFaceCollection.setStyle` for more
        details. This function forwards the the call to
        `SurfaceFaceCollection.setStyle` function.

        Parameters
        ----------
        face: str
            face of the surface: 'front', 'back', 'both'

        *args and **kwargs: 
            See :py:class:`~SurfaceFaceCollection.setStyle`

        See Also
        --------
        SurfaceFaceCollection.setStyle
        """
        assert face in ["front", "back", "both"]
        getattr(self, face).setStyle(*args, **kwargs)

    def addAction(self, face, *args, **kwargs):
        """See the function :py:class:`~SurfaceFaceCollection.addAction` for more
        details. This function forwards the the call to
        `SurfaceFaceCollection.addAction` function.

        Parameters
        ----------
        face: str
            face of the surface: 'front', 'back', 'both'

        *args and **kwargs: 
            See :py:func:`~SurfaceFaceCollection.addAction`

        See Also
        --------
        SurfaceFaceCollection.addAction
        """
        getattr(self, face).addAction(*args, *kwargs)

    def addMolecules(
        self,
        species: Species,
        mol: int,
        panels: List[Panel] = [],
        pos: List[float] = [],
    ):
        """Place molecules with random or specific positions onto a given
        surface (optionally specified panels).

        Parameters
        ----------
        species: Species
            Species to add.
        mol : int
            number of molecules
        surface : smoldyn.geometry.Surface
            Surface
        panels: smoldyn.geometry.Panel
            Panels of surface. If `None` or `[]` is given then all panels of
            surfece are used.
        pos: List[float]
            Position of the molecules. If not given, then randomly distribute
            the molecules onto the surface/panels.
        """
        assert mol > 0, "Needs a positive number"
        assert species
        panels = panels if panels else self.panels
        for panel in panels:
            assert panel
            assert panel.ctype
            print(111, panel.name, panel.ctype)
            k = _smoldyn.addSurfaceMolecules(
                species.name,
                species.state,
                mol,
                self.name,
                panel.ctype,
                panel.name,
                pos,
            )
            assert k == _smoldyn.ErrorCode.ok


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

    def __init__(self, name: str, surface: Union[Surface, str], panel: str):
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
        self.name: str = name
        self.surfname = surface.name if isinstance(surface, Surface) else surface
        assert panel in ["front", "back"]
        self.panel = _smoldyn.PanelFace.__members__[panel]
        k = _smoldyn.addPort(self.name, self.surfname, self.panel)
        assert k == _smoldyn.ErrorCode.ok


@dataclass
class Boundaries:
    """Boundaries may be reflective, transparent, absorbing, or periodic.
    Reflective means that all molecules that diffuse into a boundary will be
    reflected back into the system. Transparent, which is the default type,
    means that molecules just diffuse through the boundary as though it weren’t
    there. With absorbing boundaries, any molecule that touches a boundary is
    immediately removed from the system. Finally, with periodic boundaries,
    which are also called wrap-around or toroidal boundaries, any molecule that
    diffuses off of one side of space is instantly moved to the opposite edge
    of space; these are useful for simulating a small portion of a large system
    while avoiding edge effects.
    """

    low: List[float]
    high: List[float]
    types: List[str] = field(default_factory=lambda: ["r"])
    dim: field(init=False) = 0

    def __post_init__(self):
        assert len(self.low) == len(self.high), "Size mismatch."
        if len(self.types) == 1:
            self.types = self.types * len(self.low)
        self.dim = len(self.low)
        _smoldyn.setBoundaries(self.low, self.high)
        __logger__.debug("Getting boundaries", _smoldyn.getBoundaries())
        assert _smoldyn.getDim() == self.dim, (_smoldyn.getDim(), self.dim)
        if self.types:
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


class Compartment(object):
    """Compartment
    """

    def __init__(self, name: str, surface: Union[str, Surface], point: List[float]):
        """
        Parameters
        ----------
        name : str
            name of the compartment.
        surface : Union[str, Surface]
            Name of the bounding surface for this compartment.
        point : List[float]
            An interior-defining point for this compartment.
        """
        self.name = name
        self.srfname = surface.name if isinstance(surface, Surface) else surface
        self.point = point
        assert self.point, "At least one point is required."
        k = _smoldyn.addCompartment(self.name)
        assert k == _smoldyn.ErrorCode.ok
        k = _smoldyn.addCompartmentSurface(self.name, self.srfname)
        assert k == _smoldyn.ErrorCode.ok
        k = _smoldyn.addCompartmentPoint(self.name, self.point)
        assert k == _smoldyn.ErrorCode.ok

    def addMolecules(self, species: Species, mol: int):
        """Place molecules randomly in a compartment.

        Parameters
        ----------
        mol : int
            number of molecules
        species : Species
            Species to add

        See also
        --------
        :py:class:`smoldyn.kinetics.Species.addToCompartment`
        """
        assert mol > 0, "Needs a positive number"
        k = _smoldyn.addCompartmentMolecules(species.name, mol, self.name)
        assert k == _smoldyn.ErrorCode.ok


def setBounds(low: List[float], high: List[float], types: List[str] = []):
    """Define system volume by setting boundaries.

    Parameters
    ----------
    low : List[float]
        lower limit of axes. For example for x=0, y=-100, and z=0, use [0,-100,0].
    high :
        higher limit of axes e.g. for x=100, y=100, z=90, use [100,10,90]
    types : List[str]
        Boundary type. 'r' for reflexive, 't' for transparent, 'a' for
        absorbing, and 'p' for periodic boundary.

    See also
    --------
    smoldyn.setBoundaries, smoldyn.setBoundaryType
    """
    Boundaries(low, high, types)


class StateMonitor(object):
    """State Monitor
    """

    def __init__(self, objs, state, **kwargs):
        self.objs = objs
        self.state = state
        self.t = []
        self._start = kwargs.get("start", _smoldyn.getTimeStart())
        self._stop = kwargs.get("stop", _smoldyn.getTimeStop())
        self._step = kwargs.get("step", _smoldyn.getTimeStep())
        self._multiplier = kwargs.get("multiplier", 1.0)
        setattr(self, state, {})
        for o in objs:
            getattr(self, state)[o] = []
        if state == "molcount":
            self.initMolcount()

    def initMolcount(self):
        _smoldyn.addCommand(
            "i", self._start, self._stop, self._step, self._multiplier, "molcount"
        )

    def data(self):
        return _smoldyn.getData()


class Simulation(object):
    """Class to store simulation related attributes. 

    See also
    -------
    _smoldyn.simptr
    """

    def __init__(self, stop: float, step: float, quitatend: bool = False, **kwargs):
        """
        Parameters
        ----------
        stop : float
            Simulation stop time (sec)
        step : float
            Simulation step or dt (sec)
        quitatend : bool
            If `True`, Smoldyn won't prompt user at the end of simulation and
            quit. Same effect can also be achieved by setting environment variable 
            `SMOLDYN_NON_INTERACTIVE` to 1.
        kwargs :
            kwargs
        """
        self.start = kwargs.get("start", 0.0)
        self.stop = stop
        self.step = step
        self.simptr = _smoldyn.getCurSimStruct()
        assert self.simptr, "Configuration is not initialized"
        if kwargs.get("accuracy", 0.0):
            self.accuracry: float = kwargs["accuracy"]
        self.quitatend = quitatend
        # TODO: Add to documentation.
        if os.getenv("SMOLDYN_NON_INTERACTIVE", ""):
            self.quitatend = True

    def setGraphics(
        self,
        method: str,
        iter: int = 20,
        delay: int = 0,
        bg_color: Color = "white",
        frame_thickness: int = 2,
        frame_color: Color = "black",
        grid_thickness: int = 0,
        grid_color: Color = "white",
    ):
        """Set graphics parameters for this simulation.

        Parameters
        ----------
        method : str
            Avaibale options: "none", "opengl", "opengl_good", "opengl_better"
        iter : int
            Update graphics after every nth step (default 20)
        delay : int
            Additional delay between rendering
        bg_color: Color
            Background color
        frame_thickness: int
            Thickness of the frame that is drawn around the simulation volume,
            in pts. Default value is 2.
        frame_color: Color
            Specify the color of the frame. Default value is 'black'
        grid_thickness: int
            Thickness of the grid lines that can be drawn to show the virtual
            boxes. Default value is 0, so that thegrid is not drawn.
        grid_color: Color
            Color of the grid. Default "white" or [1,1,1,1]
        """
        k = _smoldyn.setGraphicsParams(method, iter, delay)
        assert k == _smoldyn.ErrorCode.ok
        k = _smoldyn.setBackgroundStyle(bg_color)
        assert k == _smoldyn.ErrorCode.ok
        k = _smoldyn.setFrameStyle(frame_thickness, frame_color)
        assert k == _smoldyn.ErrorCode.ok

        if grid_thickness > 0 and grid_color != bg_color:
            k = _smoldyn.setGridStyle(grid_thickness, grid_color)
            assert k == _smoldyn.ErrorCode.ok

    def setTiff(
        self, tiffname: Path = "OpenGL", minsuffix: int = 1, maxsuffix: int = 999
    ):
        """TIFF related parameters.

        Parameters
        ----------
        tiffname : str
            Root filename for TIFF files, which may include path information if
            desired.  If the parent directory does not exists, it will be created.
            Default is “OpenGL”, which leads to the first TIFF being saved as
            “OpenGL001.tif”.
        minsuffix : int
            Initial suffix number of TIFF files that are saved.  Default value
            is 1.
        maxsuffix : int
            Largest possible suffix number of TIFF files that are saved.  Once
            this value has been reached, additional TIFFs cannot be saved.
            Default value is 999.
            
        """
        try:
            Path(tiffname).parent.mkdir(parents=True, exist_ok=True)
        except Exception as e:
            __logger__.warning(e)
        k = _smoldyn.setTiffParams(tiffname, minsuffix, maxsuffix)
        assert k == _smoldyn.ErrorCode.ok

    def setLight(
        self,
        index: int,
        ambient: Color,
        diffuse: Color,
        specular: Color,
        pos: List[float],
    ):
        """Set the parameters for a light source, for use with opengl_better
        quality graphics. Parameters “ambient”, “diffuse”, “specular” are for
        the light’s colors, which are then specified with either a word or in
        the values as red, green, blue, and optionally alpha.

        Parameters
        ----------
        index : int
            The light index should be between 0 and 7.
        ambient : Color
            ambient
        diffuse : Color
            diffuse
        specular : Color
            specular
        pos : List[float]
            Light’s 3-dimensional position, which is specified as x, y,and zin
            the values. Lights specified this way are automatically enabled
            (turned on).
        """
        ambient = color2RGBA(ambient) if isinstance(ambient, str) else ambient
        diffuse = color2RGBA(diffuse) if isinstance(diffuse, str) else diffuse
        specular = color2RGBA(specular) if isinstance(specular, str) else specular
        k = _smoldyn.setLightParams(index, ambient, diffuse, specular, pos)
        assert k == _smoldyn.ErrorCode.ok

    @property
    def quitatend(self):
        return self.simptr.quitatend

    @quitatend.setter
    def quitatend(self, val: bool):
        self.simptr.quitatend = val

    @property
    def accuracy(self):
        return _smoldyn.getAccuracy()

    @accuracy.setter
    def accuracy(self, accuracy: float):
        # Deperacted?
        warnings.DeprecationWarning("accuracy is deprecated?")
        _smoldyn.setAccuracy(accuracy)

    def run(self, stop=None, start=None, step=None):
        if stop is not None:
            self.stop = stop
        if start is not None:
            self.start = start
        if step is not None:
            self.step = step
        print(f"[INFO] Running till {self.stop} dt={self.step}")
        _smoldyn.run(self.stop, self.step)

    def runUntil(self, t, dt):
        _smoldyn.runUntil(t, dt)

    def data(self):
        return _smoldyn.getData()
