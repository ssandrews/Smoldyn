# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2019-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

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


@dataclass
class Boundaries:
    low: List[float]
    high: List[float]
    types: List[str] = ['r']
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
