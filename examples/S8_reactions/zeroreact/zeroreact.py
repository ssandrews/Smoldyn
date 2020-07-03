# Zeroth order reactions
import smoldyn as sm

sm.setBounds(low=[0,0,0], high=(10,10,10), types='r')
red = sm.Species('red', difc=1, color=[1,0,0])
green = sm.Species('green', difc=1, color=[0,1,0])
blue = sm.Species('blue', difc=1, color=[0,0,1])
slow = sm.Reaction('slow', [], [red], kf=0.001)
med = sm.Reaction('med', [], [green], kf=0.01)
fast = sm.Reaction('fast', [], [blue], kf=0.1)

s = sm.Simulation(stop=10, step=0.1, output_files=['zeroreactout.txt'])
s.setGraphics('opengl')
s.addCommand('e', cmd='molcount zeroreactout.txt')
s.run()
