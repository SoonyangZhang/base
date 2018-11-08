#! /bin/sh
  
file1=out.txt
gnuplot<<!
set xlabel "index" 
set ylabel "ssim"
set xrange [0:1000]
set yrange [0.8:1]
set term "png"
set output "qoe.png"
plot "${file1}" u 1:2 title "100kbps" with lines,\
"${file1}" u 1:3 title "200kbps" with lines
set output
exit
!
