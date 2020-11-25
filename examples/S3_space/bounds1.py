# Demonstration of absorbing, periodic, and reflective boundaries
import smoldyn as sm

# dim 2
# low_wall x 0 a
# high_wall x 100 r
# boundaries y 0 100 p
sm.setBounds(low=[0, 0], high=[100, 100], types=["ar", "p"])

# species red green
# difc red 3
# difc green 3
# color red red
# color green green
red = sm.Species("red", color="red", difc=3)
green = sm.Species("green", color="green", difc=3)

# mol 300 red u u
# mol 30 green 50 5
red.addToSolution(300)
green.addToSolution(30, pos=[50, 5])

# time_start 0
# time_stop 200
# time_step 0.01
#
# end_file
s = sm.Simulation(200, 0.01)
s.setGraphics("opengl")
s.run()
