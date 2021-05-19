# Original author: Upinder Bhalla, NCBS Bangalore

import smoldyn
import math
import numpy as np

try:
    import moose
    import rdesigneur as rd
    import gnuplotlib as gp
except Exception:
    print('[WARN] MOOSE/gnuplotlib are missing. To install'
          ' $ python3 -m pip install pymoose --pre --user '
          ' $ python3 -m pip install gnuplotlib')
    quit(0)

g1_ = gp.gnuplotlib(title="MOOSE simualtion: Soma")

#######################################
# MOOSE model
# A chemical oscillator.
######################################
rdes = rd.rdesigneur(
    chanProto=[["make_HH_Na()", "Na"], ["make_HH_K()", "K"]],
    chanDistrib=[["Na", "soma", "Gbar", "1200"], ["K", "soma", "Gbar", "360"]],
    stimList=[["soma", "1", ".", "inject", "((t%0.2)>0.1)*1e-8"]],
    plotList=[["soma", "1", ".", "Vm", "Membrane potential"]],
)
rdes.buildModel()
moose.reinit()
mooseT_, mooseY_ = [], []
elem_ = moose.element("/model/elec/soma[0]")

########################################
# SMOLDYN MODEL
########################################
r1_ = None
bouton_ = None
bottom_ = None

def run_moose(t, args):
    global mooseT_, mooseY_
    global elem_, g1_
    dt, N = args
    mooseT_.append(t)
    mooseY_.append(elem_.Vm)
    moose.start(dt*N)
    if len(mooseT_) == 220:
        mooseT_ = mooseT_[20:]
        mooseY_ = mooseY_[20:]
        g1_.plot(np.array(mooseT_), np.array(mooseY_))
    return elem_.Vm

def update_kf(val):
    global r1_, bouton_, bottom_
    kf = max(0.0, val) * 1e3 * 1e3
    r1_.setRate(kf)
    bottom_.setRate(sv, "fsoln", "front", rate=(10+kf), revrate=0.0)
    c = min(kf/300, 1)
    bouton_.setStyle('both', color=[c, c, c])


"""
Size of bouton: 1 cubic µm
diameter of Synaptic Vesicle (SV): 40 nm
Diff of SV: 0.024 µm^2/s (2400 nm^2/s)

1px = 1nm throughout.
"""
s = smoldyn.Simulation(low=[-500, -500], high=[1500, 1500])
sv = s.addSpecies(
    "SV",
    difc=dict(all=5000, front=100),
    color=dict(fsoln="blue", front="gray"),
    display_size=6,
)
sv.addToSolution(100, pos=(500, 500))

# fused vesicle.
svFused = s.addSpecies("VSOpen", color="red", display_size=10)
s.addReaction("prod", subs=[sv], prds=[sv,sv], rate=1e-2)

# neutotransmitter. The concentration has the half-life of 2ms (PMID:
# 19844813), that is, rate is 0.693/2e-3, k ~ 346 per sec
trans = s.addSpecies("trans", color="red", difc=10000, display_size=2)
s.addReaction("decay", subs=[trans], prds=[], rate=math.log(2)/20e-3)

# BOUTON
path = s.addPath2D((1000, 0), (550, 1000), (450, 1000), (0, 0))
bouton_ = s.addSurface("bouton", panels=path.panels)
bouton_.setStyle('both', color="blue")
bouton_.setAction('both', [sv], "reflect")

# this is the bottom surface of bouton. This is sticky for synaptic vesciles
rect1 = smoldyn.Rectangle(corner=(0,0), dimensions=[1000], axis="+y")
bottom_ = s.addSurface("boutonBottom", panels=[rect1])
bottom_.setStyle('both', color="red")

# SV stick to bottom of the bouton.
bottom_.setRate(sv, "fsoln", "front", rate=100, revrate=0)
# but it reflect neurotranmitter
bottom_.setAction('back', trans, "reflect")

# They also merge to the surface, this value is dependant on the membrane
# potential. See connect
bottom_.setRate(sv, "front", "bsoln", rate=100, new_species=svFused)

# Open vesicle turns into 1000 to 2000 or neurotransmitters. Note that rate of
# this reaction is updated below. We still need to add a non-zero value of
# rate.
r1_ = s.addReaction("open2trans", subs=[svFused], prds=[trans] * 255, rate=1)

# update functions.
dt, connectN = 0.0001, 10
s.connect(run_moose, update_kf, step=connectN, args=[dt, connectN])

s.addGraphics("opengl", iter=100, text_display="time SV")
s.run(100, dt=dt)
print("Done")
