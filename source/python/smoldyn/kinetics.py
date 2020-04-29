# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2020-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

from dataclasses import dataclass, field
from typing import List

from smoldyn import _smoldyn, obj

class Species():

    def __init__(self, name, state, color='', difc=0.0, size=1.0):
        self.name: str = name
        assert self.name
        k = obj().addSpecies(self.name)
        assert k == _smoldyn.ErrorCode.ok, k
        self.state = _smoldyn.MS.__members__[state]
        self.size = size

        self._difc = difc
        self.difc: float = self._difc
        self._color = color 
        self.color: str = self._color

    @property
    def difc(self) -> float:
        return self._difc

    @difc.setter
    def difc(self, difc: float):
        assert self.state, "Please set 'state' first"
        self._difc = difc
        k = obj().setSpeciesMobility(self.name, self.state, self.difc)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set mobility: {k}"

    @property
    def color(self) -> str:
        return self._color

    @color.setter
    def color(self, color: str):
        self._color = color
        k = obj().setSpeciesStyle(self.name, self.state, self.size, color)
        assert k == _smoldyn.ErrorCode.ok, f"Failed to set color on {self}"


class Reaction:
    def __init__(self, subs: List[Species], prds: List[Species], kf, kb=0.0):
        print(f"[INFO ] Adding reaction {subs} -> {prds}")
        if kb > 0:
            print("[TODO] Add reverse reaction as well.")
