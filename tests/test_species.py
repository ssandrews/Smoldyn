"""Unit tests for the Species class in smoldyn.smoldyn.

Species objects are always constructed through `Simulation.addSpecies()` —
calling `Species(sim, ...)` directly with the Python-wrapper Simulation
double-dispatches into the wrapper's `addSpecies` and breaks the internal
ErrorCode contract (see smoldyn.py:2089 — the wrapper passes `super()`).

Each test creates its own Simulation so libsmoldyn state is fresh per test.
This is safe inside one pytest process: each Simulation() owns a distinct
simptr; the note in source/python/CMakeLists.txt:141 about pytest applies
to sharing state across test modules, not within one.
"""

import pytest

import smoldyn as S
from smoldyn import _smoldyn


def _sim():
    return S.Simulation(low=[0, 0, 0], high=[10, 10, 10])


# ---------- Construction ----------


class TestSpeciesConstruction:
    def test_minimal_construction(self):
        sim = _sim()
        sp = sim.addSpecies("A")
        assert isinstance(sp, S.Species)
        assert sp.name == "A"
        assert sp.state == "soln"
        # Diffusion default is 0 in the "soln" state.
        assert sp.difc == {"soln": 0.0}

    def test_construction_with_all_args(self):
        sim = _sim()
        sp = sim.addSpecies("X", difc=1.5, color="red", display_size=4)
        assert sp.name == "X"
        assert sp.difc == {"soln": 1.5}

    def test_empty_name_rejected(self):
        # Species.__init__ asserts on empty name (smoldyn.py:126).
        sim = _sim()
        with pytest.raises(AssertionError):
            sim.addSpecies("")

    def test_invalid_state_rejected(self):
        sim = _sim()
        with pytest.raises(NameError):
            sim.addSpecies("A", state="not_a_state")

    def test_valid_states(self):
        # The three states accepted by both Species and setSpeciesMobility.
        # Other documented states ("bsoln", "up", "down") parse but are
        # rejected by libsmoldyn at mobility-set time.
        sim = _sim()
        for state in ("soln", "front", "back"):
            sp = sim.addSpecies(f"sp_{state}", state=state, difc={state: 0.0})
            assert sp.state == state

    def test_repr_includes_name_and_state(self):
        sim = _sim()
        sp = sim.addSpecies("A", difc=0.5)
        r = repr(sp)
        assert "A" in r
        assert "soln" in r


# ---------- Diffusion coefficient ----------


class TestSpeciesDifc:
    def test_scalar_difc_wraps_to_dict(self):
        sim = _sim()
        sp = sim.addSpecies("A", difc=2.5)
        assert sp.difc == {"soln": 2.5}

    def test_dict_difc_preserved(self):
        sim = _sim()
        sp = sim.addSpecies("M", state="front", difc={"front": 1.0, "back": 0.5})
        assert sp.difc == {"front": 1.0, "back": 0.5}

    def test_difc_setter_replaces_scalar(self):
        sim = _sim()
        sp = sim.addSpecies("A", difc=1.0)
        sp.difc = 3.0
        assert sp.difc == {"soln": 3.0}

    def test_difc_setter_accepts_dict(self):
        sim = _sim()
        sp = sim.addSpecies("M", state="front", difc=0.1)
        sp.difc = {"front": 2.0, "back": 0.25}
        assert sp.difc == {"front": 2.0, "back": 0.25}


# ---------- Color ----------


class TestSpeciesColor:
    def test_named_color_resolves_to_rgba(self):
        sim = _sim()
        sp = sim.addSpecies("A", color="red")
        assert "soln" in sp.color
        soln_color = sp.color["soln"]
        # Red name should resolve to a non-black rgba with alpha 1.
        rgba = soln_color.rgba
        assert rgba[3] == 1.0
        assert rgba[0] > 0  # red channel set

    def test_rgb_tuple_color(self):
        sim = _sim()
        sp = sim.addSpecies("A", color=(0.1, 0.2, 0.3))
        soln_color = sp.color["soln"]
        assert soln_color.rgba == (0.1, 0.2, 0.3, 1.0)

    def test_dict_color_per_state(self):
        sim = _sim()
        sp = sim.addSpecies(
            "M", state="front", color={"front": "blue", "back": "green"}
        )
        assert set(sp.color.keys()) == {"front", "back"}

    def test_color_setter_replaces(self):
        sim = _sim()
        sp = sim.addSpecies("A", color="black")
        sp.color = (1.0, 0.0, 0.0)
        soln_color = sp.color["soln"]
        assert soln_color.rgba == (1.0, 0.0, 0.0, 1.0)


# ---------- Display size ----------


class TestSpeciesDisplaySize:
    def test_scalar_display_size_keyed_by_all(self):
        sim = _sim()
        sp = sim.addSpecies("A", display_size=4)
        # smoldyn.py:252 — scalar size goes under MolecState.all.
        sizes = dict(sp.display_size)
        assert _smoldyn.MolecState.all in sizes
        assert sizes[_smoldyn.MolecState.all] == 4

    def test_display_size_setter(self):
        sim = _sim()
        sp = sim.addSpecies("A", display_size=2)
        sp.display_size = 7.0
        sizes = dict(sp.display_size)
        assert sizes[_smoldyn.MolecState.all] == 7.0


# ---------- addToSolution ----------


class TestAddToSolution:
    def test_add_with_fixed_position(self):
        sim = _sim()
        sp = sim.addSpecies("X", difc=1.0)
        # Should not raise; libsmoldyn returns ok.
        sp.addToSolution(5, pos=[1, 1, 1])

    def test_add_with_bounds(self):
        sim = _sim()
        sp = sim.addSpecies("X", difc=1.0)
        sp.addToSolution(10, lowpos=[0, 0, 0], highpos=[5, 5, 5])

    def test_rejects_non_soln_species(self):
        sim = _sim()
        sp = sim.addSpecies("Mfront", state="front", difc={"front": 0.0})
        with pytest.raises(AssertionError):
            sp.addToSolution(1, pos=[0, 0, 0])


# ---------- Independence across Simulations ----------


class TestSpeciesAcrossSimulations:
    def test_same_name_in_separate_simulations(self):
        # Two independent Simulation objects can each carry a species "A".
        sim1 = _sim()
        sim2 = _sim()
        a1 = sim1.addSpecies("A", difc=1.0)
        a2 = sim2.addSpecies("A", difc=2.0)
        assert a1.difc == {"soln": 1.0}
        assert a2.difc == {"soln": 2.0}


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
