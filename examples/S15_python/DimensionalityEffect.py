import smoldyn
import smoldyn._smoldyn as S
import numpy
import matplotlib.pyplot as plt

s = smoldyn.Simulation(low=[-10,-10,-10], high=[10,10,10])
target = s.addSpecies("target", color=dict(all="red"), display_size={"all":10})
protein = s.addSpecies("protein", color="black", display_size=3, difc=dict(all=1))
counter = s.addSpecies("counter", color="white", display_size=0)

sph1 = smoldyn.Sphere(center=[0,0,0], radius=10, slices=20, stacks=20)
membrane = s.addSurface(name="membrane", panels=[sph1])
membrane.setStyle("both", color="blue", drawmode="edge")
#membrane.setRate(protein, "bsoln", "back", rate=1, revrate=0.1)


r1 = s.addReaction("r1", subs=[(target,"up"),(protein,"bsoln")], prds=[(target,"up"), counter], rate=6)
r2 = s.addReaction("r2", subs=[(target,"up"),(protein,"back")], prds=[(target,"up"), counter], rate=12)

#s.setGraphics("opengl", frame_thickness=0, text_display=["time",protein,(protein,"back"), counter])
s.addOutputData("output")
s.addCommand("ifmore counter 500 stop", "E")
s.addCommand("molcount output", "A")
s.addCommand("killmol all(all)", "A")

MS = S.MolecState

adsorbs = []
bindtimes = []
binddevs = []

for adsorb in numpy.arange(0,0.101,0.01):
	S.Simulation.setSurfaceRate(s, "membrane" ,"protein", MS.soln, MS.bsoln, MS.back, adsorb, "protein", False)
	timelist = []
	for it in range(0,10):
		membrane.addMolecules((target,"up"), N=1, pos=[10,0,0])
		s.addMolecules(protein, 1000, [0,0,0])
		s.run(start=0, stop=1000, dt=0.01)
		results = s.getOutputData("output",1)
		result = results[0][0]
		timelist = numpy.append(timelist, result)
		print("*** On-rate=",adsorb,"Median binding time=",result)
	adsorbs = numpy.append(adsorbs, adsorb)
	bindtimes = numpy.append(bindtimes, numpy.mean(timelist))
	binddevs = numpy.append(binddevs, numpy.std(timelist))

plt.errorbar(adsorbs, bindtimes, yerr=binddevs, fmt='-o')
plt.xlabel("Adsorption rate")
plt.ylabel("Median binding time")
plt.show()

