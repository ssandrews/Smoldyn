__author__ = "Dilawar Singh"
__email__ = "dilawar@subcom.tech"

import smoldyn as S
import numpy as np

def test_prd_pacement():
    sim = S.Simulation(low=[-10,-10,-10],high=[10,10,10], quit_at_end=1)
    sim.setRandomSeed(10)
    blue = sim.addSpecies(name='blue',color='blue',difc=1,display_size=0.3)
    red = sim.addSpecies(name='red',color='red',difc=0,display_size=0.3)
    sim.addMolecules(blue,number=100,lowpos=[-5,-5,-5],highpos=[5,5,5])
    sim.addMolecules(red,number=1,pos=[0,0,0])

    s1 = sim.addSurface('membrane',panels=[sim.addSphere(center=[0,0,0],radius=10,slices=10,stacks=10)])
    s1.setStyle('both',drawmode='edge',color='green')
    s1.setAction(face='both', species=[blue, red], action='reflect')

    stick = sim.addReaction(name='stick',subs=[blue,red],prds=[red,red],rate=20)
    stick.productPlacement(method='bounce', param=0.6)
    sim.addOutputData("data")
    sim.addCommand("molcount data", cmd_type="E")
    # sim.setGraphics('opengl_good', 1)
    sim.run(stop=100, dt=0.1)
    d = sim.getOutputData("data")
    d = np.array(d)
    assert (d[:,1] + d[:, 2] == 101).all()

    y = d.std(axis=0)
    ym = d.mean(axis=0)
    assert np.allclose(ym, [50, 20.75524, 80.24475], atol=1e-3, rtol=1e-3), ym
    assert np.allclose(y, [28.89636655, 20.5, 20.5], atol=1e-1, rtol=1e-1), y
    print(d)

def main():
    test_prd_pacement()

if __name__ == '__main__':
    main()
