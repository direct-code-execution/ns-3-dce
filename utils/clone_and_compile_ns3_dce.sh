#!/bin/bash

echo "This script is obsolted."
echo "Please use bake to install dce and ns-3-dce. exit"
echo "For more information, http://www.nsnam.org/projects/direct-code-execution/"
exit

# this script checkout NS3 and DCE sources, and build them.
USE_KERNEL=NO
USE_MPI=NO
USE_OPT=NO
args=("$@")
NB=$#
for (( i=0;i<$NB;i++)); do
    if [ ${args[${i}]} = '-k' ]
    then 
       USE_KERNEL=YES
       WGET=wget
    fi
    if [ ${args[${i}]} = '-m' ]
    then 
       USE_MPI=YES
       MPI_SWITCH=--enable-mpi
    fi
    if [ ${args[${i}]} = '-o' ]
    then 
       USE_OPT=YES
       OPT_SWITCH=--enable-opt
    fi    
done 
for i in patch hg make $WGET tar
do
which $i >/dev/null
if [ 1 == $? ]
then
	echo $i not found !
	echo "Please, install $i command line tool."
	exit 1
fi
done
cd `dirname $BASH_SOURCE`/../..
SAVE_PATH=$PATH
SAVE_LDLP=$LD_LIBRARY_PATH
SAVE_PKG=$PKG_CONFIG_PATH
#echo clone ns-3-dce : 
#hg clone http://code.nsnam.org/furbani/ns-3-dce

echo clone elf-loader
hg clone http://code.nsnam.org/mathieu/elf-loader/

if [ "YES" == "$USE_KERNEL" ]
then
	echo clone ns-3-linux
 	hg clone http://code.nsnam.org/furbani/ns-3-linux
fi
echo clone ns-3-dev
hg clone -r 3c5ea0bd869f http://code.nsnam.org/ns-3-dev
mkdir build
cd ns-3-dev
hg revert -a
if [ "YES" == "$USE_OPT" ]
then
	CXXFLAGS='-O3' ./waf configure -d optimized --prefix=`pwd`/../build --enable-tests $MPI_SWITCH
else
	./waf configure --prefix=`pwd`/../build --enable-tests $MPI_SWITCH
fi
./waf
./waf install
cd ..
export PATH=$SAVE_PATH:`pwd`/build/bin
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib
export PKG_CONFIG_PATH=$SAVE_PKG:`pwd`/build/lib/pkgconfig
# build elf-loader
cd elf-loader
patch -f -p1 < ../ns-3-dce/utils/elf-loader-r633.patch
make vdl-config.h
make
make test
cp ldso libvdl.so ../build/lib
cd ..

if [ "YES" == "$USE_KERNEL" ]
then
	cd ns-3-linux/
	make
	cd ..
	wget http://www.linuxgrill.com/anonymous/iproute2/NEW-OSDL/iproute2-2.6.38.tar.bz2     
	tar jxf iproute2-2.6.38.tar.bz2
	cd iproute2-2.6.38
	./configure
	LDFLAGS=-pie make CCOPTS='-fpic -D_GNU_SOURCE -O0 -U_FORTIFY_SOURCE'
	cd ../ns-3-dce
	mkdir -p build/bin_dce
	cd  build/bin_dce
	ln -s ../../../ns-3-linux/libnet-next-2.6.so liblinux.so
	ln -s ../../../ns-3-linux/libnet-next-2.6.so
	ln -s ../../../iproute2-2.6.38/ip/ip
	cd ../..
fi
cd ns-3-dce/
if [ "YES" == "$USE_KERNEL" ]
then
    WAF_KERNEL=--enable-kernel-stack=`pwd`/../ns-3-linux
fi
./waf configure --with-ns3=`pwd`/../build --prefix=`pwd`/../build --verbose $WAF_KERNEL $MPI_SWITCH $OPT_SWITCH
./waf
./waf install
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib:`pwd`/build/bin:`pwd`/../build/lib
echo Launch NS3TEST-DCE
./waf --run "ns3test-dce --verbose"
