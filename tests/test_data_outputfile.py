# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29
# Python implementation of box.txt

import smoldyn as sm
import numpy as np
import os.path

def test_data_output():
    s = sm.Simulation(low=[0, 0, 0], high=[100, 100, 100], boundary_type="ppp")

    s.addBox(size=10)

    # declaration of species A, B, and C with attributes.
    a = s.addSpecies("A", state="soln", difc=1, color="red", mol_list="Alist")
    b = s.addSpecies("B", state="soln", color="green", difc=1, mol_list="Blist")
    c = s.addSpecies("C", state="soln", difc=1.0, color="blue", mol_list="Clist")
    s.addMolecules(a, 1000)
    s.addMolecules(b, 1000)

    s.addBidirectionalReaction("r1", subs=[c], prds=(a, b), kf=0.1, kb=100)

    # FIXME: Prints only upto 10 (2 iterations rather than 100)
    s.setOutputFile("box.dat")
    # s.setGraphics("opengl")
    c = s.addCommand("molcount box.dat", cmd_type="i", on=0, off=100, step=10)
    s.run(100, dt=0.1, overwrite=True)
    print("Simulation over")

    # Now read the box.dat and verify the results.
    # On OSX, it may not work. Homebrew python is a shell script which chdir to
    # script dir before running it. I.e., box.dat would not be found easily.
    if not os.path.exists('box.dat'):
        quit(0)
    data = np.loadtxt("box.dat")
    assert data.shape == (11, 4), data.shape
    expected = [50.036, 590.727, 590.727, 409.272]
    print(data.mean(axis=0), expected)
    assert np.isclose(
        data.mean(axis=0), expected, atol=1e-1, rtol=1e-1
    ).all(), data.mean(axis=0)
    quit(0)


def main():
    test_data_output()


if __name__ == "__main__":
    main()
