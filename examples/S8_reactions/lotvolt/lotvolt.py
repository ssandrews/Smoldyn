# Simulation file for Lotka-Voltera reaction
import smoldyn as sm

sm.setBounds([-100, -100, -10], [100, 100, 10], types="p")
rabbit = sm.Species("rabbit", color="red", display_size=2, mol_list="rlist", difc=100)
fox = sm.Species("fox", color="blue", display_size=3, mol_list="flist", difc=100)
sm.setPartitions("molperbox", 1)
r1 = sm.Reaction("r1", subs=[rabbit], prds=[rabbit, rabbit], kf=10)
r2 = sm.Reaction("r2", [rabbit, fox], [fox, fox], kf=8000)
r3 = sm.Reaction("r3", [fox], [], kf=10)

rabbit.addToSolution(1000)
fox.addToSolution(1000)

s = sm.Simulation(20, step=0.001, output_files="lotvoltout.txt")
s.setTiff("lotvolt/OpenGl")
s.setGraphics("opengl", iter=5, text_display=["time", "rabbit", "fox"])
s.addCommand("molcount lotvoltout.txt", "i", on=0, off=5, step=0.01)
s.addCommand("molcount stdout", "i", on=0, off=2, step=0.1)

s.run()
