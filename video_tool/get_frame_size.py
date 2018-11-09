#!/usr/bin/env python
import csv

filename = 'r500.csv'
data_out="out500.txt"
fileObject = open(data_out, 'w')
delta=33
c=0;
with open(filename) as f:
    reader = csv.reader(f)
    for row in reader:
	if c==0:
		c=c+1
		continue
	c=c+1
	row_n=len(row)
	packet_len=float(row[row_n-1])
	inst_rate=packet_len*8/delta
        fileObject.write(row[0])
        fileObject.write("\t")
        fileObject.write(row[row_n-1])
        fileObject.write("\t")
	fileObject.write(str(inst_rate))
        fileObject.write("\t")
        fileObject.write("\n")
