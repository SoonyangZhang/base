#! /bin/sh

file1=record.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "rate/kbps"
set xrange [0:100000]
set yrange [0:2500]
set term "png"
set output "bbrpadding1.png"
plot "${file1}" u 1:2 title "bbr" with lines
set output
exit
!
