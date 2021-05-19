"""Template for writing Smoldyn model in Python

Use standard docstring to list basic file information here, including your
name, the development date, what this file does, the model name if you want
one, units used, the file version, distribution terms, etc.

Enzymatic reactions on a surface, by Steve Andrews, October 2009. Modified by
Dilawar Singh, 2020. This model is in the public domain. Units are microns and seconds.
The model was published in Andrews (2012) Methods for Molecular Biology, 804:519.
It executes a Michaelis-Menten reaction within and on the surface of a 2D circle.
"""

__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn 

# Model parameters
K_FWD = 0.001    # substrate-enzyme association reaction rate
K_BACK = 1       # complex dissociation reaction rate
K_PROD = 1       # complex reaction rate to product

# Simulation starts with declaring a Simulation object with the system boundaries.
s = smoldyn.Simulation(low=[-1, -1], high=[1, 1])

# Molecular species and their properties
# Species: S=substrate, E=enzyme, ES=complex, P=product
# Type `help(smoldyn.Species)` in Python console to see all parameters.
S = s.addSpecies("S", difc=3, color=dict(all="green"), display_size=dict(all=0.02))
E = s.addSpecies("E", color=dict(all="darkred"), display_size=dict(all=0.03))
P = s.addSpecies("P", difc=3, color=dict(all="darkblue"), display_size=dict(all=0.02))
ES = s.addSpecies("ES", color=dict(all="orange"), display_size=dict(all=0.03))

# Surfaces in and their properties. Each surface requires at least one panel.
# Add action to `both` faces for surface. You can also use `front` or `back`
# as well. Here, `all` molecules reflect at both surface faces.
sph1 = smoldyn.Sphere(center=(0, 0), radius=1, slices=50)
membrane = s.addSurface("membrane", panels=[sph1])
membrane.setAction('both', [S, E, P, ES], "reflect")
membrane.setStyle('both', color="black", thickness=1)

# Define a compartment, which is region inside the 'membrane' surface.
inside = s.addCompartment(name="inside", surface=membrane, point=[0, 0])

# Chemical reactions. Here, E + S <-> ES -> P
r1 = s.addBidirectionalReaction(
    "r1", subs=[(E,"front"), (S,"bsoln")], prds=[(ES,"front")], kf=K_FWD, kb=K_BACK)
r1.reverse.productPlacement("pgemmax", 0.2)

r2 = s.addReaction(
    "r2", subs=[(ES, "front")], prds=[(E, "front"), (P, "bsoln")], rate=K_PROD)

# Place molecules for initial condition
inside.addMolecules(S, 500)
membrane.addMolecules((E, "front"), 100)

# Output and other run-time commands
s.setOutputFile('templateout.txt', True)
s.addCommand("molcountheader templateout.txt", "B")
s.addCommand("molcount templateout.txt", "N", step=10)

s.setGraphics(
    "opengl_good", bg_color="white", frame_thickness=1,
    text_display=["time", S, (E, "front"), (ES, "front"), P] )
s = s.run(stop=10, dt=0.01)
