"""
This does not work with pytest
"""
__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn
import random
import math

random.seed(0)
smoldyn.seed = 0

# First argument is always t.
a, avals = None, []

expected_a = [
    (1.0, 1),
    (11.0, 1.841471),
    (21.0, 1e-05),
    (31.0, 1.836656),
    (41.0, 0.595962),
    (51.0, 0.841377),
    (61.0, 1.670229),
    (71.0, 0.033882),
    (81.0, 1.951055),
    (91.0, 0.370112),
]


def new_dif(t, args):
    global a, avals
    x, y = args
    # note that b.difc is not still updated.
    avals.append((t, a.difc["soln"]))
    return x * math.sin(t) + y


def update_difc(val):
    global a
    a.difc = val


def test_connect():
    global a, avals
    s = smoldyn.Simulation(low=(0, 0), high=(10, 10))
    a = s.addSpecies("a", color="black", difc=0.1)
    s.connect(new_dif, update_difc, step=10, args=[1, 1])
    s.run(100, dt=1)
    for a, b in zip(avals[1:], expected_a[1:]):
        print(a, b)
        assert math.isclose(a[1], b[1], rel_tol=1e-6, abs_tol=1e-6), (a[1], b[1])


def main():
    test_connect()


if __name__ == "__main__":
    main()
