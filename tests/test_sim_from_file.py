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
    assert s1.simptr
    print(s1)
    print(s1.getSimPtr())
    modelfile = sdir / ".." / "examples" / "S99_more" / "Min" / "Min1.txt"

    #  s2 = S.Simulation(str(modelfile), "")  # type: _smoldyn.Simulation
    # or, recommended.
    s2 = smoldyn.Simulation.fromFile(modelfile, "")  # type: Simulation
    assert s2
    assert s2.getSimPtr()
    #  assert s2.simptr

    print(s2)
    print(s2.getSimPtr())
    #  print(s2.simptr)


if __name__ == "__main__":
    test_simptr_simobj()
