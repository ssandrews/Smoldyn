# Python version of cmddata.txt
# Author: Steve
# Modified by: Dilawar

import smoldyn
import math


def is_close(listA, listB):
    for a, b in zip(listA, listB):
        if not math.isclose(a, b, rel_tol=1e-6, abs_tol=1e-6):
            return False
    return True

def test_data():
    s = smoldyn.Simulation(low=[0, 0, 0], high=[100, 100, 100])
    s.seed = 100

    R = s.addSpecies("R", difc=1, color="red")
    B = s.addSpecies("B", difc=1, color="blue")

    s.addReaction("r1", subs=[R], prds=[], rate=0.1 )

    R.addToSolution(400)
    B.addToSolution(1, pos=[50,50,50])
    s.addOutputData('mydata')
    s.addCommand(cmd="molcount mydata", cmd_type="E")
    # s.setGraphics( "opengl" )
    s.run(stop=10, dt=0.01)
    data = s.getOutputData('mydata', 0)
    assert len(data) == 1001, len(data)
    assert len(data[0]) == 3, len(data[0])
    assert data[0] == [0.0, 400.0, 1.0], data[0]
    assert is_close(data[-1], [10.0, 142.0, 1.0]), data[-1]
    for row in data:
        print(row)

def main():
    test_data()

if __name__ == '__main__':
    main()
