"""Template for writing Smoldyn model in Python

Use standard docstring to list basic file information here, including your
name, the development date, what this file does, the model name if you want
one, the file version, distribution terms, etc. 

TODO: Also, importantly, list the units used in this file, e.g. microns and
milliseconds.  This template file is here to be edited.  There is no need to
maintain any of the current text, or to keep any references to Smoldyn authors
and contributors, or the history of this file.

Enzymatic reactions on a surface, by Steve Andrews, October 2009.
This model is in the public domain.  Units are microns and seconds.
The model was published in Andrews (2012) Methods for Molecular Biology, 804:519.
It executes a Michaelis-Menten reaction within and on the surface of a 2D circle.
"""

__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn 

# Model parameters
K_FWD = 0.001  # substrate-enzyme association reaction rate
K_BACK = 1  # complex dissociation reaction rate
K_PROD = 1  # complex reaction rate to product

# Simulation starts with declaring an object of class Simulation. 
# It contains system boundaries (2D system bound between x=-1 to x=1 and y=-1 to
# y=1)
s = smoldyn.Simulation(low=[-1, -1], high=[1, 1])

# Molecular species and their properties
# Species: S=substrate, E=enzyme, ES=complex, P=product
# Diffusion coefficient is given by `difc` (in default units)
# `color` and `display_size` of the species is used in plotting.
#
# There are few other parameters. Type `help(smoldyn.Species)` in Python
# console to know about them.
S = s.addSpecies("S", difc=3, color=dict(all="green"), display_size=dict(all=0.02))
E = s.addSpecies("E", color=dict(all="darkred"), display_size=dict(all=0.03))
P = s.addSpecies("P", difc=3, color=dict(all="darkblue"), display_size=dict(all=0.02))
ES = s.addSpecies("ES", color=dict(all="orange"), display_size=dict(all=0.03))

# Surfaces in the system and their properties.
# Declare a sphere
s1 = smoldyn.Sphere(center=(0, 0), radius=1, slices=50)
# Surface requires at least one panel (Sphere in this case). Add this to
# Simulation
membrane = s.addSurface("membrane", panels=[s1])

# Add action to `both` faces for surface. You can also use `front` or `back`
# as well. `all` molecules reflect at both surface faces
membrane.setAction('both', [S, E, P, ES], "reflect")
membrane.setStyle('both', color="black", thickness=1)
inside = s.addCompartment(name="inside", surface=membrane, point=[0, 0])
inside.addMolecules(S, 500)  # Place molecules for initial condition

# Chemical reactions
# Association and dissociation reactions.
r1 = s.addBidirectionalReaction(
    "r1", subs=[(E, "front"), (S, "bsoln")], prds=[(ES, "front")], kf=K_FWD, kb=K_BACK
)
r1.reverse.productPlacement("pgemmax", 0.2)

# product formation reaction
r2 = smoldyn.Reaction("r2", subs=[(ES, "front")], prds=[(E, "front"), (P, "bsoln")], rate=K_PROD)

# surface_mol 100 E(front) membrane all all	# puts 100 E molecules on surface
membrane.addMolecules((E, "front"), 100)

# Output and other run-time commands
s.setOutputFile('templateout.txt', True)
s.addCommand(cmd="molcountheader templateout.txt", cmd_type="B")
s.addCommand(cmd="molcount templateout.txt", cmd_type="N", step=10)
# s.addCommandFromString('B molcountheader templateout.txt')
# s.addCommandFromString("N 10 molcount templateout.txt")
s.setGraphics(
    "opengl_good",
    bg_color="white",
    frame_thickness=1,
    text_display=["time", S, (E, "front"), (ES, "front"), P],
)
s = s.run(stop=10, dt=0.01)
