import smoldyn
import smoldyn._smoldyn as S
import numpy

s = smoldyn.Simulation(low=[-10,-10,-10], high=[10,10,10])
target = s.addSpecies("target", color=dict(all="red"), display_size={"all":10})
protein = s.addSpecies("protein", color="black", display_size=3, difc=dict(all=1, back=0.1))
counter = s.addSpecies("counter", color="white", display_size=0)

sph1 = smoldyn.Sphere(center=[0,0,0], radius=10, slices=20, stacks=20)
membrane = s.addSurface(name="membrane", panels=[sph1])
membrane.setStyle("both", color="blue", drawmode="edge")
membrane.setRate(protein, "bsoln", "back", rate=1, revrate=0.1)

membrane.addMolecules((target,"up"), N=10)
s.addMolecules(protein, 1000, [0,0,0])

r1 = s.addReaction("r1", subs=[(target,"up"),(protein,"bsoln")], prds=[(target,"up"), counter], rate=10)
r2 = s.addReaction("r2", subs=[(target,"up"),(protein,"back")], prds=[(target,"up"), counter], rate=10)

#s.setGraphics("opengl", frame_thickness=0, text_display=["time",protein,(protein,"back"), counter])
s.addOutputData("output")
s.addCommand("ifmore counter 500 stop", "E")
s.addCommand("molcount output", "A")


#s.run(100,0.01)


value = 1
sigma = value/5
best = 1000
oldvalue = value
for it in range(10):
	value += sigma * (numpy.random.randint(2)-0.5)
        # also see smoldyn.Simulation.setRate function.
	S.Simulation.setSurfaceRate(s, "membrane" ,"protein", S.MolecState.soln,S.MolecState.bsoln, S.MolecState.back, value, "protein", False)
	s.run(stop=100,dt=0.01)
	data=s.getOutputData("output",0)
	test=data[0][0]
	if test < best:
		print("Iteration=",it,"Attempt=",value,"Result=",test,"Success! Best=",best)
		oldvalue = value
		best = test
		sigma *= 1.1
	else:
		print("Iteration=",it,"Attempt=",value,"Result=",test,"Worse. Best=",best)
		value = oldvalue
		sigma *= 0.9


