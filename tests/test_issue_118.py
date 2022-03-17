import smoldyn

s = smoldyn.Simulation(
    low=[-1000, -1000, -10000], high=[1000, 1000, 10000], output_files=["cylinder.txt"]
)

d1 = s.addDisk(name="d1", center=[50, 25, 8500], radius=850, slices=12, vector=[0, 0, -1])
test = s.addSurface("test", panels=[d1])
