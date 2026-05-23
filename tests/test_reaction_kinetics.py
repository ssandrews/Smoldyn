"""Kinetic tests for well-known reactions.

Each test runs a short Smoldyn simulation of a textbook reaction, samples
the trajectory via ``addOutputData`` + ``molcount``, and asserts that the
observed molecule counts match the analytical ODE solution.

A fixed RNG seed (42) and large initial counts (N0 >= 2000) keep noise
small enough for the tight tolerances used here. The pattern follows
``tests/test_prd_placement.py``: per-test Simulation, no graphics, retrieve
the data table after ``sim.run`` returns. The molcount output layout is
``[time, count_species_0, count_species_1, ...]`` in species-declaration
order.
"""

import numpy as np
import pytest

import smoldyn as S


SEED = 42
N0 = 2000


# ---------- First-order decay ----------


def test_first_order_decay():
    """A -> 0 with rate k. Analytical: N(t) = N0 * exp(-k*t)."""
    k = 0.5
    sim = S.Simulation(low=[0, 0, 0], high=[10, 10, 10], quit_at_end=True)
    sim.setRandomSeed(SEED)
    a = sim.addSpecies("A", difc=1.0)
    sim.addMolecules(a, number=N0, lowpos=[0, 0, 0], highpos=[10, 10, 10])
    sim.addReaction("decay", subs=[a], prds=[], rate=k)

    sim.addOutputData("counts")
    dt = 0.01
    sim.addCommand("molcount counts", "N", step=10)  # every 0.1 s
    sim.run(stop=10.0, dt=dt, display=False)

    data = np.array(sim.getOutputData("counts", erase=False))
    t = data[:, 0]
    obs = data[:, 1]
    expected = N0 * np.exp(-k * t)

    # Sample several points across 0.5 to 4 half-lives. Tolerance is 3σ of
    # the binomial std N0*p*(1-p) with p=exp(-k*t) — appropriate for shot
    # noise that grows in relative terms as counts decay.
    for t_check in (1.0, 2.0, 4.0, 8.0):
        idx = int(np.argmin(np.abs(t - t_check)))
        p = np.exp(-k * t[idx])
        std = np.sqrt(N0 * p * (1 - p))
        assert abs(obs[idx] - expected[idx]) < 3 * std, (
            f"At t={t[idx]:.2f}: observed N_A={obs[idx]:.1f}, "
            f"expected {expected[idx]:.1f} ± 3σ={3 * std:.1f}"
        )


# ---------- Reversible isomerization ----------


def test_reversible_isomerization():
    """A <-> B with kf, kb. Equilibrium [B]/[A] = kf/kb."""
    kf, kb = 1.0, 0.5
    sim = S.Simulation(low=[0, 0, 0], high=[10, 10, 10], quit_at_end=True)
    sim.setRandomSeed(SEED)
    a = sim.addSpecies("A", difc=1.0)
    b = sim.addSpecies("B", difc=1.0)
    sim.addMolecules(a, number=N0, lowpos=[0, 0, 0], highpos=[10, 10, 10])
    sim.addBidirectionalReaction("iso", subs=[a], prds=[b], kf=kf, kb=kb)

    sim.addOutputData("counts")
    sim.addCommand("molcount counts", "N", step=10)
    sim.run(stop=20.0, dt=0.01, display=False)

    data = np.array(sim.getOutputData("counts", erase=False))
    n_a = data[:, 1]
    n_b = data[:, 2]

    # Conservation: every recorded step must satisfy N_A + N_B = N0 exactly.
    assert np.all(n_a + n_b == N0), "stoichiometric conservation violated"

    # Equilibrium: 1/(kf+kb) = 0.67 s, so the second half of a 20 s run is
    # safely in steady state. Mean over the tail should match kf/kb = 2.
    tail = data[len(data) // 2 :]
    mean_a = tail[:, 1].mean()
    mean_b = tail[:, 2].mean()
    expected_a = N0 * kb / (kf + kb)
    expected_b = N0 * kf / (kf + kb)
    assert mean_a == pytest.approx(expected_a, rel=0.05), (
        f"mean N_A={mean_a:.1f}, expected {expected_a:.1f}"
    )
    assert mean_b == pytest.approx(expected_b, rel=0.05), (
        f"mean N_B={mean_b:.1f}, expected {expected_b:.1f}"
    )


# ---------- Bimolecular irreversible ----------


def test_bimolecular_irreversible():
    """A + B -> C with rate k and N_A0 = N_B0 = N0.

    For equal initial counts the integrated 2nd-order rate law gives
    1/[A](t) - 1/[A0] = k*t, equivalently [A](t) = [A0] / (1 + k*[A0]*t).

    Smoldyn applies a macroscopic rate constant via the Smoluchowski
    binding-radius model; with sufficient diffusion the trajectory follows
    the deterministic ODE.
    """
    k = 0.01  # macroscopic rate constant in (volume / time / molecule) units
    box = 5.0
    volume = box**3  # 125
    sim = S.Simulation(low=[0, 0, 0], high=[box, box, box], quit_at_end=True)
    sim.setRandomSeed(SEED)
    a = sim.addSpecies("A", difc=5.0)
    b = sim.addSpecies("B", difc=5.0)
    c = sim.addSpecies("C", difc=5.0)
    sim.addMolecules(a, number=N0, lowpos=[0, 0, 0], highpos=[box, box, box])
    sim.addMolecules(b, number=N0, lowpos=[0, 0, 0], highpos=[box, box, box])
    sim.addReaction("bind", subs=[a, b], prds=[c], rate=k)

    sim.addOutputData("counts")
    sim.addCommand("molcount counts", "N", step=10)
    sim.run(stop=15.0, dt=0.005, display=False)

    data = np.array(sim.getOutputData("counts", erase=False))
    t = data[:, 0]
    n_a = data[:, 1]
    n_b = data[:, 2]
    n_c = data[:, 3]

    # Stoichiometric conservation: N_A(t) + N_C(t) = N0, N_B(t) + N_C(t) = N0.
    assert np.all(n_a + n_c == N0), "A/C conservation violated"
    assert np.all(n_b + n_c == N0), "B/C conservation violated"

    # Symmetry: N_A(t) == N_B(t) at every step.
    assert np.all(n_a == n_b), "A and B should remain equal by symmetry"

    # Compare counts to deterministic 2nd-order rate law at sampled times.
    a0 = N0 / volume  # concentration
    for t_check in (2.0, 5.0, 10.0):
        idx = int(np.argmin(np.abs(t - t_check)))
        expected_conc = a0 / (1 + k * a0 * t[idx])
        expected_n = expected_conc * volume
        assert n_a[idx] == pytest.approx(expected_n, rel=0.10), (
            f"At t={t[idx]:.2f}: observed N_A={n_a[idx]:.1f}, "
            f"expected {expected_n:.1f}"
        )


# ---------- Reversible binding equilibrium ----------


def test_reversible_binding_equilibrium():
    """A + B <-> C with kf, kb. Reaction-quotient ratio over the steady-state
    tail should equal kf/kb (the equilibrium constant).
    """
    kf, kb = 0.01, 0.5
    box = 5.0
    volume = box**3
    sim = S.Simulation(low=[0, 0, 0], high=[box, box, box], quit_at_end=True)
    sim.setRandomSeed(SEED)
    a = sim.addSpecies("A", difc=5.0)
    b = sim.addSpecies("B", difc=5.0)
    c = sim.addSpecies("C", difc=5.0)
    sim.addMolecules(a, number=N0, lowpos=[0, 0, 0], highpos=[box, box, box])
    sim.addMolecules(b, number=N0, lowpos=[0, 0, 0], highpos=[box, box, box])
    sim.addBidirectionalReaction("bind", subs=[a, b], prds=[c], kf=kf, kb=kb)

    sim.addOutputData("counts")
    sim.addCommand("molcount counts", "N", step=10)
    sim.run(stop=30.0, dt=0.005, display=False)

    data = np.array(sim.getOutputData("counts", erase=False))
    n_a = data[:, 1]
    n_b = data[:, 2]
    n_c = data[:, 3]

    # Stoichiometric conservation.
    assert np.all(n_a + n_c == N0), "A/C conservation violated"
    assert np.all(n_b + n_c == N0), "B/C conservation violated"

    # Trajectory should be relaxing toward equilibrium: tail noise is much
    # smaller than the early transient.
    n_total = len(data)
    early = data[: n_total // 4, 3]
    late = data[3 * n_total // 4 :, 3]
    assert late.std() < early.std(), (
        f"trajectory not settling: early std={early.std():.1f}, "
        f"late std={late.std():.1f}"
    )

    # Equilibrium constant check on the steady-state tail.
    # Reaction quotient Q = [C] / ([A] * [B]) in concentration units.
    a_eq = late.mean()  # tail of A (column 1) — recompute
    a_eq = data[3 * n_total // 4 :, 1].mean()
    b_eq = data[3 * n_total // 4 :, 2].mean()
    c_eq = data[3 * n_total // 4 :, 3].mean()
    q = (c_eq / volume) / ((a_eq / volume) * (b_eq / volume))
    k_eq = kf / kb
    assert q == pytest.approx(k_eq, rel=0.15), (
        f"reaction quotient Q={q:.3f}, expected K_eq=kf/kb={k_eq:.3f}"
    )


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
