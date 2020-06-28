"""
File: test_api.py
Author: Dilawar Singh
Email: dilawars@ncbs.res.in
Github: https://github.com/yourname
Description: 
    Tests for user api.
"""

import smoldyn as S


def test_rectangles():
    import matplotlib.pyplot as plt

    r1 = S.Rectangle((0, 10, 1), dimensions=(10, 20), axis="+x", name="A")
    print(r1.toText())
    tr = S.Triangle([[0, 0, 0], [1, 1, 1], [1, 3, -10]], name="t1")
    print(tr.toText())
    sph = S.Sphere([0, -10, 0], 3, name="s1")
    print(sph.toText())
    cyl = S.Cylinder([20, 30, 70], [20, 50, 70], 4, 20, 20, name="cyl1")
    print(cyl.toText())
    disk = S.Disk([20, 20, 20], 10, [10, 20, 20], name="d1")
    print(disk.toText())


def main():
    test_rectangles()


if __name__ == "__main__":
    main()
