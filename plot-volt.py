#!/usr/bin/env python
import sys
import linecache
import matplotlib.pyplot as plt
import numpy as np

def readCurVol(fileName):
	curList = []
	volList = []
	allLines = linecache.getlines(fileName)
	for line in allLines:
		if line[0] == 'H' or line[0] == 'L':
			# 'H 0.000000 1.281206'
			conv,cur,vol = line.strip().split()
			curList.append(float(cur))
			volList.append(float(vol))

	return (curList, volList)

def getX(y):
	lenx = len(y)
	x = np.linspace(0, lenx, lenx)
	return x

def shift_cur(y):
	fy = np.array(y)
	return fy+1.23

def adcFilter(y, win):
	fy = []
	length = len(y)
	pos = 0;
	window=win
	while pos < length - window:
		fy.append( np.sum(y[pos:pos+window])/window )
		pos += 1
	while pos < length:
		fy.append( np.sum(y[pos:])/len(y[pos:]) )
		pos += 1
	return fy

def drawPlot(fileName):
	plt.figure(figsize=(8,4))
	cur,vol = readCurVol(fileName)
	x = getX(cur)

	#vol = adcFilter(vol)
	#cur = shift_cur(cur)
	plt.plot(x,vol,label="voltage",color="black",linewidth=1)
	#plt.plot(x,cur, 'bo-')
	#plt.plot(x,vol, 'ro-')

	plt.xlabel("time/(1/80ms)")
	plt.ylabel("volt/(v)")
	plt.title("adc plot")
	#plt.ylim(0, np.max(vol))
	plt.ylim(np.min(vol), np.max(vol))
	print np.max(vol) - np.min(vol)
	plt.legend()
	plt.show()

drawPlot(sys.argv[1])

