#!/bin/bash
# this script checkout NS3 and DCE sources, and build them.
USE_KERNEL=NO
USE_VDL=NO
USE_MPI=NO
WAF_VDL=
args=("$@")
NB=$#
for (( i=0;i<$NB;i++)); do
    if [ ${args[${i}]} = '-k' ]
    then 
       USE_KERNEL=YES
       WGET=wget
    fi
    if [ ${args[${i}]} = '-v' ]
    then 
       USE_VDL=YES
    fi
    if [ ${args[${i}]} = '-m' ]
    then 
       USE_MPI=YES
       MPI_SWITCH=--enable-mpi
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
echo clone readversiondef
hg clone http://code.nsnam.org/mathieu/readversiondef
if [ "YES" == "$USE_KERNEL" ]
then
	echo clone ns-3-linux
 	hg clone http://code.nsnam.org/furbani/ns-3-linux
fi
echo clone ns-3-dev
hg clone -r 7752dc4ce7e9 http://code.nsnam.org/ns-3-dev
mkdir build
cd ns-3-dev
hg revert -a
patch -p1 <../ns-3-dce/utils/packet-socket-upgrade-exp.patch
patch -p1 <../ns-3-dce/utils/remove-default-simulator-asserts.patch
./waf configure --prefix=`pwd`/../build --enable-tests $MPI_SWITCH
./waf
./waf install
cd ..
export PATH=$SAVE_PATH:`pwd`/build/bin
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib
export PKG_CONFIG_PATH=$SAVE_PKG:`pwd`/build/lib/pkgconfig
cd readversiondef/
make 
make install PREFIX=`pwd`/../build/
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
if [ "YES" == "$USE_VDL" ]
then
	hg clone -r d7ef4732dccc http://code.nsnam.org/mathieu/elf-loader/
	cd elf-loader
	make vdl-config.h
	make
	make test
	cp libvdl.so ../build/lib
	cd ..
	WAF_VDL="--enable-vdl-loader"
fi	
cd ns-3-dce/
if [ "YES" == "$USE_KERNEL" ]
then
    WAF_KERNEL=--enable-kernel-stack=`pwd`/../ns-3-linux
fi
./waf configure --prefix=`pwd`/../build --verbose $WAF_KERNEL $WAF_VDL $MPI_SWITCH
./waf
./waf install
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib:`pwd`/build/bin:`pwd`/../build/lib
. utils/setenv.sh
echo Launch NS3TEST-DCE
./build/bin/ns3test-dce --verbose
