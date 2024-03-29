"""NOTE: This does not work with pytest since this scripts use global variables.

See https://stackoverflow.com/a/38802415/1805129
"""

__author__           = "Dilawar Singh"
__email__            = "dilawar.s.rajput@gmail.com"


import smoldyn
import random
import math

random.seed(0)
smoldyn.seed = 0

# First argument is always t.
spa, spb = None, None
avals, bvals = [], []

expected_a = [
    (0.0, 0.1),
    (10.0, 0.844422),
    (20.0, 10.757954),
    (30.0, 20.420572),
    (40.0, 30.258917),
    (50.0, 40.511275),
    (60.0, 50.404934),
    (70.0, 60.783799),
    (80.0, 70.303313),
    (90.0, 80.476597),
    (100.0, 90.583382),
    (110.0, 100.908113),
    (120.0, 110.504687),
    (130.0, 120.281838),
    (140.0, 130.755804),
    (150.0, 140.618369),
    (160.0, 150.250506),
    (170.0, 160.909746),
    (180.0, 170.982785),
    (190.0, 180.810217),
]

expected_b = [
    (0.0, 1),
    (10.0, 1.0),
    (20.0, 0.455979),
    (30.0, 1.912945),
    (40.0, 0.011968),
    (50.0, 1.745113),
    (60.0, 0.737625),
    (70.0, 0.695189),
    (80.0, 1.773891),
    (90.0, 0.006111),
]


def tuple_isclose(a, b) -> bool:
    res = True
    for _a, _b in zip(a, b):
        if not math.isclose(_a, _b):
            res = False
    return res


def new_dif_1(t, args):
    global spa, avals
    print("calling new_dif_1")
    x, y = args
    # note that a.difc is not still updated.
    avals.append((t, spa.difc["soln"]))
    return t + random.random()


def new_dif_2(t, args):
    global spb, bvals
    print("calling new_dif_2")
    x, y = args
    # note that b.difc is not still updated.
    bvals.append((t, spb.difc["soln"]))
    return x * math.sin(t) + y


def test_connect_1():
    global spa, avals
    s = smoldyn.Simulation(low=(0, 0), high=(10, 10))
    spa = s.addSpecies("a", color="black", difc=0.1)
    s.connect(new_dif_1, "spa.difc", step=10, args=[1, 1])
    s.run(200, dt=1)
    assert len(avals) == len(expected_a)
    for _x, _y in zip(avals, expected_a):
        print("test_connect_1", _x, _y)
        assert tuple_isclose(_x, _y), (_x, _y)


def test_connect_2():
    global spb, bvals
    s1 = smoldyn.Simulation(low=(0, 0), high=(10, 10))
    spb = s1.addSpecies("b", color="blue", difc=1)
    s1.connect(new_dif_2, "spb.difc", step=1, args=[1, 1])
    s1.run(stop=100, dt=10)
    assert len(bvals) == len(expected_b)
    for _x, _y in zip(bvals, expected_b):
        print("test_connect_2", _x, _y)
        assert tuple_isclose(_x, _y), (_x, _y)


def main():
    global avals, bvals
    print("==== test connect 1")
    test_connect_1()
    print("==== test connect 2")
    test_connect_2()


if __name__ == "__main__":
    main()
