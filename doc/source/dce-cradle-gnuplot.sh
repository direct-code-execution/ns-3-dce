#!/bin/bash

#set -x
OUTPUT=$1

mkdir -p ${OUTPUT}

TRIAL="`seq 1 100`"
STACKS="dce dce-linux3 ns3 nsc-linux"

# for Stack - Kbps (TCP)
cat ${OUTPUT}/goodput.txt | dbcoldefine exp seed bps |dbmultistats -k exp bps |dbcol exp mean stddev | grep -v dccp > ${OUTPUT}/goodput.dat
cat ${OUTPUT}/121120_1701/goodput.txt | dbcoldefine exp seed bps |dbmultistats -k exp bps |dbcol exp mean stddev | grep -v dccp >> ${OUTPUT}/goodput.dat

gnuplot  << EndGNUPLOT
set ylabel "Goodput (kbps)"
set terminal postscript eps lw 3 "Helvetica" 24
set output "${OUTPUT}/goodput.eps"
set xrange [-1:5]
set xtics font "Helvetica,14"
set xtics ("1) DCE\ncradle" 0,  "2) DCE cradle\n(Linux3)" 1, "3) ns-3" 2, "4) NSC Linux" 3, "5) Linux\n2.6.32-28" 4)
set style fill pattern
set boxwidth 0.5
set pointsize 2
set xzeroaxis
set grid ytics

plot \
        '${OUTPUT}/goodput.dat' usi :(\$2/1000):(\$3/1000) notitle w boxerrorbars

set terminal png lw 3 16
set xtics nomirror rotate by -45 font ",16"
set output "${OUTPUT}/goodput-dce-cradle-tcp.png"
replot

quit
EndGNUPLOT

# for Stack - Kbps (DCCP)
cat ${OUTPUT}/goodput.txt | dbcoldefine exp seed bps |dbmultistats -k exp bps |dbcol exp mean stddev | grep dccp | grep -v dccp3 > ${OUTPUT}/goodput-dccp.dat
cat ${OUTPUT}/121120_1701/goodput.txt | dbcoldefine exp seed bps |dbmultistats -k exp bps |dbcol exp mean stddev | grep dccp >> ${OUTPUT}/goodput-dccp.dat

gnuplot  << EndGNUPLOT
set ylabel "Goodput (kbps)"
set terminal postscript eps lw 3 "Helvetica" 24
set output "${OUTPUT}/goodput-dccp.eps"
set xrange [-1:2]
set yrange [0:]
set xtics font "Helvetica,14"
set xtics ("DCE cradle\n(DCCP)" 0, "Linux 2.6.32-28\n(DCCP)" 1)
#set xtics ("DCE cradle\n(DCCP)" 0,  "DCE cradle\n(DCCP,Linux3)" 1, "Linux 2.6.32-28\n(DCCP)" 2)
set style fill pattern
set boxwidth 0.5
set pointsize 2
set xzeroaxis
set grid ytics

plot \
        '${OUTPUT}/goodput-dccp.dat' usi :(\$2/1000):(\$3/1000) notitle w boxerrorbars

set terminal png lw 3 16
unset xtics 
set xtics ("DCE cradle\n(DCCP)" 0, "Linux 2.6.32-28\n(DCCP)" 1)
set output "${OUTPUT}/goodput-dccp.png"
replot

quit
EndGNUPLOT



# for Nnodes - RealTime
rm -f ${OUTPUT}/sim-time-nnodes*.dat
for stack in $STACKS
do
cat ${OUTPUT}/sim-time-nnodes.txt | grep "$stack " | dbcoldefine stack node trial time | dbmultistats -k node time |dbsort -n node | dbcol node mean stddev >> ${OUTPUT}/sim-time-nnodes-$stack.dat
done


gnuplot  << EndGNUPLOT
set ylabel "Actual Time (s)"
set xlabel "Number of nodes (n)"
set terminal postscript eps lw 3 "Helvetica" 24
set output "${OUTPUT}/sim-time-nnodes.eps"
#set xrange [0:49]
set yrange [0:40]
#set xtics font "Helvetica,16"
set pointsize 1.5
set xzeroaxis
set grid
set key top left

plot \
        '${OUTPUT}/sim-time-nnodes-dce.dat' usi 1:2 title "DCE cradle" w lp pt 9, \
        '${OUTPUT}/sim-time-nnodes-dce-linux3.dat' usi 1:2 title "DCE cradle (linux-3)" w lp, \
        '${OUTPUT}/sim-time-nnodes-ns3.dat' usi 1:2 title "ns-3" w lp pt 6, \
        '${OUTPUT}/sim-time-nnodes-nsc-linux.dat' usi 1:2 title "NSC Linux" w lp ,\
        '${OUTPUT}/numstack-1000-sim-time-nnodes-nsc-linux.dat' usi 1:2 title "NSC-1000" w lp 

set terminal png lw 3 16
set output "${OUTPUT}/sim-time-nnodes.png"
replot

quit
EndGNUPLOT

# for Duration - RealTime
rm -f ${OUTPUT}/sim-time-duration*.dat
for stack in $STACKS
do
cat ${OUTPUT}/sim-time-duration.txt | grep "$stack " | dbcoldefine stack duration trial time | dbmultistats -k duration time |dbsort -n duration | dbcol duration mean stddev >> ${OUTPUT}/sim-time-duration-$stack.dat
done

gnuplot  << EndGNUPLOT
set ylabel "Actual Time (s)"
set xlabel "Simulation Time (s)"
set terminal postscript eps lw 3 "Helvetica" 24
set output "${OUTPUT}/sim-time-duration.eps"
set pointsize 1.5
set xzeroaxis
set yrange [0:15]
set grid
set key top left

plot \
        '${OUTPUT}/sim-time-duration-dce.dat' usi 1:2 title "DCE cradle" w lp pt 9, \
        '${OUTPUT}/sim-time-duration-dce-linux3.dat' usi 1:2 title "DCE cradle (linux-3)" w lp, \
        '${OUTPUT}/sim-time-duration-ns3.dat' usi 1:2 title "ns-3" w lp pt 6, \
        '${OUTPUT}/sim-time-duration-nsc-linux.dat' usi 1:2 title "NSC Linux" w lp ,\
        '${OUTPUT}/numstack-1000-sim-time-duration-nsc-linux.dat' usi 1:2 title "NSC-1000" w lp 

set terminal png lw 3 16
set output "${OUTPUT}/sim-time-duration.png"
replot

quit
EndGNUPLOT


# for tcptrace graph
for stack in $STACKS
do

 if [ "$stack" != "dce-dccp" ] && [ "$stack" != "dce-dccp3" ] ; then

xpl2gpl ${OUTPUT}/a2b_tsg-$stack.xpl
mv a2b_tsg-$stack* ${OUTPUT}/
grep -E -v "replot|term|output|pause" ${OUTPUT}/a2b_tsg-$stack.gpl > a
echo "set xrange [:-946684790.0]\nset term postscript enhanced lw 3 \"Helvetica\" 24\nset output \"a2b_tsg-$stack.eps\"" | cat - a > ${OUTPUT}/a2b_tsg-$stack.gpl
tmpPWD=`pwd`
cd ${OUTPUT}; gnuplot a2b_tsg-$stack.gpl
cd ${tmpPWD}
 fi
done


