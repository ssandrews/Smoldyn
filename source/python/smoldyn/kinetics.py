# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2020-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

from dataclasses import dataclass, field
from typing import List

from smoldyn import _smoldyn
from smoldyn.config import __logger__


@dataclass
class NullSpecies:
    name: str = ""
    state: str = _smoldyn.MolecState.__members__["all"]


class Species:
    def __init__(self, name, state, color="", difc=0.0, size=1.0, mol_list=""):
        self.name: str = name
        assert self.name

        k = _smoldyn.addSpecies(self.name)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add molecule: {k}"

        self.state = _smoldyn.MolecState.__members__[state]

        self._difc: float = difc
        self._color = color
        self._size: float = size

        self.difc: float = self._difc
        self.size: float = self._size
        self.color: str = self._color

        self._mol_list: str = mol_list
        if mol_list:
            self.mol_list: str = mol_list

    def __repr__(self):
        return f"<Molecule: {self.name}, difc={self.difc}, state={self.state}>"

    def setStyle(self):
        k = _smoldyn.setMoleculeStyle(self.name, self.state, self.size,
                                      self.color)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set style on {self}, {k}"

    @property
    def difc(self) -> float:
        return self._difc

    @difc.setter
    def difc(self, difc: float):
        assert self.state, "Please set 'state' first"
        self._difc = difc
        k = _smoldyn.setSpeciesMobility(self.name, self.state, self.difc)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set mobility: {k}"

    @property
    def mol_list(self) -> str:
        return self._mol_list

    @mol_list.setter
    def mol_list(self, val):
        k = _smoldyn.addMolList(val)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to add mollist: {k}"
        k = _smoldyn.setMolList(self.name, self.state, val)
        assert (k == _smoldyn.ErrorCode.ok
                ), f"Failed to set mol_list={val} on {self}: {k}"
        self._mol_list = val

    @property
    def color(self) -> str:
        return self._color

    @color.setter
    def color(self, color: str):
        self._color = color
        self.setStyle()

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, size: float):
        self._size = size
        self.setStyle()


class HalfReaction:
    def __init__(self, subs: List[Species], prds: List[Species], k, rname=""):
        assert len(subs) < 3, "At most two reactants are supported."
        r1 = subs[0]
        r2 = subs[1] if len(subs) == 2 else NullSpecies()
        if not rname:
            rname = 'r%d' % id(self)
        assert rname
        k = _smoldyn.addReaction(rname, r1.name, r1.state, r2.name, r2.state,
                                 [x.name for x in prds],
                                 [x.state for x in prds], k)
        if k != _smoldyn.ErrorCode.ok:
            __logger__.warning(f" Reactant1 : {r1}")
            __logger__.warning(f" Reactant2 : {r2}")
            __logger__.warning(f" Subtrate  : {subs}")
            __logger__.warning(f" Products  : {prds}")
            raise RuntimeError(f"Failed to add reaction: {k}")


class Reaction:
    def __init__(self, subs: List[Species], prds: List[Species], kf, kb=0.0):
        assert len(subs) < 3, "At most two reactants are supported"
        self.forward = HalfReaction(subs, prds, kf)
        self.backbard = None
        if kb > 0:
            self.backbard = HalfReaction(prds, subs, kb)
