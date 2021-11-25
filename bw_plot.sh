#! /bin/sh
algo=ana
file1=traces/${algo}_rate.txt
output=${algo}
gnuplot<<!
set xlabel "time/s" 
set ylabel "rate/kbps"
set xrange [0:100]
set yrange [0:80]
set term "svg"
set output "${output}_bw.svg"
plot "${file1}" u 1:(\$2/1000) title "send rate" with lines lw 2, 40 title "bottleneck" dashtype 2
set output
exit
!

file2=traces/${algo}_delay.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "delay/ms"
set xrange [0:100]
set yrange [0:600]
set term "svg"
set output "${output}_delay.svg"
plot "${file2}" u 1:2 title "recv owd" with lines lw 2, 50 title "link delay" dashtype 2
set output
exit
!