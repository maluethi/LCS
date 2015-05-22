__author__ = 'matthias'

from feedtrough import *

LinearMotor = Feedtrough("linear actuator", 1)
RotaryMotor = Feedtrough("rotary actuator", 1)

#LinearMotor.comport = "/dev/pts/26"
LinearMotor.com_init()

RotaryMotor.com = LinearMotor.com



RotaryMotor.com_write("hello")
LinearMotor.com_write("bla")
LinearMotor.getInfo(True)
RotaryMotor.setParameter("S3", '1,0,0')

LinearMotor.com_close()
