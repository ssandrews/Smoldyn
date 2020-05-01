# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2019-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"


from smoldyn import _smoldyn, obj
from dataclasses import dataclass


class Compartment:
    """Compartment
    """

    def __init__(self, arg):
        super().__init__()
        self.arg = arg


@dataclass
class Partition:
    name: str
    value: float

    def __post_init__(self):
        obj().setPartitions(self.name, self.value)


class MoleculePerBox(Partition):
    def __init__(self, size: float):
        super().__init__("molperbox", size)


class Box(Partition):
    def __init__(self, size):
        super().__init__("boxsize", size)
