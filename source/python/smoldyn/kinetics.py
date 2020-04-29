# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2020-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

from dataclasses import dataclass, field
from typing import List

from smoldyn import _smoldyn, obj

@dataclass
class Species:
    name: str
    _name: str = field(repr=False, init=False)
    state: str
    _state: _smoldyn.MS = field(repr=False, init=False)
    diffc: float
    _diffc: float = field(repr=False, init=False)
    color: str
    _color: str = field(repr=False, init=False)

    def __post_init__(self):
        obj().addSpecies(self.name)

    @property
    def difc(self) -> float:
        return self._diffc

    @difc.setter
    def diffc(self, difc: float):
        assert self._type, "Please set 'type' first"
        self._difc = difc
        k = obj().setSpeciesMobility(self.name, self._state, self._diffc)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set mobility on {self}"

    @property
    def color(self) -> str:
        return self._color

    @color.setter
    def color(self, color: str):
        self._color = color
        k = obj().setSpeciesColor(self.name, color)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set color on {self}"


class Reaction:
    def __init__(self, subs: List[Species], prds: List[Species], kf, kb=0.0):
        print(f"[INFO ] Adding reaction {subs} -> {prds}")
        if kb > 0:
            print("[TODO] Add reverse reaction as well.")
