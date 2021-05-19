import smoldyn
import smoldyn._smoldyn as S

s = smoldyn.Simulation(low=[0, 0], high=[100, 100])
red = s.addSpecies("red", difc=3, color="red")
s.addMolecules(red, 100)  # or, red.addToSolution(100)
#s.addCommand("molcount stdout", "N", step=10)
s.addCommand("molcount stdout", "E")

S.Simulation.setSimTimes(s, 0, 100, 0.1)

#s.setGraphics("opengl", 10)
# Simple run works:
# s.run(stop=100, dt=0.1, start=0, display=True, quit_at_end=True)

# runTimeStep doesn't work:
for it in range(10):
    # S.Simulation.runTimeStep(s)
    # Alternate call to runTimeStep, same result:
    s.runTimeStep()
