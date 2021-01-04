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
    (0.0, 0.1),
    (10.0, 1.0),
    (20.0, 0.4559788891106302),
    (30.0, 1.9129452507276277),
    (40.0, 0.011968375907138173),
    (50.0, 1.7451131604793488),
    (60.0, 0.7376251462960712),
    (70.0, 0.6951893788977833),
    (80.0, 1.773890681557889),
    (90.0, 0.00611134607662478),
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
        print("test_connect", a, b)
        assert math.isclose(a[1], b[1], rel_tol=1e-6, abs_tol=1e-6), (a[1], b[1])


def main():
    test_connect()


if __name__ == "__main__":
    main()
