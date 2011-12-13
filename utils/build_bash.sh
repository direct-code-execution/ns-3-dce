#!/bin/bash
echo It is funny but we use bash to download and build bash.
FROM=$PWD
mkdir /tmp/build_bash_$$
cd /tmp/build_bash_$$
wget ftp://ftp.gnu.org/gnu/bash/bash-4.1.tar.gz
for i in 01 02 03 04 05 06 07 08 09 10 11
do
  wget ftp://ftp.gnu.org/gnu/bash/bash-4.1-patches/bash41-0$i
done
tar xvf bash-4.1.tar.gz
cd bash-4.1/
for i in 01 02 03 04 05 06 07 08 09 10 11
do
  patch -p0 <../bash41-0$i
done
CFLAGS="-g -fPIC" LDFLAGS=-pie CFLAGS_FOR_BUILD='-g -fPIC'   ./configure --without-bash-malloc
make
cp bash $FROM
cd $FROM
