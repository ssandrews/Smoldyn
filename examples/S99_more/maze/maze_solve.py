#
# Solve a maze using brute force
#

__author__ = "Dilawar Singh"
__email__ = "dilawar.s.rajput@gmail.com"

import math
import smoldyn

from pathlib import Path
import typing as T


def add_maze(s, mazefile) -> T.List[smoldyn.Panel]:
    """Add maze"""
    panels = []
    offset, scale = 1.5, 10
    with open(mazefile, "r") as f:
        for i, ll in enumerate(f.read().strip().split("\n")):
            l = [scale * (offset + float(x)) for x in ll.split()]
            panels += s.addPath2D([(l[0], l[1]), (l[2], l[3])]).panels

    # close the entry notch
    r = s.addRectangle(corner=[10, 10], dimensions=[30], axis="+y", name="r")
    panels.append(r)
    return panels


s = smoldyn.Simulation(low=[0, 0], high=[120, 120])

A = s.addSpecies("A", difc=2, color="blue")
A.addToSolution(1000, pos=[15, 15])

B = s.addSpecies("B", difc=0, color="black", display_size=3)
B.addToSolution(10, lowpos=[105, 112], highpos=[110, 112])

panels = add_maze(s, Path(__file__).parent / "maze.txt")
maze = s.addSurface("maze", panels=panels)
maze.setAction("front", [A], "reflect")
maze.setAction("back", [A], "reflect")
maze.setStyle("both", thickness=1, color="red")

# When any A meets B, maze is solved. So we set a reaction between A and B
s.addReaction("done", [A, B], [], rate=10)

# Stop when A + B -> ∅ happens i.e. maze is solved.
s.addOutputData('posdata')
s.addCommand("molpos B posdata", "E")
s.addCommand("ifless B 10 stop", "E")

s.addGraphics("opengl", iter=50)
s.run(3000, dt=0.01, quit_at_end=True)

print(s.getOutputData("posdata"))
print('all done')
