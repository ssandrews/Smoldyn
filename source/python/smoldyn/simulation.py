# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2020-, Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

__all__ = ['Boundaries', 'Model']

from dataclasses import dataclass, field
from typing import List

from smoldyn import _smoldyn
from .kinetics import Species

from smoldyn.config import __logger__

@dataclass
class Boundaries:
    low: List[float]
    high: List[float]
    types: List[str]
    dim: field(init=False) = 0

    def __post_init__(self):
        assert len(self.low) == len(self.high), "Size mismatch"
        if len(self.types) == 1:
            self.types = self.types * len(self.low)
        self.dim = len(self.low)
        _smoldyn.setBoundaries(list(zip(self.low, self.high)))
        __logger__.debug('Getting boundaries', _smoldyn.getBoundaries())
        assert _smoldyn.getDim() == self.dim, (_smoldyn.getDim(), self.dim)


class Model(object):
    """Model class.
    """

    def __init__(self, bounds):
        self.bounds = bounds

    @property
    def dim(self):
        return self.bound.dim

    def run(self, stoptime, dt=1e-3):
        _smoldyn.run(stoptime, dt)

    def addMolecules(self, mol : Species, N, pos):
        __logger__.debug(f"Adding {N} of {mol} with pos {pos}")
        res = _smoldyn.addSolutionMolecules(mol.name, N
                , self.bounds.low, self.bounds.high)
        assert res == _smoldyn.ErrorCode.ok

    def runSim(self, t, dt):
        _smoldyn.runSim(t, dt)
