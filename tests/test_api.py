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
    """Tests geometric shape by converting them to quivalent smoldyn text
    lines.

    >>> test_rectangles()
    panel PanelShape.rect +x 0 10 1 10 20 A
    panel PanelShape.tri  0 0 0 1 1 1 1 3 -10 t1
    panel PanelShape.sph  0 -10 0 3 10 10 s1
    panel PanelShape.cyl  20 30 70 20 50 70 4 20 20 cyl1
    panel PanelShape.disk  20 20 20 10 10 20 20 d1
    """
    r1 = S.Rectangle((0, 10, 1), dimensions=(10, 20), axis="+x", name="A")
    print(r1.toText())
    tr = S.Triangle(vertices=[[0, 0, 0], [1, 1, 1], [1, 3, -10]], name="t1")
    print(tr.toText())
    sph = S.Sphere(center=[0, -10, 0], radius=3, slices=10, stacks=10, name="s1")
    print(sph.toText())
    cyl = S.Cylinder(start=[20, 30, 70], end=[20, 50, 70], radius=4, slices=20,stacks=20, name="cyl1")
    print(cyl.toText())
    disk = S.Disk(center=[20, 20, 20], radius=10, vector=[10, 20, 20], name="d1")
    print(disk.toText())


if __name__ == "__main__":
    import doctest

    doctest.testmod()
