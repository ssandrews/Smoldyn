# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2020-, Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

__all__ = ['Boundaries', 'Simulation']

from dataclasses import dataclass, field
from typing import List

import smoldyn

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
        smoldyn.obj().bounds = list(zip(self.low, self.high))
        assert smoldyn.obj().dim == self.dim


class Simulation(object):
    """Simulation class.
    
    for Simulation
    """

    def __init__(self, bounds):
        self.bounds = bounds

    def run(self, stoptime, dt=1e-3):
        smoldyn.obj().run(stoptime, dt)
