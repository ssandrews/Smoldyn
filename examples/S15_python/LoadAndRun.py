import smoldyn
import smoldyn._smoldyn as S
from pathlib import Path

scriptdir = Path(__file__).parent

print("Smoldyn version = ",smoldyn.version())

EC=S.ErrorCode
print("Test of errorCodeToString: ",S.errorCodeToString(EC.ok))

sim1 = smoldyn.Simulation(low=[0,0], high=[100,100])
red = sim1.addSpecies("red",difc=1,color="red")
sim1.addMolecules(red,100)
print("========= Here is a nearly empty simulation, from variable sim1: ==========")
sim1.displaySim()
print("==================================")

sim1.updateSim()
print("========= Here is the updated sim1: ==========")
sim1.displaySim()
print("==================================")

minpath = scriptdir / 'Min1.txt'
print("======== Loading Min1.txt into sim2 =======")
print("Path to Min1.txt file: ",minpath)

sim2 = smoldyn.Simulation.fromFile(minpath,"")

print("Running sim2")
sim2.run(stop=10,dt=0.005)

print("==== Now using runUntil ====")
sim2.runUntil(20,0.005)
