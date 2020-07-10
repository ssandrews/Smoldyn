"""Smoldyn user API.
"""

from __future__ import annotations

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2020-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

__all__ = [
    "__version__",
    "version",
    "Species",
    "Panel",
    "Triangle",
    "Rectangle",
    "Sphere",
    "Hemisphere",
    "Cylinder",
    "Disk",
    "Surface",
    "Port",
    "Boundaries",
    "Partition",
    "MoleculePerBox",
    "Box",
    "Compartment",
    "StateMonitor",
    "Command",
    "Simulation",
    "Reaction",
    "setBounds",
]

import os
import operator
import functools
from pathlib import Path
from dataclasses import dataclass, field
from smoldyn.config import __logger__
from smoldyn.utils import color2RGBA
from typing import Union, Tuple, List, Dict
from smoldyn import _smoldyn

# Smoldyn version
__version__ = _smoldyn.__version__
version = lambda: __version__

# Color type.
# Either a string such as 'black', 'red', 'orange' or a tuple of 4 values such
# as [1,1,0,1] specifying RGBA value.
ColorType = Union[str, Tuple[float, float, float], Tuple[float, float, float, float]]
DiffConst = Union[float, Dict[str, float]]


class Color:
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


def _toMS(st: Union[str, _smoldyn.MolecState]) -> _smoldyn.MolecState:
    """Convert a string to equivalent MolecState.

    Parameters
    ----------
    st : Tuple[str, _smoldyn.MolecState]
        Given state either a string or MolecState. If MolecState is given, this
        function simply returns it.
    """
    return _smoldyn.MolecState.__members__[st] if isinstance(st, str) else st


class Species(object):
    """Chemical species.
    """

    def __init__(
        self,
        name: str,
        state: str = "soln",
        color: Union[ColorType, Dict[str, ColorType]] = {"soln": "black"},
        difc: Union[float, Dict[str, float]] = 0.0,
        display_size: float = 3,
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
        display_size : float, optional
            display size of the molecule. For the “opengl” graphics level, the
            display size value is in pixels.  Here, numbers from 2 to 4 are
            typically good choices (deafult 2px).  For the two better graphics
            options, the display size value is the radius with which the
            molecule is drawn, using the same units as elsewhere in the input
            file.
        mol_list : str, optional
            molecule list (default '')

        """
        self.name: str = name
        assert self.name

        self._displaySize: Dict[str, float] = {}
        self._color: Dict[str, Color] = {}
        self._difc: Dict[str, float] = {}
        self._mol_list: str = ""

        k = _smoldyn.addSpecies(self.name)
        assert k == _smoldyn.ErrorCode.ok, "Failed to add molecule"

        if state not in _smoldyn.MolecState.__members__:
            raise NameError(
                f"{state} is not a valid MolecState. Available "
                "states are:{', '.join(_smoldyn.MolecState.__members__.keys())}"
            )

        self.state = state

        # Prepare diffusion constant dict (for state).
        if not isinstance(difc, dict):
            difc = {self.state: difc}
        self.difc = difc

        # assign color
        self.color = color
        self.display_size = display_size
        if mol_list:
            self.mol_list: str = mol_list

    def __repr__(self):
        return f"<Molecule: {self.name}, difc={self.difc}, state={self.state}>"

    def __to_disp__(self):
        return f"{self.name}({self.state})"

    def setStyle(
        self,
        display_size: Union[float, Dict[str, float]],
        color: Union[ColorType, Dict[str, ColorType]],
    ):
        self.color = color
        self.display_size = display_size

    @property
    def difc(self) -> DiffConst:
        return self._difc

    @difc.setter
    def difc(self, difconst: DiffConst):
        """Isotropic Diffusion coeffiecient of the Species.

        Parameters
        ----------
        difconst : DiffConst
            Default value is 0. If a single value is given, all states of the
            molecule are assigned the same value. To assign state specific
            values, use a dictionary. Missing states will be assigned 0.0.
        """
        self._difc = (
            {self.state: difconst} if not isinstance(difconst, dict) else difconst
        )
        for state, D in self._difc.items():
            st = (
                _smoldyn.MolecState.__members__[state]
                if isinstance(state, str)
                else state
            )
            k = _smoldyn.setSpeciesMobility(self.name, st, D)
            assert k == _smoldyn.ErrorCode.ok

    @property
    def mol_list(self) -> str:
        return self._mol_list

    @mol_list.setter
    def mol_list(self, val):
        k = _smoldyn.addMolList(val)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add mollist: {k}"
        k = _smoldyn.setMolList(self.name, _toMS(self.state), val)
        assert (
            k == _smoldyn.ErrorCode.ok
        ), f"Failed to set mol_list={val} on {self}: {k}"
        self._mol_list = val

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, clr: Union[ColorType, Color]):
        self._color = {self.state: Color(clr)} if not isinstance(clr, dict) else clr
        for state, _clr in self._color.items():
            _clr = Color(_clr)
            state = (
                _smoldyn.MolecState.__members__[state]
                if isinstance(state, str)
                else state
            )
            k = _smoldyn.setMoleculeColor(self.name, state, _clr.rgba)
            assert k == _smoldyn.ErrorCode.ok

    @property
    def display_size(self):
        return self._displaySize

    @display_size.setter
    def display_size(self, size: Union[float, Dict[str, float]]):
        """Set the display_size of the molecule. 

        Parameters
        ----------
        size : Union[float, Dict[str, float]]
            If a single value is provided, it is applied to all states of the
            moelcule. For state specific value of display_size, use a
            dictionary.

        Note
        -----
        For the “opengl” graphics level, the display size value is in pixels.
        Here, numbers from 2 to 4 are typically good choices (deafult 2px).
        For the two better graphics options, the display size value is the
        radius with which the molecule is drawn, using the same units as
        elsewhere in the input file.
        """
        if isinstance(size, (float, int)):
            size = {_smoldyn.MolecState.all: size}
        self._displaySize = size
        for state, size in self._displaySize.items():
            k = _smoldyn.setMoleculeSize(self.name, state, size)
            assert k == _smoldyn.ErrorCode.ok

    def addToSolution(
        self,
        N: int,
        fixed: List[float] = [],
        highpos: List[float] = [],
        lowpos: List[float] = [],
    ):
        """Add molecule to solution.

        Parameters
        ----------
        N : float
            Number of molecules
        fixed : List[float]
            Fixed location. If given, both `lowpos` and `highpos` will be set
            to this value.
        highpos : List[float]
            Upper bound on the molecules location. Molecules will be distributed
            uniformly between the lowerbound and this value.
        lowpos : List[float]
            Lower bound on the molecules location. Molecules will be distributed
            uniformly between the upper bound and this value.
        """
        assert (
            self.state == "soln"
        ), f"You can't use this function on a Species with type {self.state}"
        if fixed:
            lowpos = highpos = fixed
        k = _smoldyn.addSolutionMolecules(self.name, N, lowpos, highpos)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add to solution: {k}"


@dataclass
class NullSpecies(Species):
    name: str = ""
    state = _smoldyn.MolecState.__members__["none"]

    def __len__(self):
        return 0


# Type of a substract and product of a Reaction
SpeciesState = Union[_smoldyn.MolecState, str]
SpeciesWithState = Union[Species, Tuple[Species, SpeciesState]]


class Panel(object):
    def __init__(
        self,
        name: str = "",
        shape: _smoldyn.PanelShape = _smoldyn.PanelShape.none,
        neighbors: List[Panel] = [],
    ):
        """Panels are components of a surface. One or more Panels are requried
        to form a Surface. Following geometric primitives are available.

        - [Rectangle](smoldyn.smoldyn.Rectangle)
        - [Triangle](smoldyn.smoldyn.Triangle)
        - [Sphere](smoldyn.smoldyn.Sphere)
        - [Hemisphere](smoldyn.smoldyn.Hemisphere)
        - [Cylinder](smoldyn.smoldyn.Cylinder)
        - [Disk](smoldyn.smoldyn.Disk)
        """

        self._neighbors: List[Panel] = []

        self.name = name
        self.ctype: _smoldyn.PanelShape = shape
        self.pts: List[float] = []
        self.front = _PanelFace("front", self)
        self.back = _PanelFace("back", self)

        self.surface: Surface = NullSurface()
        self.neighbors = neighbors

    def jumpTo(
        self, face1: str, panel2: Panel, face2: str, bidirectional: bool = False
    ):
        """Add a jump reaction between two panels of the same surface.

        This panel has name panel1, and face face1.  A molecule that hits this
        face of the panel, and that has “jump” action for this face, gets
        translated to the face face2 of the panel panel2 (which needs to be the
        same shape as the originating panel). 

        Parameters
        ----------
        face1 : Panel
            
        panel2 : to this Panel
        face2 : to this face

        Examples
        --------
        
        Smoldyn expression: 
            jump r1 front <-> r2 front 

        >>> r1.addJump('front', r2, 'front', True)   

        Or, 

        >>> r1.front.jumpTo(r2.front, True)
        """
        return getattr(self, face1).jumpTo(getattr(panel2, face2))

    def __str__(self):
        return f"<{self.name} type={self.ctype} index={self.index}>"

    def _axisIndex(self, axisname: str):
        axisDict = dict(x=0, y=1, z=2)
        return int(axisDict.get(axisname.lower(), axisname))

    @property
    def index(self) -> int:
        return _smoldyn.getPanelIndexNT(self.surface.name, self.name)

    def toText(self):
        return f"panel {self.ctype} {self.axisstr} {' '.join(map(str,self.pts))} {self.name}".strip()

    @property
    def neighbors(self):
        return self._neighbors

    @neighbors.setter
    def neighbors(self, panels: List[Panel]):
        self._neighbors = panels
        for panel in panels:
            self._assignNeighbor(panel)

    @property
    def neighbor(self):
        return self._neighbors[0]

    @neighbor.setter
    def neighbor(self, panel: Panel):
        self._assignNeighbor(panel)

    def _assignNeighbor(self, panel, reciprocal: bool = False):
        assert self.surface, "This panel has no Surface"
        assert panel.surface, "This panel has no Surface"
        assert self != panel, "A panel cannot be its own neighbor"
        k = _smoldyn.addPanelNeighbor(
            self.surface.name, self.name, panel.surface.name, panel.name, reciprocal
        )
        assert k == _smoldyn.ErrorCode.ok

    def setNeighbors(self, panels, reciprocal: bool = False):
        """Set neighbors.

        Parameters
        ----------
        panels :
            Neighboring panels
        reciprocal : bool
            If True, this panel also becomes neighbor of all panels in arguments.

        See Also
        --------
        Panel.neighbor, Panel.neighbors
        """
        for panel in panels:
            self._assignNeighbor(panel, reciprocal)


class Rectangle(Panel):
    def __init__(
        self, corner: List[float], dimensions: List[float], axis: str, name=""
    ):
        """Rectangle

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
        super().__init__(shape=_smoldyn.PanelShape.rect, name=name)
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
        super().__init__(shape=_smoldyn.PanelShape.tri, name=name)
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
        super().__init__(shape=_smoldyn.PanelShape.sph, name=name)
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
        super().__init__(shape=_smoldyn.PanelShape.hemi, name=name)
        self.center = center
        self.radius = radius
        self.vector = vector
        self.slices = slices
        self.stacks = stacks
        # Smoldyn Panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, *self.vector, self.slices, self.stacks]


class Cylinder(Panel):
    def __init__(
        self,
        start: List[float],
        end: List[float],
        radius: float,
        slices: int,
        stacks: int,
        name: str = "",
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
        super().__init__(shape=_smoldyn.PanelShape.cyl, name=name)
        self.start: List[float] = start
        self.end: List[float] = end
        self.radius: float = radius
        self.slices: int = slices
        self.stacks: int = stacks
        # Smoldyn panel
        self.axisstr: str = ""
        self.pts = [*self.start, *self.end, self.radius, self.slices, self.stacks]


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
        super().__init__(shape=_smoldyn.PanelShape.disk, name=name)
        self.center = center
        self.radius = radius
        self.vector = vector
        # Smoldyn panel
        self.axisstr = ""
        self.pts = [*self.center, self.radius, *self.vector]


class _PanelFace(object):
    def __init__(self, name: str, panel: Panel):
        assert name in ["front", "back"]
        self.name = _smoldyn.PanelFace.__members__[name]
        self.panel = panel

    def jumpTo(self, toface: _PanelFace, bidirectional: bool = False):
        """Add a jump reaction between two panels of same Surface

        See also
        --------
        Panel.jumpTo
        """
        assert (
            self.panel.ctype == toface.panel.ctype
        ), "Both panels must have same geometry"
        assert (
            self.panel.surface == toface.panel.surface
        ), "Both panels must have same surface"
        ## print(111, self.panel.surface.name, self.panel.name
        ##         , self.name, toface.panel.name,
        ##         toface.name, bidirectional)
        k = _smoldyn.setPanelJump(
            self.panel.surface.name,
            self.panel.name,
            self.name,
            toface.panel.name,
            toface.name,
            bidirectional,
        )
        assert k == _smoldyn.ErrorCode.ok


class _SurfaceFaceCollection(object):
    def __init__(self, faces: List[str], surfname: str):
        """Collection of faces of a surface"""
        self.faces: List[str] = faces
        self.surfname: str = surfname

    def setStyle(
        self,
        color: ColorType = "",
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
            # TODO : Check on drawmode
            k = _smoldyn.setSurfaceStyle(
                self.surfname,
                _smoldyn.PanelFace.__members__[which],
                _smoldyn.DrawMode.__members__[drawmode],
                thickness,
                Color(color).rgba,
                stipplefactor,
                stipplepattern,
                shininess,
            )
            assert k == _smoldyn.ErrorCode.ok, f"Failed to set drawing style {k}"

    def addAction(
        self, species: Union[Species, List[Species]], action: str, new_spec=None
    ):
        """The behavior of molecules named species when they collide with this
        face of this surface.

        Parameters
        ----------
        species :
            List of species or a single species. DO NOT use the name of
            species.
        action : str
            The action can be  `“reflect”`, `“absorb”`, `“transmit”`, `“jump”`,
            `“port”`, or `“periodic”`.
        new_spec: str
            TODO: Implement it.
            If `new_spec` is entered, then the molecule changes to this new
            species upon surface collision. In addition, it’s permissible to
            enter the action as “multiple,” in which case the rates need to be
            set with rate; alternatively, just setting the rates will
            automatically set the action to “multiple.” The default is
            transmission for all molecules.
        """
        assert action in ["reflect", "absorb", "transmit", "jump", "port", "periodic"]
        if isinstance(species, str):
            raise NameError(
                "This API does not accepts species by their names."
                " Please pass the objects."
            )
        listSpecies = [species] if isinstance(species, Species) else species
        for sp in listSpecies:
            if isinstance(sp, Species):
                sname, sstate = sp.name, sp.state
            else:
                sname, sstate = sp, _smoldyn.MolecState.soln

            if isinstance(sstate, str):
                sstate = _smoldyn.MolecState.__members__[sstate]

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

        self.front = _SurfaceFaceCollection(["front"], name)
        self.back = _SurfaceFaceCollection(["back"], name)
        self.both = _SurfaceFaceCollection(["front", "back"], name)
        self._addToSmoldyn()

    def _addToSmoldyn(self):
        # add panels
        assert self.name, "Surface name is missing"
        for i, panel in enumerate(self.panels):
            panel.name = panel.name if panel.name else f"panel{i}"
            panel.surface = self
            k = _smoldyn.addPanel(
                self.name, panel.ctype, panel.name, panel.axisstr, panel.pts,
            )
            assert k == _smoldyn.ErrorCode.ok, f"Failed to add panel {self.name}, {k}"

    def setStyle(self, face, *args, **kwargs):
        """See the function [setStyle](_SurfaceFaceCollection.setStyle) for more
        details. This function forwards the call to `_SurfaceFaceCollection.setStyle`.

        Parameters
        ----------
        face: str
            face of the surface: 'front', 'back', 'both'

        *args and **kwargs: 
            See `_SurfaceFaceCollection.setStyle`

        See Also
        --------
        _SurfaceFaceCollection.setStyle
        """
        assert face in ["front", "back", "both"]
        getattr(self, face).setStyle(*args, **kwargs)

    def addAction(self, face, *args, **kwargs):
        """See the function `_SurfaceFaceCollection.addAction` for more
        details. This function forwards the the call to
        `_SurfaceFaceCollection.addAction` function.

        Parameters
        ----------
        face: str
            face of the surface: 'front', 'back', 'both'

        *args and **kwargs: 
            See `_SurfaceFaceCollection.addAction`

        See Also
        --------
        _SurfaceFaceCollection.addAction
        """
        getattr(self, face).addAction(*args, *kwargs)

    def addMolecules(
        self,
        species: SpeciesWithState,
        N: int,
        panels: List[Panel] = [],
        pos: List[float] = [],
    ):
        """Place molecules with random or specific positions onto a given
        surface (optionally specified panels).

        Parameters
        ----------
        species: 
            Species to add, a Species or a tuple of (Species, MolecState)
            e.g. (A, 'front')
        N : int
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
        assert N > 0, "Expected a positive number"
        assert species
        if isinstance(species, Species):
            sname = species.name
            sstate = species.state
        else:
            assert len(species) == 2, "Expected tuple of (Species, MolecState)"
            sname = species[0].name
            sstate = _toMS(species[1])
        panels = panels if panels else self.panels

        # Distribute molecules equally among the panels.
        _N = N // len(panels)
        Ns = [_N] * len(panels)
        Ns[0] += N - sum(Ns)
        assert sum(Ns) == N

        for panel, _n in zip(panels, Ns):
            assert panel
            assert panel.ctype
            k = _smoldyn.addSurfaceMolecules(
                sname, sstate, _n, self.name, panel.ctype, panel.name, pos,
            )
            assert k == _smoldyn.ErrorCode.ok


class NullSurface(Surface):
    def __init__(self):
        self.name = ""
        self.panels = []


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
    types: Union[str, List[str]] = field(default_factory=lambda: ["r"])
    dim: int = 0

    def __post_init__(self):
        assert len(self.low) == len(self.high), "Size mismatch."
        if len(self.types) == 1:
            self.types = self.types * len(self.low)
        self.dim = len(self.low)
        _smoldyn.setBoundaries(self.low, self.high)
        __logger__.debug("Getting boundaries", _smoldyn.getBoundaries())
        assert _smoldyn.getDim() == self.dim, (_smoldyn.getDim(), self.dim)
        for _d, _t in zip(range(self.dim), self.types):
            k = _smoldyn.setBoundaryType(_d, -1, _t)
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
    def __init__(self, name: str, surface: Union[str, Surface], point: List[float]):
        """
        Comapartment.

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

    def addMolecules(self, species: Species, N: int):
        """Place number of molecules in a compartment (uniformly distributed)

        Parameters
        ----------
        N : int
            number of molecules
        species : Species
            Species to add

        See also
        --------
        :py:class:`smoldyn.kinetics.Species.addToCompartment`
        """
        assert N > 0, "Needs a positive number"
        k = _smoldyn.addCompartmentMolecules(species.name, N, self.name)
        assert k == _smoldyn.ErrorCode.ok


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


class Command(object):
    """Smoldyn command
    """

    def __init__(self, cmd: str, type: str = "", **kwargs):
        """
        Parameters
        ----------
        cmd : str
            command string.
        type : str
            Type of command. When `from_string` is set to `True`, we expect the the type to
            be included in the `cmd` string.

            Use capital letter version for integer queue.
            - ‘b’ or 'B' for before the simulation
            - ‘a’ or 'A' for after the simulation,
            - ‘e’ or 'E' for every time step during the simulation
            - ‘@’ or '&' for a single command execution at time ``time``
            - ‘n’ or 'N' for every n’th iteration of the simulation
            - ‘i’ or 'I' for a fixed time interval. The command is executed
              over the period from on to off with intervals of at least dt(the
              actual intervals will only end at the times of simulation time
              steps).
            - ‘x’ for a fixed time multiplier. The command is executed at on,
              then on+dt, then on+dt*xt, then on+dt*xt2, and so forth.
            - ‘j’ for every ``dtit`` step with a set starting iteration and
              stopping iteration.
        kwargs :
            from_string: False
                If True, use plain text string to generate the command.

        Notes
        -----
        See section 2.4 for the commands that are available.
        """
        self.__allowed_cmd_type__ = "@&aAbBeEnNiIjx"
        self.type = type
        self.on = 0.0
        self.off = 0.0
        self.step = 0.0
        self.multiplier = 0.0
        self.cmd = cmd
        self.kwargs = dict(kwargs)
        self.fromString = self.kwargs.get("from_string", False)
        self.__added__ = False
        # self.__finalize__()

    def __finalize__(self):
        # Intialize the command.
        assert (
            self.type in self.__allowed_cmd_type__
        ), f"command type {self.type} not supported."
        if self.fromString:
            k = _smoldyn.addCommandFromString(self.cmd)
            assert k == _smoldyn.ErrorCode.ok
            return
        if "@" == self.type:
            assert (
                self.kwargs.get("time", 0.0) >= 0.0
            ), "Must specifiy argument time greater than 0"
            self.on = self.off = self.kwargs["time"]
        elif self.type in "aAbBeE":
            pass
        elif self.type in "nN":
            assert (
                self.kwargs.get("step", 0.0) > 0.0
            ), "Must specify argument step greater than 0"
            self.step = self.kwargs["step"]
        elif self.type in "ixjI":
            assert self.kwargs.get("on", None) is not None, (
                "Missing argument on",
                self.kwargs,
            )
            assert self.kwargs.get("off", None), "Missing argument off"
            assert self.kwargs.get("step", None), "Missing argument step"
            self.on = self.kwargs["on"]
            self.off = self.kwargs["off"]
            self.step = self.kwargs["step"]
            if "x" == self.type:
                assert self.kwargs.get(
                    "multiplier", None
                ), "Missing argument multiplier"
                self.multiplier = self.kwargs["multiplier"]
        else:
            raise RuntimeError(f"Command type {self.type} is unsupported")
        self.__add_command__()
        self.__added__ = True

    def __add_command__(self):
        k = _smoldyn.addCommand(
            self.type, self.on, self.off, self.step, self.multiplier, self.cmd
        )
        assert k == _smoldyn.ErrorCode.ok


class Simulation(object):
    """Class to store simulation related attributes. 

    See also
    -------
    _smoldyn.simptr
    """

    def __init__(self, stop: float, step: float, quitAtEnd: bool = False, **kwargs):
        """
        Parameters
        ----------
        stop : float
            Simulation stop time (sec)
        step : float
            Simulation step or dt (sec)
        quitAtEnd : bool
            If `True`, Smoldyn won't prompt user at the end of simulation and
            quit. Same effect can also be achieved by setting environment variable 
            `SMOLDYN_NON_INTERACTIVE` to 1.
        kwargs :
            output_files :
                Declare output files.
        """
        self.start = kwargs.get("start", 0.0)
        self.stop = stop
        self.step = step
        self.simptr = _smoldyn.getCurSimStruct()
        self.commands: List[Command] = []
        self.kwargs = kwargs
        assert self.simptr, "Configuration is not initialized"
        if self.kwargs.get("accuracy", 0.0):
            self.accuracry: float = kwargs["accuracy"]

        if self.kwargs.get("output_files", []):
            self.setOutputFiles(self.kwargs["output_files"])

        # TODO : Add to documentation.
        self.quitAtEnd = quitAtEnd
        if os.getenv("SMOLDYN_NON_INTERACTIVE", ""):
            self.quitAtEnd = True

    def setOutputFiles(self, outfiles: List[str], append=True):
        """Declaration of filenames that can be used for output of simulation
        results.  Spaces are not permitted in these names.  Any previous files
        with the same name will be overwritten.

        Parameters
        ----------
        outfiles : List[str]
            Output files.  If all files don't have the same parent directory,
            last file's parent directory is used.
        append : bool
            If `True`, append the data to the exsiting file. Default `False`.

        See also
        --------
        setOutputFile
        """
        for outfile in outfiles:
            self.setOutputFile(outfile, append)

    def setOutputFile(self, outfile, append: bool = False):
        """Declaration of filenames that can be used for output of simulation
        results.  Spaces are not permitted in these names.  Any previous files
        with the same name will be overwritten.

        Parameters
        ----------
        outfile : Union[str, Path]
            Output file. 
        append : bool
            If `True`, append the data to the exsiting file. Default `False`.

        See also
        --------
        setOutputFiles
        """
        outfile = Path(outfile).resolve()
        if outfile.parent != Path("."):
            _smoldyn.setOutputPath(str(outfile.parent) + "/")
        _smoldyn.addOutputFile(outfile.name, False, append)

    def __finalize_cmds__(self):
        # Add commands to smoldyn engine.
        for cmd in self.commands:
            if cmd.__added__:
                continue
            cmd.__finalize__()

    def setGraphics(
        self,
        method: str,
        iter: int = 20,
        delay: int = 0,
        bg_color: ColorType = "white",
        frame_thickness: int = 2,
        frame_color: ColorType = "black",
        grid_thickness: int = 0,
        grid_color: ColorType = "white",
        text_display: Union[List, str] = "",
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
        text_display: 
            List of variables to be displayed. Or a text string containing
            variable names e.g. 'time E S ES(front)'
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

        self.__set_text_display__(text_display)

    # alias for setGraphics
    addGraphics = setGraphics

    @classmethod
    def __todisp_text__(self, x):
        if isinstance(x, str):
            return x
        return x.__to_disp__()

    def __set_text_display__(self, text_display):
        if isinstance(text_display, str):
            text_display = text_display.strip().split(" ")
        for item in text_display:
            if not item:
                continue
            k = _smoldyn.addTextDisplay(self.__todisp_text__(item))
            assert k == _smoldyn.ErrorCode.ok, f"Failed to set '{item}'"

    def setTiff(
        self, tiffname: Path = Path("OpenGL"), minsuffix: int = 1, maxsuffix: int = 999
    ):
        """TIFF related parameters.

        Parameters
        ----------
        tiffname : str, Path
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
        ambient = Color(ambient).rgba
        diffuse = Color(diffuse).rgba
        specular = Color(specular).rgba
        k = _smoldyn.setLightParams(index, ambient, diffuse, specular, pos)
        assert k == _smoldyn.ErrorCode.ok

    @property
    def quitAtEnd(self):
        return self.simptr.quit_at_end

    @quitAtEnd.setter
    def quitAtEnd(self, val: bool):
        self.simptr.quit_at_end = val

    @property
    def accuracy(self):
        return _smoldyn.getAccuracy()

    @accuracy.setter
    def accuracy(self, accuracy: float):
        _smoldyn.setAccuracy(accuracy)

    @property
    def data(self):
        return _smoldyn.getData()

    def run(self, stop=None, start=None, step=None):
        if stop is not None:
            self.stop = stop
        if start is not None:
            self.start = start
        if step is not None:
            self.step = step

        self.__finalize_cmds__()
        _smoldyn.run(self.stop, self.step)

    def runUntil(self, t, dt):
        self.__finalize_cmds__()
        _smoldyn.runUntil(t, dt)

    def addCommand(self, cmd: str, type: str, **kwargs):
        """Add command.

        Parameters
        ----------
        cmd : str
            Command string.
        type : str
            Type of command. When `from_string` is set to `True`, we expect the the type to
            be included in the `cmd` string.

            Use capital letter version for integer queue.
            - ‘b’ or 'B' for before the simulation
            - ‘a’ or 'A' for after the simulation,
            - ‘e’ or 'E' for every time step during the simulation
            - ‘@’ or '&' for a single command execution at time ``time``
            - ‘n’ or 'N' for every n’th iteration of the simulation
            - ‘i’ or 'I' for a fixed time interval. The command is executed
              over the period from on to off with intervals of at least dt(the
              actual intervals will only end at the times of simulation time
              steps).
            - ‘x’ for a fixed time multiplier. The command is executed at on,
              then on+dt, then on+dt*xt, then on+dt*xt2, and so forth.
            - ‘j’ for every ``dtit`` step with a set starting iteration and
              stopping iteration.
        kwargs :
            kwargs of :func:`Command <smoldyn.smoldyn.Class.__init__>`.
        """
        c = Command(cmd, type, from_string=False, **kwargs)
        self.commands.append(c)

    def addCommandFromString(self, cmd: str):
        """Add command using a single string. See the Smoldyn's User Manual for
        details.

        Parameters
        ----------
        cmd : str
            Command string
        """
        c = Command(cmd, from_string=True)
        self.commands.append(c)


class HalfReaction(object):

    # Shortcodes for types (fixme: deprecated?)
    __typedict__ = dict(
        i="irrev",
        p="pgem",
        x="pgemmax",
        y="pgemmax2",
        r="ratio",
        b="unbindrad",
        q="pgem2",
        s="ratio2",
        o="offset",
        f="fixed",
    )

    def __init__(
        self,
        name: str,
        subs: List[SpeciesWithState],
        prds: List[SpeciesWithState],
        rate: float,
        compartment: Compartment = None,
        surface: Surface = None,
    ):
        """Half Reaction (only occurs in forwared direction).

        Parameters
        ----------
        name: str
            name of the reaction.
        subs : List[SpeciesWithState]
            List of rectants.
        prds : List[SpeciesWithState]
            List of products.
        rate : float
            rate of the reaction
        compartment: Compartment
            If not ``None``, restrict the reaction to this compartment.
        surface: Surface
            If not ``None``, restricts this reaction to this surface.
        """

        self.name = name
        self.rate = rate
        self.compartment = compartment
        self.surface = surface

        assert len(subs) < 3, "At most two reactants are supported."
        if subs is None or len(subs) == 0:
            assert len(prds) > 0, "At least one product for a zero-order reaction."
            subs = [NullSpecies()]
        r1 = subs[0]
        r2 = subs[1] if len(subs) == 2 else NullSpecies()

        if isinstance(r1, Species):
            r1name, r1state = r1.name, _toMS(r1.state)
        else:
            assert len(r1) == 2, "Expected tuple of (Species, state) e.g. (A, 'front')"
            r1name = r1[0].name
            r1state = _toMS(r1[1])

        if isinstance(r2, Species):
            r2name, r2state = r2.name, _toMS(r2.state)
        else:
            assert len(r2) == 2, "Expected tuple of (Species,state) e.g. (S, 'back')"
            r2name, r2state = r2[0].name, _toMS(r2[1])

        prdNames, prdStates = [], []
        for x in prds:
            if isinstance(x, Species):
                prdNames.append(x.name)
                prdStates.append(_toMS(x.state))
            else:
                prdNames.append(x[0].name)
                prdStates.append(_toMS(x[1]))

        if not name:
            name = "r%d" % id(self)
        k = _smoldyn.addReaction(
            name, r1name, r1state, r2name, r2state, prdNames, prdStates, rate,
        )
        if k != _smoldyn.ErrorCode.ok:
            __logger__.warning(f" Substrates  : {subs}")
            __logger__.warning(f" Products  : {prds}")
            raise RuntimeError(f"Failed to add reaction: {k}")

        if self.compartment is not None or self.surface is not None:
            cname = self.compartment.name if self.compartment else ""
            sname = self.surface.name if self.surface else ""
            assert cname or sname
            k = _smoldyn.setReactionRegion(self.name, cname, sname)
            assert k == _smoldyn.ErrorCode.ok

    def setProductPlacement(
        self, type: str, param: float = 0.0, product: str = None, pos: List[float] = []
    ):
        """Placement method and parameters for the products of reaction.
        This also affects the binding radius of the reverse reaction, as
        explained in the manual. 

        Parameters
        ----------
        type : 'irrev' ('i'), 'pgem' ('p'), 'pgemmax' ('x')
            , 'pgemmax2' ('y'), 'ratio' ('r'), 'unbindrad' ('b')
            , 'pgem2' ('q'), 'ratio2' ('s'), 'offset' ('o'), 'fixed' ('f')
        param : float
            Parameter value. Usually required except for type 'irrev'
        product: str, optional
            Required for type 'fixed' and 'offset'
        pos: 
            Required for type 'fixed' and 'offset'

        Notes
        -----
        To create a “bounce” type reaction, for simulating excluded volume,
        enter the typeas bounce.  In this case, enterno parameter for the
        default algorithm orone parameter.The default algorithm, also entered
        with a -2 parameter, performs ballistic reflection for spherical
        molecules. Enter a parameter of -1 for an algorithm in which the reactant
        edges get separated by the same amount as they used to overlap, along
        their separation vector (e.g. consider two reactants each of radius 1,
        so the binding radius is set to 2; then, if the center-to-center
        distance is found to be 1.6, the molecules get separated to make the
        center-to-center distance equal to 2.4).  Alternatively, you can use
        the parameter value to define the new separation, which should be
        larger than the binding radius.
        """
        type = self.__typedict__.get(type, type)
        revType = _smoldyn.RevParam.__members__[type]
        if type in ["fixed", "offset"]:
            assert product, "Product is required"
            assert pos, "pos is required"
        k = _smoldyn.setReactionProducts(self.name, revType, param, product, pos)
        assert k == _smoldyn.ErrorCode.ok


class Reaction(object):
    """A chemical reaction. Each reaction creates two HalfReactions: forward
    and backward. 
    """

    def __init__(
        self,
        name: str,
        subs: List[SpeciesWithState],
        prds: List[SpeciesWithState],
        kf: float,
        kb: float = 0.0,
        compartment: Compartment = None,
        surface: Surface = None,
    ):
        """__init__.

        Parameters
        ----------
        name : str
            name of the reaction.
        subs : List[SpeciesWithState]
            subtrates
        prds : List[SpeciesWithState]
            products
        kf : float
            Forward rate constant
        kb : float
            Backward rate constant (default 0.0)
        compartment : Compartment
            Reaction compartment. If not `None`, both forward and backward
            reactions will be limited to this compartment. 
        surface : Surface
            Reaction surface. If not `None`, both forward and backward
            reactions will be limited to this surface. 
        """
        self.name = f"r{id(self):d}" if not name else name
        fwdname, revname = (name + "fwd", name + "rev") if kb > 0.0 else (name, "")
        self.forward = HalfReaction(fwdname, subs, prds, kf, compartment, surface)
        self.backward = None
        if kb > 0:
            self.backward = HalfReaction(revname, prds, subs, kb, compartment, surface)


def setBounds(low: List[float], high: List[float], types: Union[str, List[str]] = []):
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


def addMoleculesToSolution(molecule, *args, **kwargs):
    """Add molecules to the solution. 

    An alias of Species.addToSolution

    See also
    --------
    Species.addToSolution
    """
    molecule.addMoleculesToSolution(*args, **kwargs)
