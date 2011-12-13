#!/bin/bash
echo Getting and building coreutils
FROM=$PWD
mkdir /tmp/build_coreutils_$$
cd /tmp/build_coreutils_$$
wget ftp://ftp.gnu.org/gnu/coreutils/coreutils-8.5.tar.xz
tar xvf coreutils-8.5.tar.xz
cd coreutils-8.5
CFLAGS="-g -fPIC" LDFLAGS=-pie ./configure --enable-no-install-program=arch,hostname,su,libstdbuf.so --prefix=$FROM
#CFLAGS="-g -fPIC" LDFLAGS=-pie ./configure --prefix=$FROM
# do not try to build libstdbuf because it's fail with this options.
make 
touch src/libstdbuf.so
make install
cd $FROM
ls -ltr



