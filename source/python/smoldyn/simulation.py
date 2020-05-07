# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2020-, Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

__all__ = ['Model']

from dataclasses import dataclass, field
from typing import List

from smoldyn import _smoldyn
from .kinetics import Species
from .geometry import Boundaries

from smoldyn.config import __logger__


class Model(object):
    """Model class.
    """

    def __init__(self, bounds):
        self.bounds : Boundaries = bounds

    @property
    def dim(self):
        return self.bound.dim

    @property
    def accuracy(self):
        return _smoldyn.getAccuracy()

    @accuracy.setter
    def accuracy(self, accuracy: float):
        _smoldyn.setAccuracy(accuracy)

    def run(self, stoptime, dt=1e-3):
        _smoldyn.run(stoptime, dt)

    def addMolecules(self, mol : Species, N, pos):
        __logger__.debug(f"Adding {N} of {mol} with pos {pos}")
        res = _smoldyn.addSolutionMolecules(mol.name, N
                , self.bounds.low, self.bounds.high)
        assert res == _smoldyn.ErrorCode.ok

    def runSim(self, t, dt):
        _smoldyn.runSim(t, dt)
