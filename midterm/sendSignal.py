import numpy as np
import serial
import time

waitTime = 0.1

song1 = np.array(
  [261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261])
length1 = np.array(
  [1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2]
)

song2 = np.array(
  [392, 330, 330, 349, 294, 294,
  262, 294, 330, 349, 392, 392, 392,
  392, 330, 330, 349, 294, 294,
  261, 330, 392, 392, 330,
  294, 294, 294, 294, 294, 330, 349,
  330, 330, 330, 330, 330, 349, 392,
  392, 330, 330, 349, 294, 294,
  261, 330, 392, 392, 261])
length2 = np.array(
  [1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 3,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 3 ]
) 

song3 = np.array(
  [261, 293, 330, 261, 261, 293, 330, 261, 
  330, 349, 392, 329, 349, 392,
  392, 440, 392, 349, 330, 261,
  392, 440, 392, 349, 330, 261,
  293, 196, 261, 293, 196, 261])
length3 = np.array(
  [1, 1, 1, 1, 1, 1, 1, 1
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1]
)

# generate the waveform table
signalLength = 1024
t = np.linspace(0, 2*np.pi, signalLength)
signalTable = (np.sin(t) + 1.0) / 2.0 * ((1<<16) - 1)

# output formatter
formatter = lambda x: "%.3f" % x

# send the waveform table to K66F
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
print("get song signal ...")
line = s.readline()
index = int(line)
print("index %d" % (int(index)))
print("It may take about %d seconds ..." % (int(signalLength * waitTime)))

if index == 0:
  for data in song0:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
  for data in length0:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
if index == 1:
  for data in song1:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
  for data in length1:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
if index == 2:
  for data in song2:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
  for data in length2:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
s.close()
print("Signal sended")
