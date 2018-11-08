#!/usr/bin/env python
import csv

filename = 'ssim.csv'
data_out="out.txt"
fileObject = open(data_out, 'w')
c=0;
with open(filename) as f:
    reader = csv.reader(f)
    for row in reader:
        fileObject.write(str(c))
	c=c+1
        fileObject.write("\t")     
        for i in range(len(row)):

            fileObject.write(row[i])
            fileObject.write("\t")
        fileObject.write("\n")
