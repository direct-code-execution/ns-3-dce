#!/bin/sh


OUTFILE=./dce-syscall.dat
echo > ${OUTFILE}

# for older dce (simu)
cd ns-3-dce-old
DCE_NEW_HEAD=`hg head --template "{rev}"`
loop=4741
while [ $loop -le ${DCE_NEW_HEAD} ];
do
hg update -r $loop -C > /dev/null

# simu era
if [ $loop -le 6722 ] ; then
if [ ! -f src/process-manager/libc-simu.c ] ; then
 loop=`expr $loop + 1`
 continue 
fi
CNT=`grep _fn src/process-manager/libc-simu.c |wc -l`
else
CNT=`grep _fn src/dce/model/libc-dce.c |wc -l`
fi

DATE=`hg parents -R . --template "{date|shortdate}"`
echo $DATE $CNT >> ${OUTFILE}
loop=`expr $loop + 1`
done

cd ..


# for new DCE
cd ns-3-dce
DCE_NEW_HEAD=`hg head --template "{rev}"`

loop=1
while [ $loop -le ${DCE_NEW_HEAD} ];
do
hg update -r $loop -C > /dev/null

if [ $loop -le 180 ] ; then
CNT=`grep _fn ./model/libc-dce.c |wc -l`
else
CNT=`grep -E "DCE|NATIVE" ./model/libc-ns3.h |grep -v \#| grep -v '*' | grep -v '//' |wc -l`
fi

DATE=`hg parents -R . --template "{date|shortdate}"`
echo $DATE $CNT >> ${OUTFILE}
loop=`expr $loop + 1`

done
cd ..


gnuplot << EndGNUPLOT
set ylabel "The number of emulated system calls (n)"
set xlabel "Date (Y/M)"
set terminal postscript eps lw 3 "Helvetica" 24
set output "dce-syscalls.eps"
set pointsize 2
set xzeroaxis
set grid
set xdata time
set timefmt "%Y-%m-%d"
set format x "%y/%m"
set nokey
set xtics "2009 07 01", 60*60*24*180
set xrange ["2009 07 01":"2013 01 01"]

plot "${OUTFILE}" using 1:2 with lp
EndGNUPLOT
