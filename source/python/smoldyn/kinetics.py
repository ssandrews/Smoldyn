# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2020-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

from dataclasses import dataclass
from typing import List

import smoldyn

@dataclass
class Species:
    name: str
    diffc: float = 0.0
    color: str = 'black'


class Reaction():

    def __init__(self, subs: List[Species], prds: List[Species], kf, kb=0.0):
        print(f"[INFO ] Adding reaction {subs} -> {prds}")
        if kb > 0:
            print(f"[TODO] Add reverse reaction as well.")
        
