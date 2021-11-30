#! /bin/sh
output=gcc

algo=gcc
./waf --run "gcc_tcp " 2>${algo}.log || exit

file1=${algo}.log
tcp1_file=traces/gcc_tcp1_bw.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "rate/kbps"
set xrange [0:60]
set yrange [0:250]
set term "svg"
set output "${output}_bw.svg"
plot "${file1}" u 1:(\$2) title "rate" with lines lw 2, \
  "${tcp1_file}" u 1:(\$2/1000) title "tcp flow1" with lines lw 1
set output
exit
!

gnuplot<<!
set xlabel "time/s" 
set ylabel "loss/percent"
set xrange [0:60]
set yrange [0:50]
set term "svg"
set output "${output}_loss.svg"
plot "${file1}" u 1:(\$4*100) title "loss" with lines lw 2
set output
exit
!
