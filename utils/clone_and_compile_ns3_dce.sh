#!/bin/bash
# this script checkout NS3 and DCE sources, and build them.
USE_KERNEL=NO
args=("$@")
NB=$#
for (( i=0;i<$NB;i++)); do
    if [ ${args[${i}]} = '-k' ]
    then 
       USE_KERNEL=YES
       WGET=wget
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
hg clone http://code.nsnam.org/ns-3-dev -r 49dadc40be43
mkdir build
cd ns-3-dev
patch -p1 <../ns-3-dce/utils/packet-socket-upgrade-exp.patch
./waf configure --prefix=`pwd`/../build --enable-tests
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
 	git clone git://git.kernel.org/pub/scm/linux/kernel/git/davem/net-next.git net-next-2.6
	make unpatch
	make  setup
	make defconfig
	make config
	sed s/CONFIG_PACKET=m/CONFIG_PACKET=y/ config >c2
	rm config
	mv c2 config
	make
	cd ..
	wget http://www.linuxgrill.com/anonymous/iproute2/NEW-OSDL/iproute2-2.6.38.tar.bz2     
	tar jxf iproute2-2.6.38.tar.bz2
	cd iproute2-2.6.38
	./configure
	LDFLAGS=-pie make CCOPTS='-fpic -D_GNU_SOURCE -O0 -U_FORTIFY_SOURCE'
	cd ../ns-3-dce
	ln -s ../ns-3-linux/libnet-next-2.6.so
	mkdir -p build/bin_dce
	cd  build/bin_dce
	ln -s ../../../ns-3-linux/libnet-next-2.6.so
	ln -s ../../../iproute2-2.6.38/ip/ip
	cd ../../example/ccnx
	ln -s ../../build/bin_dce/libnet-next-2.6.so
	cd ../..
fi
cd ns-3-dce/
if [ "YES" == "$USE_KERNEL" ]
then
    WAF_KERNEL=--enable-kernel-stack=`pwd`/../ns-3-linux
fi
./waf configure --prefix=`pwd`/../build --verbose $WAF_KERNEL
./waf
./waf install
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib:`pwd`/build/bin:`pwd`/../build/lib
. utils/setenv.sh
echo Launch NS3TEST-DCE
./build/bin/ns3test-dce --verbose





