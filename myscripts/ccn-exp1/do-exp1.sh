#!/bin/bash
for size in 10 15 20 30 40
do
   SZ=$(expr 100 '*' $size)
   echo Size is $SZ bytes
   time dce-ccn-exp1 --ccnd=1 --size=$SZ
   tshark -z io,stat,0.01,ip.dst==10.1.1.1 -r dce-ccn-xchg-$SZ-NODE_Zero-0.pcap -w /dev/null >udp_tput_$SZ.data
done
time dce-ccn-exp1 --ccnd=0
tshark -z io,stat,0.01,ip.dst==10.1.1.2 -r dce-tcp-xchg-NODE_Zero-0.pcap -w /dev/null >tcp_tput.data
gnuplot plt1.txt -

 