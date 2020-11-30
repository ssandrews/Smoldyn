"""Test the Simulation class.
"""

__author__ = "Dilawar Singh"
__email__ = "dilawar@subcom.tech"

import smoldyn as S

def test_refactor():
    sim = S.Simulation(low=[-10,-10,-10],high=[10,10,10], quit_at_end=1)
    blue = sim.addSpecies(name='blue',color='blue',difc=1,display_size=0.3)
    red = sim.addSpecies(name='red',color='red',difc=0,display_size=0.3)
    sim.addMolecules(blue,number=100,lowpos=[-5,-5,-5],highpos=[5,5,5])
    sim.addMolecules(red,number=1,pos=[0,0,0])

    s1 = sim.addSurface('membrane',panels=[S.Sphere(center=[0,0,0],radius=10,slices=10,stacks=10)])
    s1.setStyle('both',drawmode='edge',color='green')
    s1.setAction(face='both', species=[blue, red], action='reflect')

    stick = sim.addReaction(name='stick',subs=[blue,red],prds=[red,red],rate=20)
    stick.productPlacement(method='bounce', param=0.6)
    sim.setTime(dt=0.1)
    sim.setGraphics('opengl_good',1)
    sim.run(stop=100)

def main():
    test_refactor()

if __name__ == '__main__':
    main()
