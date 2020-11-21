# Creates random lines
import os
import random

number=input('Enter number of lines: ')
xlow=float(input('Enter x low: '))
xhigh=float(input('Enter x high: '))
ylow=float(input('Enter y low: '))
yhigh=float(input('Enter y high: '))

print('start_surface lines')
for i in range(1,number):
	x0=random.uniform(xlow,xhigh)
	x1=random.uniform(xlow,xhigh)
	y0=random.uniform(ylow,yhigh)
	y1=random.uniform(ylow,yhigh)
	print('panel tri %f %f %f %f' %(x0,y0,x1,y1))
print('end_surface')
