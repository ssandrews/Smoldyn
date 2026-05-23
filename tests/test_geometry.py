"""Unit tests for the Panel geometry classes (Rectangle, Triangle, Sphere,
Hemisphere, Cylinder, Disk) in smoldyn.smoldyn.

These tests cover the pure-Python construction surface: stored attributes,
the `pts` layout that feeds into `Simulation.addPanel`, axis parsing, and
`toText()` rendering. Each Panel can be constructed without a Simulation
(simulation=None) because Panels become "live" only when attached to a
Surface, so this file is pure unit-level coverage with no global state.
"""

import pytest

import smoldyn as S
from smoldyn import _smoldyn


# ---------- Rectangle ----------


class TestRectangle:
    def test_basic_construction(self):
        r = S.Rectangle(corner=(0, 10, 1), dimensions=(10, 20), axis="+x", name="A")
        assert r.name == "A"
        assert r.ctype == _smoldyn.PanelShape.rect
        assert r.axisstr == "+x"
        assert r.pts == [0, 10, 1, 10, 20]

    def test_axis_lowercased(self):
        r = S.Rectangle(corner=(0, 0), dimensions=(5,), axis="+X")
        assert r.axisstr == "+x"

    def test_negative_axis(self):
        r = S.Rectangle(corner=(0, 0, 0), dimensions=(1, 1), axis="-y")
        assert r.axisstr == "-y"
        assert r.pts == [0, 0, 0, 1, 1]

    def test_numeric_axis_accepted(self):
        # The constructor accepts "+0", "+1", "+2" as axis indices.
        r = S.Rectangle(corner=(0, 0, 0), dimensions=(1, 1), axis="+0")
        assert r.axisstr == "+0"
        assert r._axisIndex("0") == 0
        assert r._axisIndex("1") == 1
        assert r._axisIndex("2") == 2

    def test_axis_without_sign_rejected(self):
        with pytest.raises(AssertionError):
            S.Rectangle(corner=(0, 0, 0), dimensions=(1, 1), axis="x")

    def test_axis_invalid_letter_rejected(self):
        with pytest.raises(AssertionError):
            S.Rectangle(corner=(0, 0, 0), dimensions=(1, 1), axis="+q")

    def test_default_name_is_empty(self):
        r = S.Rectangle(corner=(0, 0), dimensions=(1,), axis="+x")
        assert r.name == ""

    def test_to_text(self):
        r = S.Rectangle(corner=(0, 10, 1), dimensions=(10, 20), axis="+x", name="A")
        assert r.toText() == "panel rect +x 0 10 1 10 20 A"

    def test_to_text_unnamed_strips_trailing_space(self):
        r = S.Rectangle(corner=(0, 0), dimensions=(1,), axis="+x")
        text = r.toText()
        assert text == text.rstrip()


# ---------- Triangle ----------


class TestTriangle:
    def test_construction_flattens_vertices(self):
        verts = [[0, 0, 0], [1, 1, 1], [1, 3, -10]]
        t = S.Triangle(vertices=verts, name="t1")
        assert t.name == "t1"
        assert t.ctype == _smoldyn.PanelShape.tri
        assert t.axisstr == ""
        assert t.pts == [0, 0, 0, 1, 1, 1, 1, 3, -10]
        assert t.vertices == verts

    def test_2d_triangle(self):
        t = S.Triangle(vertices=[[0, 0], [1, 0]])
        assert t.pts == [0, 0, 1, 0]

    def test_to_text(self):
        t = S.Triangle(vertices=[[0, 0, 0], [1, 1, 1], [1, 3, -10]], name="t1")
        assert t.toText() == "panel tri  0 0 0 1 1 1 1 3 -10 t1"


# ---------- Sphere ----------


class TestSphere:
    def test_construction(self):
        sph = S.Sphere(center=[0, -10, 0], radius=3, slices=10, stacks=10, name="s1")
        assert sph.ctype == _smoldyn.PanelShape.sph
        assert sph.center == [0, -10, 0]
        assert sph.radius == 3
        assert sph.slices == 10
        assert sph.stacks == 10
        assert sph.pts == [0, -10, 0, 3, 10, 10]

    def test_stacks_default_to_slices(self):
        sph = S.Sphere(center=[0, 0, 0], radius=1, slices=8)
        assert sph.stacks == 8
        assert sph.pts == [0, 0, 0, 1, 8, 8]

    def test_nonpositive_stacks_replaced_by_slices(self):
        sph = S.Sphere(center=[0, 0, 0], radius=1, slices=12, stacks=-5)
        assert sph.stacks == 12

    def test_zero_slices_rejected(self):
        with pytest.raises(AssertionError):
            S.Sphere(center=[0, 0, 0], radius=1, slices=0)

    def test_to_text(self):
        sph = S.Sphere(center=[0, -10, 0], radius=3, slices=10, stacks=10, name="s1")
        assert sph.toText() == "panel sph  0 -10 0 3 10 10 s1"


# ---------- Hemisphere ----------


class TestHemisphere:
    def test_construction(self):
        h = S.Hemisphere(
            center=[1, 2, 3],
            radius=4,
            vector=[0, 0, 1],
            slices=8,
            stacks=6,
            name="h1",
        )
        assert h.ctype == _smoldyn.PanelShape.hemi
        # pts layout: center, radius, vector, slices, stacks
        assert h.pts == [1, 2, 3, 4, 0, 0, 1, 8, 6]
        assert h.axisstr == ""

    def test_negative_radius_allowed(self):
        # Negative radius is a meaningful Smoldyn input (inverts surface front).
        h = S.Hemisphere(
            center=[0, 0, 0],
            radius=-2,
            vector=[1, 0, 0],
            slices=4,
            stacks=4,
        )
        assert h.radius == -2
        assert h.pts[3] == -2


# ---------- Cylinder ----------


class TestCylinder:
    def test_construction(self):
        c = S.Cylinder(
            start=[20, 30, 70],
            end=[20, 50, 70],
            radius=4,
            slices=20,
            stacks=20,
            name="cyl1",
        )
        assert c.ctype == _smoldyn.PanelShape.cyl
        # pts layout: start, end, radius, slices, stacks
        assert c.pts == [20, 30, 70, 20, 50, 70, 4, 20, 20]

    def test_to_text(self):
        c = S.Cylinder(
            start=[20, 30, 70],
            end=[20, 50, 70],
            radius=4,
            slices=20,
            stacks=20,
            name="cyl1",
        )
        assert c.toText() == "panel cyl  20 30 70 20 50 70 4 20 20 cyl1"


# ---------- Disk ----------


class TestDisk:
    def test_construction(self):
        d = S.Disk(
            center=[20, 20, 20],
            radius=10,
            vector=[10, 20, 20],
            slices=10,
            name="d1",
        )
        assert d.ctype == _smoldyn.PanelShape.disk
        # pts layout per smoldyn.py:687: center, radius, vector, slices
        assert d.pts == [20, 20, 20, 10, 10, 20, 20, 10]

    def test_negative_slices_rejected(self):
        with pytest.raises(AssertionError):
            S.Disk(center=[0, 0], radius=1, vector=[1, 0], slices=-1)

    def test_zero_slices_allowed(self):
        # smoldyn.py:679 only asserts `slices > -1`, so 0 must work.
        d = S.Disk(center=[0, 0], radius=1, vector=[1, 0], slices=0)
        assert d.slices == 0


# ---------- Panel base behavior ----------


class TestPanelBase:
    def test_default_name_when_unnamed(self):
        sph = S.Sphere(center=[0, 0, 0], radius=1, slices=4)
        # _getName falls back to "<ctype><index>" when name is empty.
        assert sph._getName(7) == f"{_smoldyn.PanelShape.sph}7"

    def test_explicit_name_wins_over_index(self):
        sph = S.Sphere(center=[0, 0, 0], radius=1, slices=4, name="mysph")
        assert sph._getName(7) == "mysph"

    def test_front_back_faces_present(self):
        r = S.Rectangle(corner=(0, 0), dimensions=(1,), axis="+x")
        assert r.front.name == _smoldyn.PanelFace.front
        assert r.back.name == _smoldyn.PanelFace.back


# ---------- Path2D ----------


class TestPath2D:
    def _sim(self):
        return S.Simulation(low=[0, 0], high=[10, 10])

    def test_axis_aligned_segments_become_rectangles(self):
        sim = self._sim()
        # Horizontal segment (theta=0) and vertical segment (theta=pi/2).
        p = S.Path2D((0, 0), (5, 0), (5, 5), simulation=sim)
        assert len(p.panels) == 2
        assert all(isinstance(panel, S.Rectangle) for panel in p.panels)

    def test_diagonal_segment_becomes_triangle(self):
        sim = self._sim()
        p = S.Path2D((0, 0), (1, 1), simulation=sim)
        assert len(p.panels) == 1
        assert isinstance(p.panels[0], S.Triangle)

    def test_closed_path_appends_first_point(self):
        sim = self._sim()
        p = S.Path2D((0, 0), (1, 0), (1, 1), simulation=sim, closed=True)
        assert p.points[0] == p.points[-1]
        # 3 segments after closing.
        assert len(p.panels) == 3


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
