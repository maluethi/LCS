#!/usr/bin/env python

from FeedthroughMotorControls import *
import os
import sys
import subprocess

initFeedtrough()
initAxis1()

NIterations = int(sys.argv[1])

#Acceleration = ReadParameter(ROTARYAXIS, "A")
#Decceleration = ReadParameter(ROTARYAXIS, "D")
#MaxSpeed = ReadParameter(ROTARYAXIS, "VM")
#MinSpeed = ReadParameter(ROTARYAXIS, "VI")

# Do this for the reference run of the rotary encoder:
Move = 200000
moveHorizontal(Move)
time.sleep(CalcMovementTimeDefault(Move))

Move = -150000
moveHorizontal(Move)
time.sleep(CalcMovementTimeDefault(Move))

HomeAxis(1)
time.sleep(0.5)

Move = 200000
moveHorizontal(Move)
time.sleep(CalcMovementTimeDefault(Move))

Move = -100000
moveHorizontal(Move)
time.sleep(CalcMovementTimeDefault(Move))

for i in range(NIterations):
	Move = 100000
	if i%2 == 0:
		moveHorizontal(Move)
		time.sleep(CalcMovementTimeDefault(Move)+0.5)
	elif i%2 == 1:
		moveHorizontal(-Move)
		time.sleep(CalcMovementTimeDefault(Move)+0.5)

HomeAxis(1)
print "Test finished"
