#!/bin/python

import time
import sys
import serial

f = open(sys.argv[1], "r")
ser = serial.Serial(sys.argv[2], 115200, timeout=0)

sleep_count = 0
for x in f:
	s = ser.read(100)
	if s:
		print(s.decode(), end='')
	sleep_count = sleep_count + 1
	ser.write(x.encode())
	if sleep_count > 2:
		time.sleep(1)
		sleep_count = 0
ser.close()
f.close()