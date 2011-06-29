#!/bin/bash
# this script checkout NS3 and DCE sources, and build them.
cd `dirname $BASH_SOURCE`/../..
SAVE_PATH=$PATH
SAVE_LDLP=$LD_LIBRARY_PATH
SAVE_PKG=$PKG_CONFIG_PATH
#echo clone ns-3-dce : 
#hg clone http://code.nsnam.org/furbani/ns-3-dce
echo clone readversiondef
hg clone http://code.nsnam.org/mathieu/readversiondef
echo clone ns-3-linux
hg clone http://code.nsnam.org/mathieu/ns-3-linux
echo clone ns-3-11
hg clone http://code.nsnam.org/ns-3.11
mkdir build
cd ns-3.11 
./waf configure --prefix=`pwd`/../build
./waf
./waf install
cd ..
# Replace pkg files path with the good one :)
cd build
pwd >path
sed -i 's/\//\\\//g' path
REP=$(cat path)
cd lib/pkgconfig
for i in *pc
do
    sed -i "s/\/usr\/local/$REP/g" $i
done
cd ../../..
export PATH=$SAVE_PATH:`pwd`/build/bin
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib
export PKG_CONFIG_PATH=$SAVE_PKG:`pwd`/build/lib/pkgconfig
cd readversiondef/
make 
make install PREFIX=`pwd`/../build/
cd ..
cd ns-3-dce/
./waf configure --prefix=`pwd`/../build --verbose
./waf
./waf install
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib:`pwd`/build/bin:`pwd`/../build/lib
# Configure test simulation files.
mkdir -p files-0/tmp 
mkdir -p files-0/etc
cp /etc/passwd files-0/etc
echo Launch NS3TEST-DCE
./build/bin/ns3test-dce --verbose




