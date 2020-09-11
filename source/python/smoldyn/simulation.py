# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2020-, Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

__all__ = ['StateMonitor', 'Simulation']

import warnings
from smoldyn import _smoldyn
from .kinetics import Species
from .geometry import Boundaries

class StateMonitor(object):
    """State Monitor
    """
    def __init__(self, objs, state, **kwargs):
        self.objs = objs
        self.state = state
        self.t = []
        self._start = kwargs.get('start', _smoldyn.getTimeStart())
        self._stop = kwargs.get('stop', _smoldyn.getTimeStop())
        self._step = kwargs.get('step', _smoldyn.getTimeStep())
        self._multiplier = kwargs.get('multiplier', 1.0)
        setattr(self, state, {})
        for o in objs:
            getattr(self, state)[o] = []
        if state == 'molcount':
            self.initMolcount()

    def initMolcount(self):
        _smoldyn.addCommand('i', self._start, self._stop, self._step,
                self._multiplier, 'molcount')


class Simulation(object):
    """This is the top level class.
    """

    def __init__(self, stop:float, step:float, **kwargs):
        self.start = kwargs.get('start', 0.0)
        self.stop = stop
        self.step = step
        if kwargs.get('accuracy', 0.0):
            self.accuracry: float = kwargs['accuracy']

    def setGraphics(self, method:str, timestep:int, delay:int=0):
        k = _smoldyn.setGraphicsParams(method, timestep, delay)
        assert k == _smoldyn.ErrorCode.ok

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
