__author__ = "Dilawar Singh"
__email__ = "dilawar@subcom.tech"

from pathlib import Path
import numpy as np

import smoldyn

sdir = Path(__file__).parent


def test_multiple_iteration():
    modelfile = sdir / "min.txt"
    s = smoldyn.Simulation.fromFile(modelfile)

    assert s
    assert s.getSimPtr() and s.getSimPtr() == s.simptr

    print(s.start, s.stop, s.dt)
    assert s.start == 0.0
    assert s.stop == 500
    assert s.dt == 0.002
    s.addOutputData("moments")
    s.addCommand("molmoments MinD_ATP(front) moments", "N", step=2)
    for i in range(10):
        print(f"\n\n=============================\nRunning iteration {i}")
        s.run(stop=(i + 1) * 10, dt=1, start=(i * 10))
        data = s.getOutputData("moments")
        for l in data:
            print(l)
        assert data, "No data returned."
        assert len(data) == 6, len(data)  
        assert not np.ma.allequal(data[1], data[-1]), (data[1], data[-1])


if __name__ == "__main__":
    test_multiple_iteration()
