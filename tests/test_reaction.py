"""Unit tests for Reaction and BidirectionalReaction in smoldyn.smoldyn.

Reactions need a Simulation with declared Species, so each test builds a
fresh Simulation and registers the reactants/products via
`sim.addSpecies(...)` (the supported construction path — see the note in
test_species.py).
"""

import pytest

import smoldyn as S


def _sim_with_species(*names):
    sim = S.Simulation(low=[0, 0, 0], high=[10, 10, 10])
    species = {n: sim.addSpecies(n, difc=1.0) for n in names}
    return sim, species


# ---------- Reaction construction ----------


class TestReactionConstruction:
    def test_unimolecular(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("decay", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        assert isinstance(r, S.Reaction)
        assert r.name == "decay"
        assert r.order == 1
        assert r.rate == 0.1  # rate is read back from libsmoldyn

    def test_bimolecular(self):
        sim, sp = _sim_with_species("A", "B", "C")
        r = sim.addReaction("bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], rate=0.5)
        assert r.order == 2
        assert len(r.subs) == 2
        assert len(r.prds) == 1

    def test_zero_order(self):
        # Zero substrates is allowed when there is at least one product.
        sim, sp = _sim_with_species("A")
        r = sim.addReaction("creation", subs=[], prds=[sp["A"]], rate=0.01)
        assert r.order == 0
        assert len(r.prds) == 1

    def test_degradation_to_nothing(self):
        # One substrate, no products is the standard degradation reaction.
        sim, sp = _sim_with_species("A")
        r = sim.addReaction("degrade", subs=[sp["A"]], prds=[], rate=0.05)
        assert r.order == 1
        assert len(r.prds) == 0

    def test_more_than_two_subs_rejected(self):
        # smoldyn.py:1338 — at most two reactants are supported.
        sim, sp = _sim_with_species("A", "B", "C", "D")
        with pytest.raises(AssertionError):
            sim.addReaction(
                "x", subs=[sp["A"], sp["B"], sp["C"]], prds=[sp["D"]], rate=1.0
            )

    def test_auto_name_when_empty(self):
        # Empty name gets auto-generated as f"r{id(self)}" (smoldyn.py:1329)
        # and that auto-name is what's registered with libsmoldyn.
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        assert r.name.startswith("r")
        assert r.name[1:].isdigit()
        # Rate must round-trip via the auto-name, proving libsmoldyn knows it.
        r.rate = 2.0
        assert r.rate == 2.0

    def test_tuple_form_substrate(self):
        # Substrates may be passed as (Species, state) tuples instead of bare
        # Species objects — used to target a specific surface-bound state.
        sim, sp = _sim_with_species("M", "P")
        r = sim.addReaction(
            "release",
            subs=[(sp["M"], "soln")],
            prds=[sp["P"]],
            rate=0.1,
        )
        assert r.order == 1


# ---------- Rate ----------


class TestReactionRate:
    def test_rate_setter_updates(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        r.rate = 2.5
        assert r.rate == 2.5

    def test_rate_setter_idempotent_same_value(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        r.rate = 2.5
        r.rate = 2.5
        assert r.rate == 2.5

    def test_rate_setter_rejects_negative(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        with pytest.raises(AssertionError):
            r.rate = -1.0


# ---------- Reaction probability ----------


class TestReactionProbability:
    def test_default_is_zero(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        assert r.reaction_probability == 0.0

    def test_set_via_constructor_unimolecular(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction(
            "x", subs=[sp["A"]], prds=[sp["B"]], reaction_probability=0.3
        )
        assert r.reaction_probability == 0.3

    def test_set_via_property_unimolecular(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        r.reaction_probability = 0.4
        assert r.reaction_probability == 0.4

    def test_bimolecular_above_one_rejected(self):
        sim, sp = _sim_with_species("A", "B", "C")
        r = sim.addReaction(
            "bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], rate=0.5
        )
        with pytest.raises(AssertionError):
            r.reaction_probability = 1.5

    def test_negative_rejected(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        with pytest.raises(AssertionError):
            r.reaction_probability = -0.1


# ---------- Binding radius ----------


class TestBindingRadius:
    def test_default_is_zero(self):
        sim, sp = _sim_with_species("A", "B", "C")
        r = sim.addReaction(
            "bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], rate=0.5
        )
        assert r.binding_radius == 0.0

    def test_set_via_constructor(self):
        sim, sp = _sim_with_species("A", "B", "C")
        r = sim.addReaction(
            "bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], binding_radius=0.2
        )
        assert r.binding_radius == 0.2

    def test_set_via_property(self):
        sim, sp = _sim_with_species("A", "B", "C")
        r = sim.addReaction(
            "bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], rate=0.5
        )
        r.binding_radius = 0.3
        assert r.binding_radius == 0.3

    def test_rejected_on_unimolecular(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        with pytest.raises(AssertionError):
            r.binding_radius = 0.5

    def test_negative_rejected(self):
        sim, sp = _sim_with_species("A", "B", "C")
        r = sim.addReaction(
            "bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], rate=0.5
        )
        with pytest.raises(AssertionError):
            r.binding_radius = -0.1


# ---------- Deprecated setRate shim ----------


class TestSetRateDeprecated:
    def test_emits_deprecation_warning(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        with pytest.warns(DeprecationWarning, match="setRate is deprecated"):
            r.setRate(2.0)
        assert r.rate == 2.0

    def test_sentinel_routes_to_probability(self):
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        with pytest.warns(DeprecationWarning):
            r.setRate(-1.0, reaction_probability=0.4)
        assert r.reaction_probability == 0.4


# ---------- Intersurface ----------


class TestIntersurface:
    def test_unimolecular_rejects_intersurface(self):
        # smoldyn.py:1454 — setIntersurface requires order == 2.
        sim, sp = _sim_with_species("A", "B")
        r = sim.addReaction("x", subs=[sp["A"]], prds=[sp["B"]], rate=0.1)
        with pytest.raises(AssertionError):
            r.setIntersurface([1])

    def test_rules_length_must_match_products(self):
        sim, sp = _sim_with_species("A", "B", "C")
        r = sim.addReaction(
            "bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], rate=0.5
        )
        with pytest.raises(AssertionError):
            r.setIntersurface([1, 2])  # 2 rules but only 1 product


# ---------- BidirectionalReaction ----------


class TestBidirectionalReaction:
    def test_forward_only_when_kb_zero(self):
        sim, sp = _sim_with_species("A", "B")
        br = sim.addBidirectionalReaction(
            "rxn", subs=[sp["A"]], prds=[sp["B"]], kf=0.5, kb=0.0
        )
        assert isinstance(br.forward, S.Reaction)
        assert br.reverse is None
        # When kb == 0, the forward reaction takes the bare name.
        assert br.forward.name == "rxn"

    def test_both_directions_when_kb_positive(self):
        sim, sp = _sim_with_species("A", "B")
        br = sim.addBidirectionalReaction(
            "rxn", subs=[sp["A"]], prds=[sp["B"]], kf=0.5, kb=0.25
        )
        assert isinstance(br.forward, S.Reaction)
        assert isinstance(br.reverse, S.Reaction)
        # smoldyn.py:1562 — when kb > 0, names are suffixed.
        assert br.forward.name == "rxnfwd"
        assert br.reverse.name == "rxnrev"

    def test_kf_kb_initial_values(self):
        sim, sp = _sim_with_species("A", "B")
        br = sim.addBidirectionalReaction(
            "rxn", subs=[sp["A"]], prds=[sp["B"]], kf=0.5, kb=0.25
        )
        assert br.kf == 0.5
        assert br.kb == 0.25

    def test_kf_setter(self):
        sim, sp = _sim_with_species("A", "B")
        br = sim.addBidirectionalReaction(
            "rxn", subs=[sp["A"]], prds=[sp["B"]], kf=0.5, kb=0.25
        )
        br.kf = 1.5
        assert br.kf == 1.5
        assert br.forward.rate == 1.5

    def test_kb_setter(self):
        sim, sp = _sim_with_species("A", "B")
        br = sim.addBidirectionalReaction(
            "rxn", subs=[sp["A"]], prds=[sp["B"]], kf=0.5, kb=0.25
        )
        br.kb = 0.75
        assert br.kb == 0.75
        assert br.reverse is not None
        assert br.reverse.rate == 0.75

    def test_kb_setter_requires_reverse(self):
        # When kb=0, reverse is None; the kb setter asserts reverse exists.
        sim, sp = _sim_with_species("A", "B")
        br = sim.addBidirectionalReaction(
            "rxn", subs=[sp["A"]], prds=[sp["B"]], kf=0.5, kb=0.0
        )
        with pytest.raises(AssertionError):
            br.kb = 1.0

    def test_reversed_subs_and_prds(self):
        # The reverse reaction swaps substrates and products.
        sim, sp = _sim_with_species("A", "B", "C")
        br = sim.addBidirectionalReaction(
            "bind", subs=[sp["A"], sp["B"]], prds=[sp["C"]], kf=0.5, kb=0.25
        )
        assert br.forward.order == 2
        assert br.reverse is not None
        assert br.reverse.order == 1  # one substrate (C) in the reverse direction


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
