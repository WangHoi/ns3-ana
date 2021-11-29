#! /bin/sh
algo=ana
kbps=90
./waf --run "ana-example --kbps=${kbps}" 2>a.log || exit

file1=traces/${algo}_rate.txt
file2=traces/${algo}_rateAck.txt
file3=traces/${algo}_rateRecv.txt
tcp1_file=traces/tcp1_bw.txt
tcp2_file=traces/tcp2_bw.txt
output=${algo}
gnuplot<<!
set xlabel "time/s" 
set ylabel "rate/kbps"
set xrange [0:60]
set yrange [0:100]
set term "svg"
set output "${output}_bw.svg"
plot "${file1}" u 1:(\$2/1000) title "send rate" with lines lw 2, \
 "${file3}" u 1:(\$2/1000) title "recv rate" with lines lw 2, \
 "${file2}" u 1:(\$2/1000) title "ack rate" with lines lw 1, \
 "${tcp1_file}" u 1:(\$2/1000) title "tcp flow1" with lines lw 1, \
 ${kbps} title "bottleneck" dashtype 2
set output
exit
!

file2=traces/${algo}_delay.txt
rtt_file=traces/${algo}_rtt.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "delay/ms"
set xrange [0:60]
set yrange [0:600]
set term "svg"
set output "${output}_delay.svg"
plot "${file2}" u 1:2 title "recv owd" with lines lw 2, \
 "${rtt_file}" u 1:2 title "rtt" with lines lw 2
set output
exit
!
