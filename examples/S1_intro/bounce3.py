__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn as S

S.setBounds(low=(0,0,0), high=(100,100,100), types=['r','r','r'])
red = S.Species('red', difc=3)
red.addToSolution(100)
red.addToSolution(30, highpos=[20,30,20])

green = S.Species('green', difc=1)
green.addToSolution(30)

s = S.Simulation(100, 0.01, quitatend=True)
s.setGraphics('opengl', 10)
s.run()
