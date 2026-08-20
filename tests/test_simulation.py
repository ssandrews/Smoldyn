"""Unit tests for the Simulation wrapper in smoldyn.smoldyn.

Covers construction (1D/2D/3D, boundary type variants), bounds round-trip,
initial state via count(), accuracy / seed / quitatend, output data
plumbing, time-window setters, and Simulation.fromFile.

Each test creates its own Simulation, isolating libsmoldyn state per test
within this single pytest process.
"""

from pathlib import Path
import pytest

import smoldyn as S

FIXTURE_DIR = Path(__file__).parent


# ---------- Construction ----------


class TestSimulationConstruction:
    def test_3d_construction(self):
        sim = S.Simulation(low=[0, 0, 0], high=[10, 20, 30])
        assert sim.simptr is not None
        low, high = sim.getBoundaries()
        assert low == [0.0, 0.0, 0.0]
        assert high == [10.0, 20.0, 30.0]

    def test_2d_construction(self):
        sim = S.Simulation(low=[0, 0], high=[5, 5])
        low, high = sim.getBoundaries()
        assert len(low) == 2
        assert low == [0.0, 0.0]
        assert high == [5.0, 5.0]

    def test_1d_construction(self):
        sim = S.Simulation(low=[0], high=[7])
        low, high = sim.getBoundaries()
        assert low == [0.0]
        assert high == [7.0]

    def test_empty_low_rejected(self):
        # Asserted at smoldyn.py:1710.
        with pytest.raises(AssertionError):
            S.Simulation(low=[], high=[1])

    def test_empty_high_rejected(self):
        with pytest.raises(AssertionError):
            S.Simulation(low=[0], high=[])

    def test_dimension_mismatch_rejected(self):
        # smoldyn.py:1722 — when boundary_type is a single char, low and high
        # must have matching dimensions.
        with pytest.raises(AssertionError):
            S.Simulation(low=[0, 0], high=[1, 1, 1])


# ---------- Boundary types ----------


class TestBoundaryTypes:
    def test_single_char_broadcasts(self):
        # "r" should broadcast to each dimension.
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1], boundary_type="r")
        assert sim.simptr is not None

    def test_string_per_dimension(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1], boundary_type="rrr")
        assert sim.simptr is not None

    def test_list_per_dimension(self):
        sim = S.Simulation(low=[0, 0], high=[1, 1], boundary_type=["r", "r"])
        assert sim.simptr is not None

    def test_deprecated_types_alias_warns(self):
        # `types` is the deprecated name for `boundary_type` (smoldyn.py:1713).
        with pytest.warns(DeprecationWarning):
            S.Simulation(low=[0, 0], high=[1, 1], types="r")


# ---------- count() and dimension reporting ----------


class TestCount:
    def test_fresh_sim_has_zero_objects(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        c = sim.count()
        assert c["species"] == 0
        assert c["surface"] == 0
        assert c["compartment"] == 0

    def test_count_reports_3d(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        assert sim.count()["dim"] == 3

    def test_count_reports_2d(self):
        sim = S.Simulation(low=[0, 0], high=[1, 1])
        assert sim.count()["dim"] == 2

    def test_species_count_grows(self):
        # libsmoldyn lazily creates an internal "empty" species the first time
        # one is added, so the absolute count after N adds is N + 1. We just
        # assert that the count strictly increases.
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        before = sim.count()["species"]
        sim.addSpecies("A")
        after_one = sim.count()["species"]
        sim.addSpecies("B")
        after_two = sim.count()["species"]
        assert before < after_one < after_two


# ---------- Properties ----------


class TestSimProperties:
    def test_accuracy_default(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        # Default accuracy from libsmoldyn is 10.0.
        assert sim.accuracy == 10.0

    def test_accuracy_from_constructor(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1], accuracy=5.0)
        assert sim.accuracy == 5.0

    def test_accuracy_setter(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        sim.accuracy = 7.5
        assert sim.accuracy == 7.5

    def test_quit_at_end_default_false(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        assert sim.quitatend is False

    def test_quit_at_end_from_constructor(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1], quit_at_end=True)
        assert sim.quitatend is True

    def test_set_random_seed(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        sim.setRandomSeed(42)
        assert sim.seed == 42
        sim.setRandomSeed(99)
        assert sim.seed == 99

    def test_seed_from_constructor(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1], seed=42)
        assert sim.seed == 42

    def test_negative_seed_is_skipped(self):
        # seed < 0 means "don't set", so libsmoldyn keeps its default RNG state.
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1], seed=-1)
        assert sim.simptr is not None


# ---------- Output data ----------


class TestOutputData:
    def test_add_output_data(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        sim.addOutputData("moments")

    def test_get_empty_returns_empty_list(self):
        # Reading a populated-but-empty data table should return [], not crash.
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        sim.addOutputData("moments")
        assert sim.getOutputData("moments", erase=False) == []

    def test_dataname_with_space_rejected(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        with pytest.raises(AssertionError):
            sim.addOutputData("two words")

    def test_get_with_space_rejected(self):
        # The space check in getOutputData (smoldyn.py:2231) runs before the
        # libsmoldyn call, so this exercises the wrapper guard, not the C side.
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        with pytest.raises(AssertionError):
            sim.getOutputData("two words")


# ---------- Commands ----------


class TestCommands:
    def test_add_command_with_step(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        sim.dt = 1.0  # addCommand defaults step to self.dt
        sim.addCommand("echo step", cmd_type="N", step=10)

    def test_add_command_str(self):
        sim = S.Simulation(low=[0, 0, 0], high=[1, 1, 1])
        # Should not raise; the C++ side parses the command-type prefix.
        sim.addCommandStr("A echo hello")


# ---------- fromFile ----------


class TestFromFile:
    def test_load_min_model(self):
        # min.txt is the existing integration-test fixture in tests/.
        sim = S.Simulation.fromFile(FIXTURE_DIR / "min.txt", "")
        assert sim is not None
        assert sim.simptr is not None
        # min.txt declares stop=500, dt=0.002 (see test_sim_from_file.py:25-27).
        assert sim.start == 0.0
        assert sim.stop == 500
        assert sim.dt == 0.002

    def test_load_with_string_path(self):
        sim = S.Simulation.fromFile(str(FIXTURE_DIR / "min.txt"), "")
        assert sim is not None

    def test_missing_file_raises(self):
        with pytest.raises(FileNotFoundError):
            S.Simulation.fromFile(FIXTURE_DIR / "does_not_exist.txt", "")


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
