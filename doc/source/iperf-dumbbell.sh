TRIAL="`seq 1 100`"
OUTPUT="output/`date \"+%y%m%d_%H%M\"`"

mkdir -p ${OUTPUT}

# DCCP Test
ssh -f 192.168.39.5 iperf-dccp -d -s -p 5002 > /dev/null
ssh -f 192.168.39.6 iperf-dccp -d -s -p 5002 > /dev/null
# TCP test
ssh -f 192.168.39.5 iperf -s -p 5001 > /dev/null
ssh -f 192.168.39.6 iperf -s -p 5001 > /dev/null


for trial in $TRIAL
do

# for dccp
stack="dccp-linux"
echo -n "$stack $trial " >>$OUTPUT/goodput.txt
ssh 192.168.39.1 iperf-dccp -d -p 5002 -c 10.2.0.2  -t 60 -l 1024 -x MS -y c  | cut -d ',' -f 9 >> 
$OUTPUT/goodput.txt &
ssh 192.168.39.2 iperf-dccp -d -p 5002 -c 10.2.1.2  -t 60 -l 1024 -x MS -y c > /dev/null  &
wait
wait

# for tcp
stack="tcp-linux"
echo -n "$stack $trial " >>$OUTPUT/goodput.txt
ssh 192.168.39.1 iperf -p 5001 -c 10.2.0.2  -t 60 -l 1024 -x MS -y c  | cut -d ',' -f 9 >> $OUTPUT/
goodput.txt &
ssh 192.168.39.2 iperf -p 5001 -c 10.2.1.2  -t 60 -l 1024 -x MS -y c > /dev/null  &
wait
wait

done


ssh -f 192.168.39.5 pkill iperf-dccp

ssh -f 192.168.39.6 pkill iperf-dccp



ssh -f 192.168.39.5 pkill iperf

ssh -f 192.168.39.6 pkill iperf
