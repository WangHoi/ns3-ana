#! /bin/sh
output=ana_ana

algo=ana
kbps=100
msDelay=50
msQdelay=300
./waf --run "ana-example --ana=true --kbps=${kbps} --msDelay=${msDelay} --msQdelay=${msQdelay}" 2>a.log || exit

file1=traces/${algo}_rate.txt
file2=traces/${algo}_rateAck.txt
file3=traces/${algo}_rateRecv.txt
ana1_file=traces/ana1_rate.txt
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
 "${ana1_file}" u 1:(\$2/1000) title "ana1 send rate" with lines lw 2
set output
exit
!

file2=traces/${algo}_delay.txt
rtt_file=traces/${algo}_rtt.txt
ana1_file=traces/ana1_delay.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "delay/ms"
set xrange [0:60]
set yrange [0:600]
set term "svg"
set output "${output}_delay.svg"
plot "${file2}" u 1:2 title "recv owd" with lines lw 2, \
 "${rtt_file}" u 1:2 title "rtt" with lines lw 2, \
 "${ana1_file}" u 1:2 title "ana1 owd" with lines lw 2
set output
exit
!

loss_file=traces/${algo}_loss.txt
ana1_file=traces/ana1_loss.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "loss/percent"
set xrange [0:60]
set yrange [0:50]
set term "svg"
set output "${output}_loss.svg"
plot "${loss_file}" u 1:(\$2*100) title "loss" with lines lw 2, \
 "${ana1_file}" u 1:(\$2*100) title "ana1 loss" with lines lw 2
set output
exit
!
