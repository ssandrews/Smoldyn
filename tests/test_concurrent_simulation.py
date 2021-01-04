__author__ = "Dilawar Singh"
__email__ = "dilawar@subcom.tech"

"""
FIXME: Two Simulations could not be run in the serial mode.
"""

import smoldyn
import time
import threading


def sim1():
    s = smoldyn.Simulation(low=[0, 0], high=[100, 100])
    red = s.addSpecies("red", color="red", difc=dict(all=3, front=0), display_size=5)
    yellow = s.addSpecies(
        "yellow", color="black", difc=dict(soln=3, back=1), display_size=5
    )
    blue = s.addSpecies("blue", color="blue", difc=3, display_size=5)

    red.addToSolution(100)
    yellow.addToSolution(50, pos=(50, 50))
    blue.addToSolution(50, pos=(20, 20))

    # Construct a closed path in 2D.
    p = s.addPath2D((0, 0), (100, 0), (100, 100), (0, 100), closed=True)
    walls = s.addSurface("walls", panels=p.panels)
    walls.setAction("both", [red, yellow, blue], "reflect")
    walls.setStyle("both", color="black")

    sph = s.addSphere(center=(50, 50), radius=20, slices=20)
    surf = s.addSurface("stick", panels=[sph])
    surf.setRate(red, "fsoln", "front", rate=10, revrate=1)
    surf.setRate(yellow, "bsoln", "back", rate=10, revrate=1)
    surf.setRate(blue, "fsoln", "bsoln", rate=10, revrate=1)
    surf.setStyle("front", color=(1, 0.7, 0))
    surf.setStyle("back", color=(0.6, 0, 0.6))
    surf.addMolecules((red, "front"), 100)

    # FIXME: Concurrent simulation doesn't work when graphics is enabled.
    # s.setGraphics("opengl")
    s.addCommand("killmolinsphere red all", "b")
    s.addOutputData("data1")
    s.addCommand(cmd="molcount data1", cmd_type='E')
    return s


def sim2():
    sim = smoldyn.Simulation(low=[0, 0, 0], high=[100, 100, 100])

    # Set it after Simulation object is created.
    sim.seed = 0

    spRed = sim.addSpecies("red", color="red", difc=3, display_size=3)
    spRed.addToSolution(20, highpos=[10, 50, 50])

    spGreen = sim.addSpecies("green", color="green", difc=1, display_size=3)
    sim.addMolecules(spGreen, 20, highpos=[10, 50, 50])

    # Add Surfaces
    r1 = sim.addRectangle(corner=[100, 0, 0], dimensions=[100, 100], axis="-x")
    r2 = sim.addRectangle(corner=[0, 0, 0], dimensions=[100, 100], axis="+y")
    r3 = sim.addRectangle(corner=[0, 100, 0], dimensions=[100, 100], axis="-y")
    r4 = sim.addRectangle(corner=[0, 0, 0], dimensions=[100, 100], axis="+z")
    r5 = sim.addRectangle(corner=[0, 0, 100], dimensions=[100, 100], axis="-z")

    s1 = sim.addSurface("walls", panels=[r1, r2, r3, r4, r5])
    # s1.both.setStyle(drawmode='edge')
    s1.setStyle("both", drawmode="edge")
    s1.setAction("both", [spRed, spGreen], "reflect")

    # portsurf
    rr = sim.addRectangle(corner=[0, 0, 0], dimensions=[100, 100], axis="+x")
    portSurf = sim.addSurface("portsurf", panels=[rr])
    portSurf.setStyle("front", drawmode="face", color="gray")
    portSurf.setStyle("back", drawmode="face", color=[0.2, 0, 0, 1])
    portSurf.setAction("front", [spRed, spGreen], "port")
    portSurf.setAction("back", [spRed, spGreen], "reflect")

    # Ports
    sim.addPort(name="testport", surface=portSurf, panel="front")

    #  sim.setGraphics("opengl", 20)
    sim.addOutputData("data2")
    sim.addCommand(cmd="molcount data2", cmd_type='E')
    return sim

def main():
    s1, s2 = sim1(), sim2()

    t1 = threading.Thread(target=lambda t: s1.run(t, dt=0.1, overwrite=True), args=(100,))
    t1.start()

    t2 = threading.Thread(target=lambda t: s2.run(t, dt=0.1, overwrite=True), args=(100,))
    t2.start()

    t1.join()
    t2.join()

    data1 = s1.getOutputData('data1')
    data2 = s2.getOutputData('data2')
    assert len(data1) == len(data2) == 1001
    print(data1)
    print(data2)


if __name__ == "__main__":
    main()
