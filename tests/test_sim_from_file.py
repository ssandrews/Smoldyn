__author__ = "Dilawar Singh"
__email__ = "dilawar@subcom.tech"

from pathlib import Path

import smoldyn
import smoldyn._smoldyn as S

sdir = Path(__file__).parent


def test_simptr_simobj():
    s1 = smoldyn.Simulation([0, 0], [10, 10])
    assert s1
    assert s1.getSimPtr()
    assert s1.simptr == s1.getSimPtr()
    assert s1.simptr.dt == 0.0, "dt should be initialized to 0"

    modelfile = sdir / "min.txt"
    s2 = smoldyn.Simulation.fromFile(modelfile, "")  # type: Simulation

    assert s2
    assert s2.getSimPtr() and s2.getSimPtr() == s2.simptr

    assert s2.start == 0.0
    assert s2.stop == 500
    assert s2.dt == 0.002
    s2.addOutputData('moments')
    s2.addCommand("molmoments MinD_ATP(front) moments", "N", step=10)
    s2.run(stop=100, dt=1)
    data = s2.getOutputData("moments")
    assert len(data) == 11, len(data)
    for row in data:
        print(row)



if __name__ == "__main__":
    test_simptr_simobj()
