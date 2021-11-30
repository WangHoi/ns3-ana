#! /bin/sh
kbps=${1:-500}
output=ana_tcp_$kbps

algo=ana
msDelay=50
msQdelay=300
tcp=true
./waf --run "ana-example --tcp=${tcp} --kbps=${kbps} --msDelay=${msDelay} --msQdelay=${msQdelay}" 2>a.log || exit

file1=traces/${algo}_rate.txt
file2=traces/${algo}_rateAck.txt
file3=traces/${algo}_rateRecv.txt
tcp1_file=traces/tcp1_bw.txt
tcp2_file=traces/tcp2_bw.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "rate/kbps"
set xrange [0:60]
set yrange [0:${kbps}]
set term "svg"
set output "${output}_bw.svg"
plot "${file1}" u 1:(\$2/1000) title "send rate" with lines lw 2, \
 "${file3}" u 1:(\$2/1000) title "recv rate" with lines lw 2, \
 "${file2}" u 1:(\$2/1000) title "ack rate" with lines lw 1, \
 "${tcp1_file}" u 1:(\$2/1000) title "tcp flow1" with lines lw 1
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

loss_file=traces/${algo}_loss.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "loss/percent"
set xrange [0:60]
set yrange [0:50]
set term "svg"
set output "${output}_loss.svg"
plot "${loss_file}" u 1:(\$2*100) title "loss" with lines lw 2
set output
exit
!
