"""Smoldyn user API."""

__author__ = "Dilawar Singh"
__email__ = "dilawar.s.rajput@gmail.com"

__all__ = [
    "__version__",
    "version",
    "getError",
    "Simulation",
    "Species",
    "Panel",
    "Triangle",
    "Rectangle",
    "Sphere",
    "Hemisphere",
    "Cylinder",
    "Disk",
    "Path2D",
    "Surface",
    "Port",
    "Partition",
    "Compartment",
    "Reaction",
    "ReactionRule",
    "BidirectionalReaction",
]

import operator
import functools
import math
import warnings
from pathlib import Path
from dataclasses import dataclass

from typing import Union, Tuple, List, Dict, Optional, Sequence, Literal, TypeAlias
from collections.abc import Callable


from smoldyn.types import Color, BoundType, ColorType, DiffConst
from smoldyn import _smoldyn

# Path of model file.
# It does not look like there is a beter way to find this path out
# automatically. This is needed by simptr.
import inspect

__top_model_file__ = Path(inspect.stack()[-1].filename)

# Logger
import logging

__logger__ = logging.getLogger(__name__)
ch = logging.StreamHandler()
ch.setLevel(logging.WARNING)
ch.setFormatter(logging.Formatter("%(name)s - %(levelname)s - %(message)s"))
__logger__.addHandler(ch)


def version() -> str:
    """Get version"""
    return str(_smoldyn.__version__)


# alias of version()
__version__: str = version()


def getError(clear: bool = False) -> Tuple[_smoldyn.ErrorCode, str]:
    return _smoldyn.getError(clear)  # type: ignore


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
    def __init__(
        self,
        simulation: _smoldyn.Simulation,
        name: str,
        state: str = "soln",
        color: ColorType | Dict[str, ColorType] = {"soln": "black"},
        difc: float | Dict[str, float] = 0.0,
        display_size: float = 3,
        mol_list: str = "",
    ):
        """
        Species or Molecule.

        Parameters
        ----------
        name : str
            Name of the species.
        state : str
            State of the species. One of the following:
                'soln', 'front', 'back', 'up', 'down', 'bsoln', 'all', 'none','some'
        color:
            To assign different colors to different states, use a dictionary of state and colors
            e.g. {'soln':'red', 'front':'blue'}. If a single color is given, all
            otherwise all states will be asigned the same color.
        difc : float or a dict of floats
            Diffusion coefficient. If a single value is given, all states of
            the molecule gets the same value. To assign different values to
            different states, use a dict e.g. {'soln':1, 'front':2}.
        display_size : float, optional
            display size of the molecule. For the ``opengl`` graphics method, the
            this value is in pixels.  Here, numbers from 2 to 4 are
            typically good choices (default 3px).  For better graphics
            options (opengl_good and opengl_better), the display size value is
            the radius with which the molecule is drawn, using the same units
            as elsewhere in the input file.
        mol_list : str, optional
            molecule list (default '')

        """
        self.simulation = simulation
        self.name: str = name
        assert self.name

        self._displaySize: Dict[str, float] = {}
        self._color: Dict[str, Color] = {}
        self._difc: Dict[str, float] = {}
        self._mol_list: str = ""

        k = self.simulation.addSpecies(self.name)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add molecule {k}"

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
        self.setStyle(color=color, display_size=display_size)
        if mol_list:
            self.mol_list: str = mol_list

    def __repr__(self) -> str:
        return f"<Molecule: {self.name}, difc={self.difc}, state={self.state}>"

    def __to_disp__(self) -> str:
        return f"{self.name}({self.state})"

    def setStyle(
        self,
        *,
        display_size: None | float | Dict[str, float],
        color: None | ColorType | Dict[str, ColorType],
    ) -> None:
        if color:
            self.color = color
        if display_size:
            self.display_size = display_size

    @property
    def difc(self) -> DiffConst:
        return self._difc

    @difc.setter
    def difc(self, difconst: DiffConst) -> None:
        """Isotropic Diffusion coeffiecient of the Species.

        Parameters
        ----------
        difconst : DiffConst (default '0')
            If a single value is given, all states of the
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
            k = self.simulation.setSpeciesMobility(self.name, st, D)
            assert k == _smoldyn.ErrorCode.ok

    @property
    def mol_list(self) -> str:
        return self._mol_list

    @mol_list.setter
    def mol_list(self, val: str) -> None:
        k = self.simulation.addMolList(val)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add mollist: {k}"
        k = self.simulation.setMolList(self.name, _toMS(self.state), val)
        assert k == _smoldyn.ErrorCode.ok, (
            f"Failed to set mol_list={val} on {self}: {k}"
        )
        self._mol_list = val

    @property
    def color(self) -> Dict[str, Color]:
        return self._color

    @color.setter
    def color(self, color: ColorType | Dict[str, ColorType]) -> None:
        if not isinstance(color, dict):
            self._color = {self.state: Color(color)}
        else:
            self._color = {k: Color(v) for (k, v) in color.items()}
        for state, _clr in self._color.items():
            k = self.simulation.setMoleculeColor(
                self.name, _toMS(state), Color(_clr).rgba
            )
            assert k == _smoldyn.ErrorCode.ok

    @property
    def display_size(self) -> Dict[str, float]:
        return self._displaySize

    @display_size.setter
    def display_size(self, size: Union[float, Dict[str, float]]) -> None:
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
            state = (
                _smoldyn.MolecState.__members__[state]
                if isinstance(state, str)
                else state
            )
            k = self.simulation.setMoleculeSize(self.name, state, size)
            assert k == _smoldyn.ErrorCode.ok

    def addToSolution(
        self,
        N: int,
        pos: List[float] = [],
        highpos: List[float] = [],
        lowpos: List[float] = [],
    ) -> None:
        """Add molecule to solution.

        Parameters
        ----------
        N : float
            Number of molecules
        pos : List[float]
            Fixed location. If given, both `lowpos` and `highpos` will be set
            to this value.
        highpos : List[float]
            Upper bound on the molecules location. Molecules will be distributed
            uniformly between the lowerbound and this value.
        lowpos : List[float]
            Lower bound on the molecules location. Molecules will be distributed
            uniformly between the upper bound and this value.
        """
        assert self.state == "soln", (
            f"You can't use this function on a Species with type {self.state}"
        )
        if pos:
            lowpos = highpos = pos
        k = self.simulation.addSolutionMolecules(self.name, N, lowpos, highpos)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add to solution: {k}"

    def setSurfaceDrift(
        self,
        surface: Union["Surface", str],
        drift: List[float],
        *,
        state: Union[str, _smoldyn.MolecState, None] = None,
        panelshape: Union[str, _smoldyn.PanelShape] = "all",
    ) -> None:
        """Set the drift velocity of surface-bound molecules of this species,
        relative to the panels they are bound to (``surface_drift`` statement).

        Parameters
        ----------
        surface : Surface or str
            Surface on which the drift applies, or ``"all"`` for all surfaces.
        drift : List[float]
            Drift vector with dim-1 values, in panel coordinates.
        state : str, optional
            Surface-bound state ("front", "back", "up", "down"), or "all".
            Defaults to this species' state if it is surface-bound, and to
            "all" otherwise.
        panelshape : str, optional
            Panel shape ("rect", "tri", "sph", "cyl", "hemi", "disk") on which
            the drift applies, or "all" (default).
        """
        if state is None:
            state = self.state if self.state in ("front", "back", "up", "down") else "all"
        sname = surface if isinstance(surface, str) else surface.name
        ps = (
            _smoldyn.PanelShape.__members__[panelshape]
            if isinstance(panelshape, str)
            else panelshape
        )
        k = self.simulation.setSpeciesSurfaceDrift(self.name, _toMS(state), sname, ps, drift)
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"setSurfaceDrift failed for species '{self.name}': {k}")


@dataclass
class NullSpecies(Species):
    name: str = ""
    state = _smoldyn.MolecState.__members__["none"]

    def __bool__(self) -> Literal[False]:
        return False

    def __len__(self) -> Literal[0]:
        return 0


# Type of a substract and product of a Reaction
SpeciesState: TypeAlias = Union[_smoldyn.MolecState, str]
SpeciesWithState: TypeAlias = Union[Species, Tuple[Species, SpeciesState]]


class Panel(object):
    def __init__(
        self,
        *,
        name: str = "",
        shape: _smoldyn.PanelShape = _smoldyn.PanelShape.none,
        neighbors: List["Panel"] = [],
        simulation: Optional[_smoldyn.Simulation] = None,
    ) -> None:
        """Panels are components of a surface. One or more Panels are requried
        to form a Surface. Following geometric primitives are available.

        - [Rectangle](smoldyn.smoldyn.Rectangle)
        - [Triangle](smoldyn.smoldyn.Triangle)
        - [Sphere](smoldyn.smoldyn.Sphere)
        - [Hemisphere](smoldyn.smoldyn.Hemisphere)
        - [Cylinder](smoldyn.smoldyn.Cylinder)
        - [Disk](smoldyn.smoldyn.Disk)
        """

        self.axisstr: str = ""
        self._neighbors: List[Panel] = []
        self.name = name
        self.ctype: _smoldyn.PanelShape = shape
        self.pts: List[float] = []
        self.simulation = simulation
        self.front = _PanelFace(name="front", panel=self, simulation=self.simulation)
        self.back = _PanelFace(simulation=self.simulation, name="back", panel=self)
        self.surface = Surface.null(simulation)
        self.neighbors = neighbors

    def _setSimulation(self, simulation: _smoldyn.Simulation) -> None:
        assert simulation
        self.simulation = simulation
        self.front.simulation = simulation
        self.back.simulation = simulation

    def jumpTo(
        self, face1: str, panel2: "Panel", face2: str, bidirectional: bool = False
    ) -> None:
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
        jfrom = getattr(self, face1)
        assert jfrom, f"Could not locate {face1} on {self}."
        assert isinstance(jfrom, _PanelFace)
        jto = getattr(panel2, face2)
        assert jto, f"Could not locate {face2} on {panel2}."
        assert isinstance(jto, _PanelFace)

        return jfrom.jumpTo(jto, bidirectional)

    def __str__(self) -> str:
        return f"<{self.name} type={self.ctype} index={self.index} points={self.pts}>"

    def _axisIndex(self, axisname: str) -> int:
        axisDict = dict(x=0, y=1, z=2)
        return int(axisDict.get(axisname.lower(), axisname))

    @property
    def index(self) -> int:
        assert self.simulation
        v: int = self.simulation.getPanelIndexNT(self.surface.name, self.name)
        return v

    def toText(self) -> str:
        return f"panel {self.ctype.name} {self.axisstr} {' '.join(map(str, self.pts))} {self.name}".strip()

    @property
    def neighbors(self) -> List["Panel"]:
        return self._neighbors

    @neighbors.setter
    def neighbors(self, panels: List["Panel"]) -> None:
        self._neighbors = panels
        for panel in panels:
            self._assignNeighbor(panel)

    @property
    def neighbor(self) -> "Panel":
        return self._neighbors[0]

    @neighbor.setter
    def neighbor(self, panel: "Panel") -> None:
        self._assignNeighbor(panel)

    def _assignNeighbor(self, panel: "Panel", reciprocal: bool = False) -> None:
        assert self.surface, f"Panel {self} has no Surface assigned."
        assert panel.surface, f"Panel {panel} has no Surface assigned."
        assert self != panel, "A panel cannot be its own neighbor"
        assert self.simulation
        k = self.simulation.addPanelNeighbor(
            self.surface.name, self.name, panel.surface.name, panel.name, reciprocal
        )
        assert k == _smoldyn.ErrorCode.ok

    def _getName(self, index: int) -> str:
        if self.name:
            return self.name
        return f"{self.ctype.name}{index}"

    def setNeighbors(self, panels: List["Panel"], reciprocal: bool = False) -> None:
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
        self,
        corner: Sequence[float],
        dimensions: Sequence[float],
        axis: str,
        name: str = "",
        simulation: Optional[_smoldyn.Simulation] = None,
    ) -> None:
        """Rectangle

        Parameters
        ----------
        corner :
            One corner of the rectangle.
        dimensions :
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
        super().__init__(
            simulation=simulation, shape=_smoldyn.PanelShape.rect, name=name
        )
        self.corner = corner
        self.dimensions = dimensions
        assert axis[0] in "+-", "axis must precede by '+' or '-'"
        assert axis[1].lower() in "012xyz"
        self.axisstr = axis.lower()
        self.axisIndex = self._axisIndex(axis[1])
        self.toGenericPanel()

    def toGenericPanel(self) -> Tuple[str, List[float]]:
        self.pts = [*self.corner, *self.dimensions]
        return self.axisstr, self.pts


class Triangle(Panel):
    def __init__(
        self,
        *,
        vertices: Sequence[Sequence[float]] = [[]],
        name: str = "",
        simulation: Optional[_smoldyn.Simulation] = None,
    ):
        """Triangle Panel.

        Parameters
        ----------
        vertices :
            vertices of triangle.
        name :
            name
        """
        super().__init__(
            simulation=simulation, shape=_smoldyn.PanelShape.tri, name=name
        )
        self.axisstr = ""
        self.vertices = vertices
        self.pts = functools.reduce(operator.iconcat, vertices, [])


class Sphere(Panel):
    def __init__(
        self,
        *,
        center: List[float],
        radius: float,
        slices: int,
        stacks: int = 0,
        name: str = "",
        simulation: Optional[_smoldyn.Simulation] = None,
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
        super().__init__(
            simulation=simulation, shape=_smoldyn.PanelShape.sph, name=name
        )
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
        *,
        center: List[float],
        radius: float,
        vector: List[float],
        slices: int,
        stacks: int,
        name: str = "",
        simulation: Optional[_smoldyn.Simulation] = None,
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
        super().__init__(
            simulation=simulation, shape=_smoldyn.PanelShape.hemi, name=name
        )
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
        *,
        start: List[float],
        end: List[float],
        radius: float,
        slices: int,
        stacks: int,
        name: str = "",
        simulation: Optional[_smoldyn.Simulation] = None,
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
        super().__init__(
            simulation=simulation, shape=_smoldyn.PanelShape.cyl, name=name
        )
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
        self,
        *,
        center: List[float],
        radius: float,
        slices: int,
        vector: List[float],
        name: str = "",
        simulation: Optional[_smoldyn.Simulation] = None,
    ):
        """Disk

        Parameters
        ----------
        center :
            center of the disk.
        radius : float
            radius of the disk.
        slices : int
            Number of slices for drawing.
        vector:
            A vector that points away from the front of the disk. The size of
            this vector is 2 for 2-D and 3 for 3-D.
        name: optional
            name of this Disk.
        """
        super().__init__(
            simulation=simulation, shape=_smoldyn.PanelShape.disk, name=name
        )
        assert slices > -1, f"Slices must be positive. Given value {slices}"
        self.center = center
        self.radius = radius
        self.slices = slices
        self.vector = vector
        # Smoldyn panel
        self.axisstr = ""
        # From the manual: #Slice is entered last (after normal vector).
        self.pts = [*self.center, self.radius, *self.vector, self.slices]


class _PanelFace(object):
    def __init__(self, *, simulation: _smoldyn.Simulation, name: str, panel: Panel):
        self.simulation = simulation
        assert name in ["front", "back"]
        self.name = _smoldyn.PanelFace.__members__[name]
        self.panel = panel

    def jumpTo(self, toface: "_PanelFace", bidirectional: bool = False) -> None:
        """Add a jump reaction between two panels of same Surface

        See also
        --------
        Panel.jumpTo
        """
        assert self.panel.ctype == toface.panel.ctype, (
            "Both panels must have same geometry"
        )
        assert self.panel.surface == toface.panel.surface, (
            "Both panels must have same surface"
        )
        __logger__.debug(
            f"{self.panel.surface.name}, {self.panel.name} "
            f", {self.name}, {toface.panel.name}, {toface.name}, "
            f"{bidirectional}"
        )
        k = self.simulation.setPanelJump(
            self.panel.surface.name,
            self.panel.name,
            self.name,
            toface.panel.name,
            toface.name,
            bidirectional,
        )
        assert k == _smoldyn.ErrorCode.ok


class _SurfaceFaceCollection(object):
    def __init__(
        self, simulation: _smoldyn.Simulation, faces: List[str], surfname: str
    ):
        """Collection of faces of a surface"""
        self.simulation = simulation
        self.faces: List[str] = faces
        self.surfname: str = surfname
        self.__valid_actions_ = [
            "reflect",
            "trans",
            "absorb",
            "jump",
            "port",
            "mult",
            "no",
            "none",
        ]

    def setStyle(
        self,
        color: None | ColorType = None,
        drawmode: str = "none",
        thickness: float = 1.0,
        stipplefactor: int = -1,
        stipplepattern: int = -1,
        shininess: int = -1,
    ) -> None:
        """Set drawing style for the face of surface.

        Parameters
        ----------
        drawmode : str
            May be "none", "vertex", "edge", "face", or combinations of ‘v’,
            ‘e’, or ‘f’ for multiple renderings of vertices, edges, and/or
            faces. 2-D spheres and hemispheres are either filled or are
            outlined depending on the polygon frontcharacter. If multiple
            renderings are chosen in 3D, then panel faces are shown in the
            requested color and other renderings are shown in black.
        thickness : float
            Boldness of the surface in pixels for drawing purposes.  This is
            only relevant for 1-D and 2-D simulations, and for 3-D simulations
            in which surfaces are drawn with just vertices or edges and not
            faces.
        color : Color
            color e.g. 'black', 'red', [0.1,0.1,0.1] etc.
        stipplefactor : int
            stipplefactor is the repeat distance for the entire stippling
            pattern (1 is a good choice).  Stippling of the surface edges, for
            drawing purposes.  This is only relevant for 3-D simulations in
            which surfaces are drawn with just edges and not faces, and with
            opengl_good or opengl_better display method.
        stipplepattern : int
            stipplepattern a hexidecimal integer, enter the stippling pattern between
            ``0x0000`` (0) and ``0xFFFF`` (65536), ``0x00FF`` (256) has long dashes,
            ``0x0F0F`` (3855) has medium dashes, ``0x5555`` (21845)  has dots, etc.
            Turn stippling off with ``0xFFFF`` (65536).
        shininess : float
            Shininess of the surface for drawing purposes.  This value can
            range from 0 for visually flat surfaces to 128 for very shiny
            surfaces.  This is only relevant for some simulations.
        """
        for which in self.faces:
            # TODO : Check on drawmode
            k = self.simulation.setSurfaceStyle(
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

    def setAction(
        self,
        species: Union[Species, List[Species]],
        action: str,
        new_species: Optional[Species] = None,
    ) -> None:
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
        new_species:
            TODO: Implement it.
            If `new_species` is entered, then the molecule changes to this new
            species upon surface collision. In addition, it’s permissible to
            enter the action as “multiple,” in which case the rates need to be
            set with rate; alternatively, just setting the rates will
            automatically set the action to “multiple.” The default is
            transmission for all molecules.
        """
        assert action in self.__valid_actions_, (
            f"'{action}' is not a valid action. Available actions: {', '.join(self.__valid_actions_)}"
        )

        if isinstance(species, str):
            raise NameError(
                "This API does not accepts species by their names or the keyword 'all'."
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

            # TODO: see issue #17
            if new_species:
                raise NotImplementedError("This feature is not implemented.")

            for which in self.faces:
                k = self.simulation.setSurfaceAction(
                    self.surfname,
                    _smoldyn.PanelFace.__members__[which],
                    sname,
                    sstate,
                    _smoldyn.SrfAction.__members__[action],
                )
                assert k == _smoldyn.ErrorCode.ok, f"Failed setSurfaceAction: {k}"


class Path2D(object):
    def __init__(
        self,
        *points: Tuple[float, float],
        simulation: _smoldyn.Simulation,
        closed: bool = False,
    ):
        """Construct a 2D path from given points.

        A Path2D consists of `Rectangle` and `Triangle`.

        Parameters
        ----------
        *points:
            Points
        simulation:
            _smoldyn.Simulation object.
        closed: bool
            If closed is `True`, the last point and the first point are
            connected. Default `False`.
        """
        self.points = list(points)

        self.simulation = simulation
        assert self.simulation

        if closed and self.points[0] != self.points[-1]:
            self.points.append(self.points[0])
        self.panels: List[Panel] = []
        for p1, p2 in zip(self.points, self.points[1:]):
            (x1, y1), (x2, y2) = p1, p2
            theta = math.atan2(y2 - y1, x2 - x1)
            __logger__.debug(f"theta={theta} {p1} and {p2}")
            if theta in [0.0, math.pi, math.pi / 2.0, -math.pi / 2.0]:
                if theta == 0.0:
                    axis = "+y"
                elif theta == math.pi:
                    axis = "-y"
                elif theta == math.pi / 2.0:
                    axis = "-x"
                elif theta == -math.pi / 2.0:
                    axis = "+x"
                else:
                    raise RuntimeError(
                        "Should not be here. Python3 numerical computation is broken!"
                    )
                length = x2 - x1 if y2 == y1 else y2 - y1
                p = Rectangle(
                    simulation=self.simulation,
                    corner=(x1, y1),
                    dimensions=[length],
                    axis=axis,
                )
                __logger__.info(f"Added a Rectangle {p}")
                self.panels.append(p)
            else:
                t = Triangle(simulation=self.simulation, vertices=((x1, y1), (x2, y2)))
                self.panels.append(t)


class Surface(object):
    """Surfaces are infinitesimally thin structures that can be used to represent cell membranes, obstructions, system boundaries, or other things.

    Note
    ----

    They are 2D structures in 3D simulations, or 1D lines or curves in 2D simulations (or 0D points
    in 1D simulations).Each surface has a “front” and a “back” face, so molecules can interact
    differently with the two sides of a surface.Each surface is composed of one or more
    “:py:class:`~Panels`”, where each panels can be a rectangle, triangle, sphere, hemisphere,
    cylinder, or a disk. Surfaces can be disjoint, with separate non-connected portions.  However,
    all portions of a given surface type are displayed in the same way and interact with molecules
    in the same way.

    """

    def __init__(
        self, *, simulation: _smoldyn.Simulation, name: str, panels: List[Panel]
    ):
        """
        Parameters
        ----------
        simulation:
            Simulation object
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
        self.simulation = simulation
        self.panels = panels
        self.name = name
        if self.name:
            self.initlaize()

    def initlaize(self) -> None:
        assert self.simulation
        self.simulation.addSurface(self.name)
        self.front = _SurfaceFaceCollection(self.simulation, ["front"], self.name)
        self.back = _SurfaceFaceCollection(self.simulation, ["back"], self.name)
        self.both = _SurfaceFaceCollection(
            self.simulation, ["front", "back"], self.name
        )
        self._addPanelsToSmoldyn()

    @classmethod
    def null(cls, simulation: _smoldyn.Simulation) -> "Surface":
        return cls(simulation=simulation, name="", panels=[])

    def _addPanelsToSmoldyn(self) -> None:
        """Call libsmodyn API to construct this surface' Panels"""
        assert self.name, "Surface name is missing"
        for i, panel in enumerate(self.panels):
            panel.name = panel._getName(i)
            panel.surface = self

            # Set simulation object before adding Panel.
            panel._setSimulation(self.simulation)

            # Add panels.
            k = self.simulation.addPanel(
                self.name,
                panel.ctype,
                panel.name,
                panel.axisstr,
                panel.pts,
            )
            assert k == _smoldyn.ErrorCode.ok, (
                f"Failed to add panel (error={k}): Surface:{self.name} {panel}"
            )

    def setStyle(self, face: str, *args: int | float, **kwargs: int | float) -> None:
        """See the function :func:`_SurfaceFaceCollection.setStyle` for more
        details. This function forwards arguments to
        :func:`_SurfaceFaceCollection.setStyle`.

        Parameters
        ----------
        face: str
            face of the surface: 'front', 'back', 'both'
        *args:
            See :func:`_SurfaceFaceCollection.setStyle`
        **kwargs:
            See :func:`_SurfaceFaceCollection.setStyle`

        See Also
        --------
        :func:`_SurfaceFaceCollection.setStyle`
        """
        assert face in ["front", "back", "both"]
        getattr(self, face).setStyle(*args, **kwargs)

    def setAction(
        self,
        face: str,
        species: Species | List[Species],
        action: str,
        new_species: None | Species = None,
    ) -> None:
        """Set drawing style for the face of surface
        (calls :func:`_SurfaceFaceCollection.setAction`)

        Parameters
        ----------
        face: str
            face of the surface: 'front', 'back', 'both'
        species :
            List of species or a single species. DO NOT use the name of
            species.
        action : str
            The action can be  `“reflect”`, `“absorb”`, `“transmit”`, `“jump”`,
            `“port”`, or `“periodic”`.
        new_species:
            TODO: Implement it.
            If `new_species` is entered, then the molecule changes to this new
            species upon surface collision. In addition, it’s permissible to
            enter the action as “multiple,” in which case the rates need to be
            set with rate; alternatively, just setting the rates will
            automatically set the action to “multiple.” The default is
            transmission for all molecules.


        See Also
        --------
        :func:`_SurfaceFaceCollection.setAction`
        """
        getattr(self, face).setAction(species, action, new_species)

    def addMolecules(
        self,
        species: SpeciesWithState,
        N: int,
        panels: Optional[List[Panel]] = None,
        pos: List[float] = [],
    ) -> None:
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
            Panels of surface. If `None` is given then all panels of
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

        panels = panels if panels is not None else self.panels

        # Distribute molecules equally among the panels.
        _N = N // len(panels)
        Ns = [_N] * len(panels)
        Ns[0] += N - sum(Ns)
        assert sum(Ns) == N

        for panel, _n in zip(panels, Ns):
            assert panel
            assert panel.ctype
            k = self.simulation.addSurfaceMolecules(
                sname,
                sstate,
                _n,
                self.name,
                panel.ctype,
                panel.name,
                pos,
            )
            assert k == _smoldyn.ErrorCode.ok, k

    def _setRate(
        self,
        species: SpeciesWithState,
        state1: SpeciesState,
        state2: SpeciesState,
        rate: float,
        new_species: None | Species = None,
        isinternal: bool = False,
    ) -> None:
        if not isinstance(species, Species):
            sname, sstate = species[0].name, species[1]
        else:
            sname, sstate = species.name, species.state
        state1 = _toMS(state1)
        new_species_str = new_species.name if new_species else ""
        k = self.simulation.setSurfaceRate(
            self.name,
            sname,
            _toMS(sstate),
            _toMS(state1),
            _toMS(state2),
            rate,
            new_species_str,
            isinternal,
        )
        assert k == _smoldyn.ErrorCode.ok, "Failed to set rate"

    def setRate(
        self,
        species: SpeciesWithState,
        state1: SpeciesState,
        state2: SpeciesState,
        rate: float,
        revrate: float = 0.0,
        new_species: None | Species = None,
        isinternal: bool = False,
    ) -> None:
        """The rate constant for transitions from `state1` to `state2` of molecules
        named `species`.

        Parameters
        ----------
        species : SpeciesWithState
            A species or a tuple of `Species` and its state.
        state1 : SpeciesState
            Can be any of: fsoln, bsoln (in solution, hitting the front or
            back of the panel, respectively), front, back, up, or down.
        state2 : SpeciesState
            Can be any of: fsoln, bsoln (in solution, hitting the front or
            back of the panel, respectively), front, back, up, or down.
        rate : float
            rate (must be on-zero positive value).
        revrate: float
            reverse rate (default 0.0). If this value is a positive non-zero
            value, then a reverse rate is also set.
        new_species :
            newspecies
        isinternal:
            When set to `True`, a different value is entered. Instead of
            entering the surface action rate, enter the probability of the
            action at each collision.  Probabilities for reflection are ignored
            since they are calculated as the probability that the molecule does
            not transmit, absorb, or jump.
        """
        self._setRate(species, state1, state2, rate, new_species, isinternal)
        if revrate > 0.0:
            self._setRate(species, state2, state1, revrate, new_species, isinternal)

    def setNeighborAction(self, action: str) -> None:
        """Set what surface-bound molecules on this surface do when they
        collide with a panel that is a neighbor of their own panel
        (``neighbor_action`` statement).

        Parameters
        ----------
        action : str
            ``"stay"`` (default) to ignore the neighboring panel and diffuse
            through it, or ``"hop"`` to move onto it with 50% probability.
        """
        assert action in ("hop", "stay"), "action must be 'hop' or 'stay'"
        k = self.simulation.setSurfaceNeighborAction(self.name, action == "hop")
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"setNeighborAction failed for surface '{self.name}': {k}")

    __actiondict__ = dict(transmit="trans", multiple="mult")

    def setActionRule(
        self,
        face: str,
        pattern: str,
        action: str,
        state: SpeciesState = "soln",
    ) -> None:
        """Set the action for all species that match `pattern` when they
        collide with this surface (``action_rule`` statement). The rule
        applies to species generated by later rule expansion, as well as to
        existing species.

        Parameters
        ----------
        face : str
            'front', 'back', or 'both'
        pattern : str
            Species pattern, e.g. ``"A*"``.
        action : str
            'reflect', 'transmit' ('trans'), 'absorb', 'jump', 'port', or
            'multiple' ('mult').
        state : str
            State of the matching molecules (default 'soln'), or 'all'.
        """
        action = self.__actiondict__.get(action, action)
        k = self.simulation.setSurfaceActionRule(
            self.name,
            _smoldyn.PanelFace.__members__[face],
            pattern,
            _toMS(state),
            _smoldyn.SrfAction.__members__[action],
        )
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"setActionRule failed for surface '{self.name}': {k}")

    def setRateRule(
        self,
        pattern: str,
        state1: SpeciesState,
        state2: SpeciesState,
        rate: float,
        *,
        state: SpeciesState = "soln",
        new_species: None | Species = None,
        isinternal: bool = False,
    ) -> None:
        """Set the rate constant for transitions from `state1` to `state2` of
        all species that match `pattern` (``rate_rule`` and
        ``rate_internal_rule`` statements). See :py:meth:`setRate` for the
        meanings of the states.

        Parameters
        ----------
        pattern : str
            Species pattern, e.g. ``"A*"``.
        state1, state2 : SpeciesState
            Initial and final states.
        rate : float
            Rate constant, or -1 for the maximum rate. If `isinternal` is
            True, this is the transition probability at each collision.
        state : SpeciesState
            State of the matching molecules (default 'soln').
        new_species : Species, optional
            Species that molecules convert to on transition.
        isinternal : bool
            Whether `rate` is an internal probability.
        """
        k = self.simulation.setSurfaceRateRule(
            self.name,
            pattern,
            _toMS(state),
            _toMS(state1),
            _toMS(state2),
            rate,
            new_species.name if new_species else "",
            isinternal,
        )
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"setRateRule failed for surface '{self.name}': {k}")


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

    def __init__(
        self,
        simulation: _smoldyn.Simulation,
        *,
        name: str,
        surface: Surface | str,
        panel: str,
    ):
        """
        Parameters
        ----------
        simulation:
            _smoldyn.Simulation() object
        name : str
            name of the port
        surface : Union[Surface,str]
            Porting surface (must be created before).
        panel : str
            Face of the which surface active for porting: "front" or "back".
        """
        self.simulation = simulation
        self.name: str = name
        self.surfname = surface.name if isinstance(surface, Surface) else surface
        assert panel in ["front", "back"]
        self.panel = _smoldyn.PanelFace.__members__[panel]
        k = self.simulation.addPort(self.name, self.surfname, self.panel)
        assert k == _smoldyn.ErrorCode.ok


class Compartment(object):
    def __init__(
        self,
        simulation: _smoldyn.Simulation,
        name: str,
        *,
        surface: str | Surface,
        point: List[float],
    ):
        """Comapartment.

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
        self.simulation = simulation
        assert self.point, "At least one point is required."
        k = self.simulation.addCompartment(self.name)
        assert k == _smoldyn.ErrorCode.ok
        k = self.simulation.addCompartmentSurface(self.name, self.srfname)
        assert k == _smoldyn.ErrorCode.ok
        k = self.simulation.addCompartmentPoint(self.name, self.point)
        assert k == _smoldyn.ErrorCode.ok

    def addMolecules(self, species: Species, N: int) -> None:
        """Place number of molecules in a compartment (uniformly distributed).

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
        k = self.simulation.addCompartmentMolecules(species.name, N, self.name)
        assert k == _smoldyn.ErrorCode.ok


class _ReactionCommon(object):
    """Methods shared by :class:`Reaction` and :class:`ReactionRule`. When
    called on a reaction rule, these settings apply to the rule and to any
    reactions that have already been generated from it.
    """

    simulation: _smoldyn.Simulation
    name: str

    __simparams__ = (
        "rate",
        "multiplicity",
        "confspread_radius",
        "binding_radius",
        "probability",
        "chi",
        "production",
    )

    def _check(self, k: _smoldyn.ErrorCode, what: str) -> None:
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"{what} failed for reaction '{self.name}': {k}")

    def setSimParams(self, **params: float) -> None:
        """Set one or more reaction parameters, given as keyword arguments,
        e.g. ``rxn.setSimParams(multiplicity=2, chi=0.3)``.

        Parameters
        ----------
        rate : float
            Reaction rate constant (``reaction_rate`` statement).
        multiplicity : int
            Reaction multiplicity (``reaction_multiplicity``).
        confspread_radius : float
            Make this a conformational spread reaction with this radius
            (``confspread_radius``).
        binding_radius : float
            Binding radius of a bimolecular reaction (``binding_radius``).
        probability : float
            Reaction probability, between 0 and 1 (``reaction_probability``).
        chi : float
            Diffusion-influenced chi value of a bimolecular reaction, between 0
            and 1, or -1 to disable (``reaction_chi``).
        production : float
            Production probability of a zeroth order reaction
            (``reaction_production``).
        """
        for param, value in params.items():
            assert param in self.__simparams__, (
                f"Unknown reaction parameter '{param}'. Allowed: {self.__simparams__}"
            )
            k = self.simulation.setReactionSimParams(self.name, param, value)
            self._check(k, f"setSimParams({param}={value})")

    def setSerialNum(self, codes: List[str]) -> None:
        """Set rules for the serial numbers of the reaction products
        (``reaction_serialnum`` statement).

        Parameters
        ----------
        codes : List[str]
            One code per product, such as ``"r1"``, ``"r2"``, ``"p1"``,
            ``"new"``, or a two-part code such as ``"r1.new"``.
        """
        k = self.simulation.setReactionSerialnum(self.name, list(codes))
        self._check(k, "setSerialNum")

    def setRepresentation(
        self,
        rctrep: List[Union[str, _smoldyn.SpeciesRepresentation]],
        prdrep: List[Union[str, _smoldyn.SpeciesRepresentation]],
    ) -> None:
        """Set whether reactants and products are represented as particles or
        on a lattice (``reaction_representation`` statement).

        Parameters
        ----------
        rctrep :
            One entry per reactant: ``"particle"``, ``"lattice"``, or ``"both"``.
        prdrep :
            One entry per product: ``"particle"`` or ``"lattice"``.
        """

        def _toSR(x: Union[str, _smoldyn.SpeciesRepresentation]) -> _smoldyn.SpeciesRepresentation:
            return _smoldyn.SpeciesRepresentation.__members__[x] if isinstance(x, str) else x

        k = self.simulation.setReactionRepresentation(
            self.name, [_toSR(x) for x in rctrep], [_toSR(x) for x in prdrep]
        )
        self._check(k, "setRepresentation")

    def log(self, filename: str, serialnums: Optional[List[int]] = None) -> None:
        """Log each occurrence of this reaction to the file `filename`
        (``reaction_log`` statement). The file also needs to be declared as an
        output file.

        Parameters
        ----------
        filename : str
            Output file name.
        serialnums : List[int], optional
            Only log reactions involving molecules with these serial numbers.
            If ``None`` (default), log all occurrences.
        """
        k = self.simulation.setReactionLog(filename, self.name, list(serialnums or []), True)
        self._check(k, "log")

    def logOff(self, serialnums: Optional[List[int]] = None) -> None:
        """Turn off reaction logging (``reaction_log_off`` statement).

        Parameters
        ----------
        serialnums : List[int], optional
            Stop logging only these serial numbers. If ``None`` (default),
            turn off all logging for this reaction.
        """
        k = self.simulation.setReactionLog(None, self.name, list(serialnums or []), False)
        self._check(k, "logOff")


class Reaction(_ReactionCommon):
    __methoddict__ = dict(
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
        simulation: _smoldyn.Simulation,
        name: str,
        subs: List[SpeciesWithState],
        prds: List[SpeciesWithState],
        *,
        rate: float = 0.0,
        compartment: None | Compartment = None,
        surface: None | Surface = None,
        binding_radius: float = 0.0,
        reaction_probability: float = 0.0,
    ):
        """Reaction (only occurs in forwared direction).

        Parameters
        ----------
        simulation: _smoldyn.Simulation
            Simulation object
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
        binding_radius: float
            binding radius (for second order reactions, if kf is not given)
        reaction_probability: float
            reaction probability (for first order reactions, if kf if not
            given)
        """
        self.simulation = simulation
        self.name = "r%d" % id(self) if not name else name
        self.subs = subs
        self.prds = prds
        self.reaction_probability = reaction_probability
        self.binding_radius = binding_radius
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

        k = self.simulation.addReaction(
            self.name,
            r1name,
            r1state,
            r2name,
            r2state,
            prdNames,
            prdStates,
            rate,
        )
        if k != _smoldyn.ErrorCode.ok:
            __logger__.warning(f" Substrates: {subs}")
            __logger__.warning(f" Products: {prds}, {prdNames}/{prdStates}")
            raise RuntimeError(f"Failed to add reaction, ErrorCode {k}")

        self.setRate(rate, reaction_probability, binding_radius)
        if self.compartment is not None or self.surface is not None:
            cname = self.compartment.name if self.compartment else ""
            sname = self.surface.name if self.surface else ""
            assert cname or sname
            k = self.simulation.setReactionRegion(self.name, cname, sname)
            assert k == _smoldyn.ErrorCode.ok

    @property
    def rate(self) -> float:
        return float(self.simulation.getReactionRate(self.name))

    @rate.setter
    def rate(self, rate: float) -> None:
        self.setRate(rate)

    def setRate(
        self,
        rate: float,
        reaction_probability: None | float = None,
        binding_radius: float = -1.0,
    ) -> None:
        # if rate is negative, then we expect either binding_radius or
        # reaction_probability. A reaction can have zero rate.
        if rate >= 0.0:
            k = self.simulation.setReactionRate(self.name, rate, False)
            assert k == _smoldyn.ErrorCode.ok
            return

        # check if reaction_probability is set when rate is not set.
        if len(self.subs) < 2:
            assert reaction_probability is not None, (
                "Must set rate or reaction_probability"
            )
            k = self.simulation.setReactionRate(self.name, reaction_probability, True)
            assert k == _smoldyn.ErrorCode.ok
        else:
            if reaction_probability is not None:
                assert 0.0 <= reaction_probability <= 1.0, (
                    "reaction_probability must be between 0 and 1"
                )
                k = self.simulation.setReactionRate(self.name, reaction_probability, 2)
                assert k == _smoldyn.ErrorCode.ok
            if binding_radius >= 0.0:
                k = self.simulation.setReactionRate(self.name, binding_radius, True)
                assert k == _smoldyn.ErrorCode.ok

    @property
    def order(self) -> int:
        return len(self.subs)

    def setIntersurface(self, rules: List[Union[int, str]]) -> None:
        """Define `rules` to allow a bimolecular reaction operates when its
        reactants are on different surfaces. In general, there should be as
        many rule values as there are products for this reaction

        Parameters
        ----------
        rules :
            List of integer or string. For each product choose `1` (or
            ``"r1"``) if it should be placed on the first reactant’s surface or
            relative to that surface, and `2` (``"r2"``) if it should be placed
            on the second reactant’s surface or relative to that surface. To
            turn off intersurface reactions, which is the default behavior,
            give rule_list as ``[-1]`` or ``None``.  To turn on intersurface
            reactions for reactions that have no products, give rule_list as
            ``[0]`` or ``[]``. This statement cannot be used together with the
            ``setSerialNum`` function for the same reaction.

        """
        assert self.order == 2, "Bimoleculear reaction is needed."
        assert len(rules) == len(self.prds), (
            "Length of rules should be equal to number of products"
        )
        rules = [-1] if rules is None else rules
        rules = [0] if rules == [] else rules
        _rules: List[int] = []

        # r1, r2 are turned to 1, 2 etc.
        for r in rules:
            r = int(r[1:]) if isinstance(r, str) else int(r)
            _rules.append(r)
        k = self.simulation.setReactionIntersurface(self.name, _rules)
        assert k == _smoldyn.ErrorCode.ok

    def productPlacement(
        self,
        method: str,
        param: float = 0.0,
        product: str = "",
        pos: List[float] = [],
    ) -> None:
        """Placement method and parameters for the products of reaction.
        This also affects the binding radius of the reverse reaction, as
        explained in the manual.

        Parameters
        ----------
        method : 'irrev' ('i'), 'pgem' ('p'), 'pgemmax' ('x')
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
        enter the type as bounce.  In this case, enterno parameter for the
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
        method = self.__methoddict__.get(method, method)
        revType = _smoldyn.RevParam.__members__[method]

        if method in ["fixed", "offset"]:
            assert product, "Product is required"
            assert pos, "pos is required"
        k = self.simulation.setReactionProducts(self.name, revType, param, product, pos)
        assert k == _smoldyn.ErrorCode.ok

    def permit(self, states: List[SpeciesState]) -> None:
        """Permit this reaction for the given reactant states
        (``reaction_permit`` statement). Not allowed for zeroth order
        reactions.

        Parameters
        ----------
        states :
            One state per reactant, e.g. ``["front", "all"]``.
        """
        assert len(states) == self.order, "Need one state per reactant"
        k = self.simulation.setReactionPermit(self.name, [_toMS(x) for x in states], True)
        self._check(k, "permit")

    def forbid(self, states: List[SpeciesState]) -> None:
        """Forbid this reaction for the given reactant states
        (``reaction_forbid`` statement). Not allowed for zeroth order
        reactions.

        Parameters
        ----------
        states :
            One state per reactant, e.g. ``["soln", "all"]``.
        """
        assert len(states) == self.order, "Need one state per reactant"
        k = self.simulation.setReactionPermit(self.name, [_toMS(x) for x in states], False)
        self._check(k, "forbid")


# A species pattern for reaction rules: a pattern string or Species, optionally
# with a state.
SpeciesPattern: TypeAlias = Union[str, Species, Tuple[Union[str, Species], SpeciesState]]


class ReactionRule(_ReactionCommon):
    def __init__(
        self,
        simulation: _smoldyn.Simulation,
        name: str,
        subs: List[SpeciesPattern],
        prds: List[SpeciesPattern],
        *,
        rate: float = -1.0,
        compartment: None | Compartment = None,
        surface: None | Surface = None,
    ):
        """Reaction rule (``reaction_rule`` statement), which generates
        reactions for all species that match its patterns. Reactions are
        generated when :py:meth:`Simulation.expandRules` is called.

        Parameters
        ----------
        simulation: _smoldyn.Simulation
            Simulation object
        name: str
            name of the rule.
        subs : List[SpeciesPattern]
            List of reactant patterns (at most two). Each is a pattern string
            such as ``"A*"``, a Species, or a tuple of either with a state,
            e.g. ``("A*", "front")``.
        prds : List[SpeciesPattern]
            List of product patterns, in the same format.
        rate : float
            rate of the generated reactions; negative to leave unset.
        compartment: Compartment
            If not ``None``, restrict the rule to this compartment.
        surface: Surface
            If not ``None``, restrict the rule to this surface.
        """
        self.simulation = simulation
        self.name = name
        self.subs = subs
        self.prds = prds
        assert len(subs) < 3, "At most two reactants are supported."
        assert subs or prds, "Reaction rule needs reactants or products."

        def _split(x: SpeciesPattern) -> Tuple[str, _smoldyn.MolecState]:
            if isinstance(x, str):
                return x, _toMS("soln")
            if isinstance(x, Species):
                return x.name, _toMS(x.state)
            pat, state = x
            return (pat if isinstance(pat, str) else pat.name), _toMS(state)

        rcts = [_split(x) for x in subs] + [("", _toMS("none"))] * (2 - len(subs))
        prdlist = [_split(x) for x in prds]
        k = self.simulation.addReactionRule(
            name,
            rcts[0][0],
            rcts[0][1],
            rcts[1][0],
            rcts[1][1],
            [x[0] for x in prdlist],
            [x[1] for x in prdlist],
            rate,
            compartment.name if compartment else None,
            surface.name if surface else None,
        )
        self._check(k, "addReactionRule")

    @property
    def order(self) -> int:
        return len(self.subs)


class BidirectionalReaction(object):
    def __init__(
        self,
        simulation: _smoldyn.Simulation,
        name: str,
        subs: List[SpeciesWithState],
        prds: List[SpeciesWithState],
        kf: float,
        kb: float = 0.0,
        *,
        binding_radius: float = 0.0,
        reaction_probability: float = 0.0,
        compartment: Compartment | None = None,
        surface: Surface | None = None,
    ):
        """A bidirectional chemical reaction that consists of two Reactions,
        forward  (always present) and reverse (`None` if ``kb<=0.0``).

        Parameters
        ----------
        simulation: _smoldyn.Simulation
            Simulation object
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
        binding_radius: float
            Binding radius (for second order reaction)
        reaction_probability: float
            Reaction probability (first order reaction)
        compartment : Compartment
            Reaction compartment. If not `None`, both forward and reverse
            reactions will be limited to this compartment.
        surface : Surface
            Reaction surface. If not `None`, both forward and reverse
            reactions will be limited to this surface.
        """
        self.simulation = simulation
        self.name = f"r{id(self):d}" if not name else name
        fwdname, revname = (name + "fwd", name + "rev") if kb > 0.0 else (name, "")
        self.forward = Reaction(
            self.simulation,
            fwdname,
            subs,
            prds,
            rate=kf,
            compartment=compartment,
            surface=surface,
            binding_radius=binding_radius,
            reaction_probability=reaction_probability,
        )

        self.reverse = None
        if kb > 0.0:
            self.reverse = Reaction(
                self.simulation,
                revname,
                prds,
                subs,
                rate=kb,
                compartment=compartment,
                surface=surface,
            )

    @property
    def kf(self) -> float:
        return self.forward.rate

    @kf.setter
    def kf(self, val: float) -> None:
        self.forward.rate = val

    @property
    def kb(self) -> float:
        return self.reverse.rate if self.reverse else 0.0

    @kb.setter
    def kb(self, val: float) -> None:
        assert self.reverse
        self.reverse.rate = val


@dataclass
class Partition(object):
    simulation: _smoldyn.Simulation
    name: str
    value: float

    def __post_init__(self) -> None:
        """Sets the virtual partitions in the simulation volumne. Two
        specialization is avaiable: :py:class:`MoleculePerBox`, and
        :py:class:`Box`.

        See also
        --------
        :py:class:`MoleculePerBox`
        :py:class:`Box`
        """
        k = self.simulation.setPartitions(self.name, self.value)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set partition: {k}"


class MoleculePerBox(Partition):
    def __init__(self, simulation: _smoldyn.Simulation, size: float):
        warnings.warn(
            "MoleculePerBox is deprecated. Please use setPartitions in the future",
            DeprecationWarning,
        )
        super().__init__(simulation, "molperbox", size)


class Box(Partition):
    def __init__(self, simulation: _smoldyn.Simulation, size: float):
        warnings.warn(
            "Box is deprecated. Please use setPartitions in the future",
            DeprecationWarning,
        )
        super().__init__(simulation, "boxsize", size)


class Simulation(_smoldyn.Simulation):  # type: ignore
    """Simulation class. A :class:`Simulation` object is an unit of simulation.
    it is recommended to create a :class:`Simulation` object for every standalone
    model.

    Note
    ----

    Every model starts with a Simulation object which has its unique
    :class:`simstruct`. Note that a user can create as many arbitrary smoldyn
    objects as one likes but these objects become active only when they
    are added to a :class:`Simulation` object.

    See also
    --------

    :class:`_smoldyn.Simulation`

    """

    def __init__(
        self,
        low: List[float],
        high: List[float],
        *,
        boundary_type: BoundType = "r",
        types: None | BoundType = None,
        log_level: int = 3,
        accuracy: float | None = None,
        output_files: List[str | Path] = [],
        seed: int = -1,
        quit_at_end: bool = False,
    ):
        """Simulation class is a container for a complete model. An object of
        this class is a stand-alone self-contained model.

        Parameters
        ----------
        low:
            Lower bound.
        high:
            Higher bound.

        log_level: int, default 3
            set log level for default logger.
            1: DEBUG, 2: INFO, 3: WARNING, 4: ERROR, 5: CRITICAL

        Other Parameters
        ----------------
        output_files :
            Declare output files that can be used in addCommand string.
        seed: `int`
            Set the random seed for the simulation.

        See also
        --------
        :py:_smoldyn.simptr
        """

        # https://stackoverflow.com/a/50886750/1805129
        logging.getLogger(__name__).setLevel(10 * log_level)
        for handler in __logger__.handlers:
            handler.setLevel(10 * log_level)
        __logger__.info(f"Setting logging level to {log_level}")

        assert low, f"You must pass a valid low bound, current value {low}"
        assert high, f"You must pass a valid high bound, current value {high}"

        if types is not None:
            warnings.warn(
                "types is deprecated. Please use boundary_type", DeprecationWarning
            )
            boundary_type = types

        # handle boundary type. Previously, it was called 'types'
        if isinstance(boundary_type, str):
            if len(boundary_type) == 1:
                assert len(low) == len(high), (
                    f"dimension mismatch {len(low)} != {len(high)}"
                )
                boundary_type = boundary_type * len(low)
            boundary_type = list(boundary_type)

        super().__init__(low, high, boundary_type)

        assert self.simptr, "Fatal error: Could not create simstruct"

        # set filepath and filename at simptr
        if __top_model_file__:
            __top_model_file__.resolve()
            super().setModelpath(str(__top_model_file__))

        assert self.simptr, "Configuration is not initialized"
        if accuracy:
            self.accuracy: float = accuracy
        self.setOutputFiles(output_files)
        if seed >= 0:
            super().setRandomSeed(seed)
        self.quitatend = quit_at_end

    @classmethod
    def fromFile(cls, path: Union[Path, str], arg: str = "") -> "Simulation":
        """Create `_smoldyn.Simulation` object from model file.

        Parameters
        ----------
        path : T.Union[Path, str]
            path
        arg : str
            arg
        """
        #  return _smoldyn.Simulation(str(path), arg)
        obj = cls.__new__(cls)
        path = Path(path).resolve()  # critical.
        if not path.exists():
            raise FileNotFoundError(str(path))
        super(Simulation, obj).__init__(str(path), arg)
        return obj

    def setOutputFiles(self, outfiles: List[str | Path], append: bool = True) -> None:
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
        assert isinstance(outfiles, (list, tuple)), "Expecting a list of files"
        for outfile in outfiles:
            self.setOutputFile(outfile, append)

    def setOutputFile(self, outfile: str | Path, append: bool = False) -> None:
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
            super().setOutputPath(str(outfile.parent) + "/")
        # FIXME: Not sure what to do with second arg here.
        super().addOutputFile(outfile.name, 0, append)

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
        text_display: List[str | Species] | str = "",
    ) -> None:
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
        k = super().setGraphicsParams(method, iter, delay)
        assert k == _smoldyn.ErrorCode.ok
        k = super().setBackgroundStyle(Color(bg_color).rgba)
        assert k == _smoldyn.ErrorCode.ok
        k = super().setFrameStyle(frame_thickness, Color(frame_color).rgba)
        assert k == _smoldyn.ErrorCode.ok

        if grid_thickness > 0 and grid_color != bg_color:
            k = super().setGridStyle(grid_thickness, Color(grid_color).rgba)
            assert k == _smoldyn.ErrorCode.ok

        self.setText(text_display)

    # alias for setGraphics
    addGraphics = setGraphics

    @classmethod
    def __todisp_text__(cls, x: str | Tuple[Species, Species | str] | Species) -> str:
        if isinstance(x, str):
            return x
        if isinstance(x, tuple):
            return f"{x[0].name}({x[1]})"
        return x.__to_disp__()

    def setText(
        self,
        text_display: str | List[str | Species],
        color: ColorType = "black",
    ) -> None:
        item_display: Sequence[str | Species] = []
        if isinstance(text_display, str):
            item_display = text_display.strip().split(" ")
        else:
            item_display = text_display
        for item in item_display:
            if not item:
                continue
            k = super().addTextDisplay(self.__todisp_text__(item))
            assert k == _smoldyn.ErrorCode.ok, f"Failed to set '{item}'"

        k = super().setTextStyle(Color(color).rgba)
        assert k == _smoldyn.ErrorCode.ok, "Failed to set text color"

    def setTiff(
        self,
        tiffname: str = "OpenGL",
        minsuffix: int = 1,
        maxsuffix: int = 999,
        every: int = 5,
    ) -> None:
        """TIFF related parameters.

        Parameters
        ----------
        tiffname : str
            Root filename for TIFF files, which may include path information if
            desired. Default is “OpenGL”, which leads to the first TIFF being saved as
            “OpenGL001.tif”.
        minsuffix : int
            Initial suffix number of TIFF files that are saved.  Default value
            is 1.
        maxsuffix : int
            Largest possible suffix number of TIFF files that are saved.  Once
            this value has been reached, additional TIFFs cannot be saved.
            Default value is 999.
        every: int
            ``every`` is the number of simulation timesteps that should elapse
            between subsequent snapshots.
        """
        k = super().setTiffParams(every, str(tiffname), minsuffix, maxsuffix)
        assert k == _smoldyn.ErrorCode.ok

    def setLight(
        self,
        index: int,
        ambient: Color,
        diffuse: Color,
        specular: Color,
        pos: List[float],
    ) -> None:
        """Set the parameters for a light source, for use with opengl_better
        quality graphics. Parameters “ambient”, “diffuse”, “specular” are for
        the light’s colors, which are then specified with either a word or in
        the values as red, green, blue, and optionally alpha.

        Parameters
        ----------
        index: int
            The light index should be between 0 and 7.
        ambient:
            ambient color
        diffuse:
            diffuse color
        specular:
            specular color
        pos : List[float]
            Light’s 3-dimensional position, which is specified as x, y, and z.
            Lights specified this way are automatically enabled (turned on).
        """
        k = super().setLightParams(
            index, ambient.rgba, diffuse.rgba, specular.rgba, pos
        )
        assert k == _smoldyn.ErrorCode.ok

    def run(
        self,
        stop: float,
        dt: float,
        *,
        start: float = 0.0,
        display: bool = True,
        overwrite: bool = False,
        accuracy: None | float = None,
        log_level: int = 3,
        quit_at_end: bool = False,
    ) -> None:
        """run the simulation.

        Parameters
        ----------
        stop :
            stop time
        dt :
            dt
        start :
            start (default 0.0)
        accuracy :
            accuracy
        overwrite:
            Overwrite existing data files.
        display:
            Show graphics (default True)
        quit_at_end:
            When set to `True`, smoldyn won't ask for Shift+Q at the end of simulation
            to close the OpenGL window. Same effect can be achieved by setting
            the environment variable SMOLDYN_NO_PROMPT. It is useful for
            running tests in batch mode.
        """

        self.stop = float(stop)
        self.dt = float(dt)
        self.start = float(start)
        if accuracy is not None:
            self.accuracy = float(accuracy)

        super().setSimTimes(self.start, self.stop, self.dt)

        self.quitatend = quit_at_end

        assert self.dt > 0.0, f"dt can't be <= 0.0! dt={self.dt}"
        assert self.stop > 0.0, f"stop time can't be <= 0.0! stop={self.stop}"

        k = super().runSim(self.stop, self.dt, display, overwrite)
        assert _smoldyn.ErrorCode.ok == k, f"Expected ErrorCode.ok, got {k}"

    def runUntil(
        self, stop: float, dt: float, display: bool = True, overwrite: bool = False
    ) -> None:
        """Run simulation until a given time.

        Parameters
        ----------
        stop :
            stop
        dt :
            dt

        display : bool
            Enable gui display (default `True`)?

        overwrite : bool
            Overwrite existing data (default `False`)?
        """
        self.stop = stop
        self.dt = float(dt)

        assert self.dt > 0.0, f"dt can't be <= 0.0! dt={self.dt}"
        assert self.stop > 0.0, f"stop time can't be <= 0.0! stop={self.stop}"
        super().runUntil(self.stop, self.dt, display, overwrite)

    def updateSim(self) -> None:
        """updateSim"""
        super().updateSim()

    def display(self) -> None:
        k = super().displaySim()
        assert k == _smoldyn.ErrorCode.ok

    def addCommand(self, cmd: str, cmd_type: str, **kwargs: float | int | str) -> None:
        """Add command.

        Parameters
        ----------
        cmd: str
            Command string.

        cmd_type: str
            Type of command. Optional when `from_string` is set to `True`. Then
            the type is to be included in the `cmd` string itself.

            Use capital letter version for integer queue.  ‘b’ or 'B' for
            before the simulation.  ‘a’ or 'A' for after the simulation.  ‘e’
            or 'E' for every time step during the simulation.  ‘@’ or '&' for a
            single command execution at time `time`.  ‘n’ or 'N' for every n’th
            iteration of the simulation.  ‘i’ or 'I' for a fixed time interval.
            The command is executed over the period from on to off with
            intervals of at least dt (the actual intervals will only end at the
            times of simulation time steps).  ‘x’ for a fixed time multiplier.
            The command is executed at on, then on+dt, then on+dt*xt, then
            on+dt*xt2, and so forth.  ‘j’ for every ``dtit`` step with a set
            starting iteration and stopping iteration.

        kwargs: dict
            kwargs of :func:`_smoldyn.Command()`.

        """
        if "step" not in kwargs:
            kwargs["step"] = self.dt
        super().addCommand(cmd, cmd_type[0], **kwargs)

    def addCommandStr(self, cmd: str) -> None:
        """Add command using a single string. See the Smoldyn's User Manual for
        details.

        Parameters
        ----------
        cmd : str
            Command string
        """
        super().addCommandStr(cmd)

    # mapping.
    def addSpecies(
        self,
        name: str,
        state: str = "soln",
        color: Union[ColorType, Dict[str, ColorType]] = {"soln": "black"},
        difc: Union[float, Dict[str, float]] = 0.0,
        display_size: float = 3,
        mol_list: str = "",
    ) -> Species:
        """Add species to Simulation

        See :class:`smoldyn.Species` docs.
        """
        # Create a species.
        s = Species(super(), name, state, color, difc, display_size, mol_list)
        return s

    def addMolecules(
        self,
        species: Species,
        number: int,
        pos: List[float] = [],
        lowpos: List[float] = [],
        highpos: List[float] = [],
    ) -> None:
        """See :func:`Species.addToSolution`"""
        species.addToSolution(number, pos, lowpos, highpos)

    def addSurface(self, name: str, *, panels: List[Panel]) -> Surface:
        """See :class:`Surface` docs"""
        return Surface(simulation=super(), name=name, panels=panels)

    def addBidirectionalReaction(
        self,
        name: str,
        subs: List[SpeciesWithState],
        prds: List[SpeciesWithState],
        kf: float,
        kb: float = 0.0,
        *,
        binding_radius: float = 0.0,
        reaction_probability: float = 0.0,
        compartment: Compartment | None = None,
        surface: Surface | None = None,
    ) -> BidirectionalReaction:
        return BidirectionalReaction(
            super(),
            name,
            subs,
            prds,
            kf,
            kb,
            binding_radius=binding_radius,
            reaction_probability=reaction_probability,
            compartment=compartment,
            surface=surface,
        )

    def addReaction(
        self,
        name: str,
        subs: List[SpeciesWithState],
        prds: List[SpeciesWithState],
        *,
        rate: float = 0.0,
        compartment: Optional[Compartment] = None,
        surface: Optional[Surface] = None,
        binding_radius: float = 0.0,
        reaction_probability: float = 0.0,
    ) -> Reaction:
        """Add a reaction (unidirectional)

        See Reaction for details.
        """
        return Reaction(
            super(),
            name,
            subs,
            prds,
            rate=rate,
            compartment=compartment,
            surface=surface,
            binding_radius=binding_radius,
            reaction_probability=reaction_probability,
        )

    def setSimParams(
        self,
        *,
        accuracy: Optional[float] = None,
        gauss_table_size: Optional[int] = None,
        quit_at_end: Optional[bool] = None,
    ) -> None:
        """Set simulation parameters. Only the given parameters are set.

        Parameters
        ----------
        accuracy : float
            Simulation accuracy, on a scale from 0 to 10 (``accuracy``
            statement). Also available as the ``accuracy`` property.
        gauss_table_size : int
            Size of the lookup table for Gaussian-distributed random numbers,
            which needs to be an integer power of two (``gauss_table_size``).
        quit_at_end : bool
            Whether to quit at the end of the simulation, rather than leaving
            the graphics window open (``quit_at_end``). Also available as the
            ``quitatend`` property.
        """
        params: Dict[str, float] = {}
        if accuracy is not None:
            params["accuracy"] = accuracy
        if gauss_table_size is not None:
            params["gauss_table_size"] = gauss_table_size
        if quit_at_end is not None:
            params["quit_at_end"] = 1 if quit_at_end else 0
        for param, value in params.items():
            k = super().setSimParams(param, value)
            if k != _smoldyn.ErrorCode.ok:
                raise RuntimeError(f"setSimParams failed for {param}={value}: {k}")

    def setOutputFormat(
        self, format: Optional[str] = None, precision: Optional[int] = None
    ) -> None:
        """Set the format of output files written by runtime commands.

        Parameters
        ----------
        format : str
            ``"ssv"`` for space-separated values (default) or ``"csv"`` for
            comma-separated values (``output_format`` statement).
        precision : int
            Number of significant digits for numbers in output files
            (``output_precision`` statement).
        """
        k = super().setOutputFormat(format or "", -1 if precision is None else precision)
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"setOutputFormat failed: {k}")

    def addSpeciesGroup(self, name: str, species: List[Union[Species, str]] = []) -> None:
        """Create the species group `name`, or add species to it
        (``species_group`` statement). Groups can be used in place of species
        names in statements and rules.

        Parameters
        ----------
        name : str
            Group name, which cannot be a species name.
        species : List[Species or str]
            Species to add; strings may include wildcards. If empty, an empty
            group is created.
        """
        names = [sp if isinstance(sp, str) else sp.name for sp in species] or [""]
        for sname in names:
            k = super().addSpeciesGroup(name, sname)
            if k != _smoldyn.ErrorCode.ok:
                raise RuntimeError(f"addSpeciesGroup failed for group '{name}': {k}")

    def addSpeciesRule(
        self,
        pattern: str,
        state: SpeciesState = "soln",
        *,
        difc: Optional[float] = None,
        drift: Optional[List[float]] = None,
        difm: Optional[List[float]] = None,
        mol_list: Optional[str] = None,
        display_size: Optional[float] = None,
        color: Optional[ColorType] = None,
    ) -> None:
        """Set properties of all species that match `pattern`, including
        species generated later by rule expansion (``difc_rule``,
        ``drift_rule``, ``difm_rule``, ``mol_list_rule``,
        ``display_size_rule``, and ``color_rule`` statements). Only the given
        properties are set.

        Parameters
        ----------
        pattern : str
            Species pattern, e.g. ``"A*"``.
        state : SpeciesState
            State of the matching molecules (default 'soln'), or 'all'.
        difc : float
            Diffusion coefficient.
        drift : List[float]
            Drift vector, with one value per dimension.
        difm : List[float]
            Diffusion matrix, with dim*dim values.
        mol_list : str
            Name of a molecule list, which needs to exist already.
        display_size : float
            Display size.
        color :
            Display color.
        """

        def _check(k: _smoldyn.ErrorCode, what: str) -> None:
            if k != _smoldyn.ErrorCode.ok:
                raise RuntimeError(f"addSpeciesRule failed setting {what} for '{pattern}': {k}")

        ms = _toMS(state)
        if difc is not None or drift is not None or difm is not None:
            k = super().setSpeciesMobilityRule(
                pattern, ms, difc if difc is not None else -1.0, drift or [], difm or []
            )
            _check(k, "mobility")
        if mol_list is not None:
            _check(super().setMolListRule(pattern, ms, mol_list), "mol_list")
        if display_size is not None or color is not None:
            rgba = list(Color(color).rgba) if color is not None else []
            k = super().setMoleculeStyleRule(
                pattern, ms, display_size if display_size is not None else -1.0, rgba
            )
            _check(k, "style")

    def addSurfaceDriftRule(
        self,
        pattern: str,
        surface: Union[Surface, str],
        drift: List[float],
        *,
        state: SpeciesState = "all",
        panelshape: Union[str, _smoldyn.PanelShape] = "all",
    ) -> None:
        """Set the surface drift of all surface-bound species that match
        `pattern` (``surface_drift_rule`` statement). See
        :py:meth:`Species.setSurfaceDrift` for the parameters.
        """
        sname = surface if isinstance(surface, str) else surface.name
        ps = (
            _smoldyn.PanelShape.__members__[panelshape]
            if isinstance(panelshape, str)
            else panelshape
        )
        k = super().setSpeciesSurfaceDriftRule(pattern, _toMS(state), sname, ps, drift)
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"addSurfaceDriftRule failed for '{pattern}': {k}")

    def setVariable(self, name: str, value: float) -> None:
        """Define or change the variable `name` (``variable`` statement).
        Statements use a variable's value when they are read, so changing a
        variable does not change parameters that were already set from it.
        Runtime commands read their arguments each time they run, so they use
        the current value.
        """
        k = super().setVariable(name, value)
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"setVariable failed for '{name}': {k}")

    def getVariable(self, name: str) -> float:
        """Return the value of the variable `name`."""
        return float(super().getVariable(name))

    def addReactionRule(
        self,
        name: str,
        subs: List[SpeciesPattern],
        prds: List[SpeciesPattern],
        *,
        rate: float = -1.0,
        compartment: Optional[Compartment] = None,
        surface: Optional[Surface] = None,
    ) -> ReactionRule:
        """Add a reaction rule. See ReactionRule for details."""
        return ReactionRule(
            super(),
            name,
            subs,
            prds,
            rate=rate,
            compartment=compartment,
            surface=surface,
        )

    def expandRules(self, iterations: Union[int, str] = "all") -> None:
        """Expand rules to generate species and reactions (``expand_rules``
        statement).

        Parameters
        ----------
        iterations : int or str
            Number of expansion iterations, ``"all"`` (default) to expand
            completely, or ``"otf"`` (``"on-the-fly"``) to expand during the
            simulation as needed.
        """
        if isinstance(iterations, str):
            iterations = {"all": -1, "otf": -2, "on-the-fly": -2}[iterations]
        assert iterations >= 0 or iterations in (-1, -2), "Invalid iterations"
        k = super().expandRules(iterations)
        if k != _smoldyn.ErrorCode.ok:
            raise RuntimeError(f"expandRules failed: {k}")

    def addPartition(self, name: str, value: float) -> Partition:
        """Sets the virtual partitions in the simulation volumne. Two
        specialization is avaiable: :py:class:`MoleculePerBox`, and
        :py:class:`Box`.

        See also
        --------
        :py:class:`MoleculePerBox`
        :py:class:`Box`
        """
        return Partition(super(), name, value)

    def addBox(self, size: float) -> Box:
        """See Box.__init__"""

        warnings.warn(
            "This function is deprecated. Please use setPartitions in the future",
            DeprecationWarning,
        )
        return Box(super(), size)

    def addMoleculePerBox(self, size: float) -> MoleculePerBox:
        """See MoleculePerBox.__init__"""

        warnings.warn(
            "This function is deprecated. Please use setPartitions in the future",
            DeprecationWarning,
        )
        return MoleculePerBox(super(), size)

    def addCompartment(
        self, name: str, *, surface: Union[str, Surface], point: List[float]
    ) -> Compartment:
        return Compartment(super(), name, surface=surface, point=point)

    def addOutputData(self, dataname: str) -> None:
        """Declares the data table called `dataname`, enabling output into it by
        one or more runtime commands. Spaces are not permitted in the dataname.

        Parameters
        ----------
        dataname : str
            dataname (spaces are not allowed)

        Returns
        -------
        None

        """
        assert " " not in dataname, f"spaces not allowed in dataname: '{dataname}'"
        r = super().addOutputData(dataname)
        assert r == _smoldyn.ErrorCode.ok, f"Failed to add output data {r}"

    def getOutputData(self, dataname: str, erase: bool = True) -> List[List[float]]:
        """Returns data that have been recorded by an observation command (e.g. molcount).

        Parameters
        ----------
        dataname : str
            dataname

        erase: bool
            When set to `True`, the original data is cleared after a copy  of data
            is returned to the user.

        Returns
        -------
        List[List[float]]
            A matrix of float.
        """
        assert " " not in dataname, f"Must not contain spaces: '{dataname}'"
        y: List[List[float]] = super().getOutputData(dataname, erase)
        return y

    def connect(
        self,
        func: Callable[[float, List[float]], float],
        target: Callable[[float], None],
        step: int,
        args: List[float] = [],
    ) -> None:
        """Connect a arbitrary Python function to Simulation. The function will
        be called at every 'step'

        Parameters
        ----------
        func :
            Python function to connect. It must have two arguments. The first
            argument is always simulation time 't'. the second argument is a
            list of float which refers to `args` (4th argument of this function)
        target :
            Target function or a property. The return value of the connected function
            i.e., func is the argument to this function.
        step : int
            The connected function func is called after these many simulation
            steps.
        args : List[float]
            argument of func (passed by reference). Any change in this list will
            be visible to func.

        Example
        -------

        >>> import math
        >>> s = smoldyn.Simulation(low=(0, 0), high=(10, 10))
        >>> a = s.addSpecies("a", color="black", difc=0.1)
        >>> avals = []

        >>> def func(t, args):
        >>>     global a, avals
        >>>     x, y = args
        >>>     avals.append((t, a.difc["soln"]))
        >>>     return x * math.sin(t) + y

        >>> def target(val):
        >>>     global a
        >>>     a.difc = val

        >>> s.connect(func, target, step=10, args=[1, 1])
        >>> s.run(100, dt=1)
        >>> avals
        [(1.0, 0.1),
         (11.0, 1.8414709848078965),
         (21.0, 9.793449296524592e-06),
         (31.0, 1.836655638536056),
         (41.0, 0.595962354676935),
         (51.0, 0.841377331195291),
         (61.0, 1.6702291758433747),
         (71.0, 0.03388222999160706),
         (81.0, 1.9510546532543747),
         (91.0, 0.37011200572554614)]
        """
        super().connect(func, target, step, args)

    def addPath2D(self, *points: Tuple[float, float], closed: bool = False) -> Path2D:
        return Path2D(*points, simulation=super(), closed=closed)

    def addPort(
        self,
        *,
        name: str,
        surface: Union[Surface, str],
        panel: str,
    ) -> Port:
        return Port(super(), name=name, surface=surface, panel=panel)

    def addPanel(
        self,
        *,
        name: str = "",
        shape: _smoldyn.PanelShape = _smoldyn.PanelShape.none,
        neighbors: List[Panel] = [],
    ) -> Panel:
        return Panel(name=name, shape=shape, neighbors=neighbors, simulation=super())

    def addTriangle(
        self, *, vertices: Sequence[Sequence[float]] = [[]], name: str = ""
    ) -> Triangle:
        return Triangle(simulation=super(), vertices=vertices, name=name)

    def addRectangle(
        self,
        corner: Sequence[float],
        dimensions: Sequence[float],
        axis: str,
        name: str = "",
    ) -> Rectangle:
        return Rectangle(
            simulation=super(),
            corner=corner,
            dimensions=dimensions,
            axis=axis,
            name=name,
        )

    def addSphere(
        self,
        *,
        center: List[float],
        radius: float,
        slices: int,
        stacks: int = 0,
        name: str = "",
    ) -> Sphere:
        return Sphere(
            simulation=super(),
            center=center,
            radius=radius,
            slices=slices,
            stacks=stacks,
            name=name,
        )

    def addHemisphere(
        self,
        *,
        center: List[float],
        radius: float,
        vector: List[float],
        slices: int,
        stacks: int,
        name: str = "",
    ) -> Hemisphere:
        return Hemisphere(
            simulation=super(),
            center=center,
            radius=radius,
            vector=vector,
            slices=slices,
            stacks=stacks,
            name=name,
        )

    def addCylinder(
        self,
        *,
        start: List[float],
        end: List[float],
        radius: float,
        slices: int,
        stacks: int,
        name: str = "",
    ) -> Cylinder:
        return Cylinder(
            simulation=super(),
            start=start,
            end=end,
            radius=radius,
            slices=slices,
            stacks=stacks,
            name=name,
        )

    def addDisk(
        self,
        *,
        center: List[float],
        radius: float,
        slices: int,
        vector: List[float],
        name: str = "",
    ) -> Disk:
        return Disk(
            simulation=super(),
            center=center,
            radius=radius,
            slices=slices,
            vector=vector,
            name=name,
        )
