# Original author: Upinder Bhalla, NCBS Bangalore
# Modified by Dilawar Singh for smoldyn integration.

import smoldyn
import random
import math


def build_model():
    """
    Size of bouton: 1 cubic µm
    diameter of Synaptic Vesicle (SV): 40 nm
    Diff of SV: 0.024 um^2/s (2400 nm^2/s)
    1px = 1nm throughout.
    """
    global r1_, bouton_
    s = smoldyn.Simulation(low=[-500, -500], high=[1500, 1500])

    # a molecule that self-generate
    sv = s.addSpecies(
        "SV", difc=dict(all=2400, front=10), color="blue", display_size=10
    )
    sv.addToSolution(100, lowpos=(0, 0), highpos=(1000, 1000))
    s.addReaction("svgen", [sv], [sv, sv], rate=1e-6)
    svFused = s.addSpecies("VSOpen", color="blue", display_size=10)

    # intermediate speficies with a lifetime.
    B = s.addSpecies("B", color="red", difc=10000, display_size=2)
    decay = s.addReaction("decay", subs=[B], prds=[], rate=math.log(2) / 20e-3)

    # A very non-interesting surface.
    path = s.addPath2D((1000, 0), (1000, 1000), (0, 1000), (0, 0))
    bouton_ = s.addSurface("bouton", panels=path.panels)
    bouton_.setStyle("both", color="blue")

    # try to add all actions.
    bouton_.setAction("both", [sv], "reflect")
    bouton_.setAction("both", [sv], "trans")
    bouton_.setAction("back", [sv], "absorb")
    bouton_.setAction("back", [sv], "jump")
    bouton_.setAction("back", [sv], "port")

    # NOTE: action 'mult', 'no' and 'none' causes error here.

    # this is the bottom surface of bouton. This is sticky for synaptic
    # vesciles
    rect1 = smoldyn.Rectangle(corner=(0, 0), dimensions=[1000], axis="+y")
    bottom = s.addSurface("boutonBottom", panels=[rect1])
    bottom.setStyle("both", color="red")
    bottom.setAction("back", B, "reflect")
    bottom.setAction("back", B, "trans")
    bottom.setAction("back", B, "absorb")
    bottom.setAction("back", B, "jump")
    bottom.setAction("back", B, "port")

    return s


def test_action():
    s = build_model()
    print("[INFO] Starting simulation ...")
    s.run(stop=20, dt=0.001)
    print("Done")


if __name__ == "__main__":
    test_action()
