"""This does not work with pytest

https://stackoverflow.com/a/38802415/1805129
"""
__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn
import random
import math

random.seed(0)
smoldyn.seed = 0

# First argument is always t.
a, b = None, None
avals, bvals = [], []

expected_b = [
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

expected_a = [
    (101.0, 0.1),
    (111.0, 101.844422),
    (121.0, 111.757954),
    (131.0, 121.420572),
    (141.0, 131.258917),
    (151.0, 141.511275),
    (161.0, 151.404934),
    (171.0, 161.783799),
    (181.0, 171.303313),
    (191.0, 181.476597),
]


def new_dif_1(t, args):
    global b, bvals
    x, y = args
    # note that b.difc is not still updated.
    bvals.append((t, b.difc["soln"]))
    return x * math.sin(t) + y


def new_dif_2(t, args):
    global a, avals
    x, y = args
    # note that a.difc is not still updated.
    avals.append((t, a.difc["soln"]))
    return t + random.random()


def test_connect_1():
    global b, bvals
    s1 = smoldyn.Simulation(low=(0, 0), high=(10, 10))
    b = s1.addSpecies("b", color="blue", difc=1)
    s1.connect(new_dif_1, "b.difc", step=10, args=[1, 1])
    s1.run(stop=100, dt=10)
    for a, b in zip(bvals, expected_b):
        print(a, b)
        assert math.isclose(a[1], b[1]), (a[1], b[1])


def test_connect_2():
    global a, avals
    s = smoldyn.Simulation(low=(0, 0), high=(10, 10))
    a = s.addSpecies("a", color="black", difc=0.1)
    s.connect(new_dif_2, "a.difc", step=10, args=[1, 1])
    s.run(200, dt=1)
    for a, b in zip(avals, expected_a):
        print(a, b)
        assert math.isclose(a[1], b[1])


def main():
    global avals, bvals
    test_connect_1()
    test_connect_2()


if __name__ == "__main__":
    main()
