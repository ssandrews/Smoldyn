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
__email__  = "dilawars@ncbs.res.in"

import smoldyn as sm

# Model parameters
K_FWD=0.001		        # substrate-enzyme association reaction rate
K_BACK=1			# complex dissociation reaction rate
K_PROD=1			# complex reaction rate to product

# System space and time definitions. It is a 2D system bound between x=-1 to
# x=1 and y=-1 to y=1.
sm.setBounds(low=[-1, -1], high=[1, 1])

# Molecular species and their properties
# Species: S=substrate, E=enzyme, ES=complex, P=product
# Diffusion coefficient is given by `difc` (in default units)
# `color` and `display_size` of the species is used in plotting.
#
# There are few other parameters. Type `help(smoldyn.Species)` in Python
# console to know about them.
S = sm.Species('S', difc=3, color='green', display_size=0.02)
E = sm.Species('E', color='darkred', display_size=0.03)
P = sm.Species('P', difc=3, color='darkblue', display_size=0.02)
ES = sm.Species('ES', color='orange', display_size=0.03)

# Surfaces in the system and their properties.
# Declare a sphere
s1 = sm.Sphere(center=(0,0), radius=1, slices=50)
# Surface requires at least one panel (Sphere in this case).
membrane = sm.Surface('membrane', panels=[s1])

# Add action to `both` faces for surface. You can also use `front` or `back`
# as well. `all` molecules reflect at both surface faces
membrane.both.addAction('all', 'reflect')
membrane.both.setStyle(color='black', thickness=1)
inside = sm.Compartment(name='inside', surface=membrane, point=[0, 0])

# Chemical reactions
# Association and dissociation reactions.
rep = sm.Reaction([E, S], [ES], kf=K_FWD, kb=K_BACK)

# product_placement back pgemmax 0.2			# for reversible reactions

# product formation reaction
prod =  sm.Reaction([ES], [E, P], kf=K_PROD)

# Place molecules for initial condition
inside.addMolecules(S, 500)
# Or,
# S.addToCompartment(500, inside)

# surface_mol 100 E(front) membrane all all	# puts 100 E molecules on surface
membrane.addMolecules(E, 100)
# or,
# E.addToSurface(100, membrane)

# # Output and other run-time commands
# text_display time S E(front) ES(front) P	# displays species counts to graphics
# ifdefine TEXTOUTPUT				# only run this if needed
#   output_files templateout.txt			# file names for text output
#   cmd B molcountheader templateout.txt		# text output run at beginning
#   cmd N 10 molcount templateout.txt		# text output run every 10 time steps
# endif

s = sm.Simulation(stop=10, step=0.01)
s.setGraphics('opengl_good', bg_color='white', frame_thickness=2)
s.run()
