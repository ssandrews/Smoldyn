# Original author: Upinder Bhalla, NCBS Bangalore

import smoldyn as S
import random
import math

r1_ = None
bouton_ = None


def generate_spike(t, args):
    x = random.random()
    if x < 0.1:
        return 1 + x
    return x


def update_kf(val):
    global r1_, bouton_
    r1_.forward.kf = 1000 ** val
    c = min(1.0, val)
    bouton_.both.setStyle(color=[c, c, c])


def build_model_smoldyn():
    """
    Size of bouton: 1 cubic µm
    diameter of Synaptic Vesicle (SV): 40 nm
    Diff of SV: 0.024 µm^2/s (2400 nm^2/s)

    1px = 1nm throughout.
    """
    global r1_, bouton_
    S.setBounds(low=[-500, -500], high=[1500, 1500])
    sv = S.Species("SV", difc=dict(all=2400, front=10), color="blue", display_size=10)
    # add a reaction with generates the sv at a fixed rate. Its better to
    # split so location of the new sv is inside the box.
    S.Reaction("svgen", [sv], [sv, sv], kf=1e-6)
    sv.addToSolution(100, lowpos=(0, 0), highpos=(1000, 1000))

    # fused vesicle.
    svFused = S.Species("VSOpen", color="blue", display_size=10)

    # neutotransmitter. The concentration has the half-life of 2ms (PMID:
    # 19844813), that is, rate is 0.693/2e-3, k ~ 346 per sec
    trans = S.Species("trans", color="red", difc=10000, display_size=2)
    S.Reaction("decay", subs=[trans], prds=[], kf=math.log(2) / 20e-3)

    # BOUTON
    path = S.Path2D((1000, 0), (1000, 1000), (0, 1000), (0, 0))
    bouton_ = S.Surface("bouton", panels=path.panels)
    bouton_.both.setStyle(color="blue")
    bouton_.both.addAction([sv], "reflect")

    # this is the bottom surface of bouton. This is sticky for synaptic
    # vesciles
    rect1 = S.Rectangle(corner=(0, 0), dimensions=[1000], axis="+y")
    bottom = S.Surface("boutonBottom", panels=[rect1])
    bottom.both.setStyle(color="red")

    bottom.setRate(
        sv, "fsoln", "front", rate=10, revrate=0.1
    )  # SV stick to bottom of the bouton.
    bottom.back.addAction(trans, "reflect")  # but it reflect neurotranmitter

    # They move to outside of bouton, this value is dependant on the membrane
    # potential
    bottom.setRate(sv, "front", "bsoln", rate=10, new_species=svFused)

    # Open vesicle turns into 1000 to 2000 or neurotransmitters.
    r1_ = S.Reaction("open2trans", subs=[svFused], prds=[trans] * 1000, kf=100)
    S.connect(generate_spike, update_kf, step=20)

    s = S.Simulation(stop=1000, step=0.005)
    s.addGraphics("opengl", iter=4)
    s.run()
    print("Done")


build_model_smoldyn()
