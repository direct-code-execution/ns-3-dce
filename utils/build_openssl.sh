#!/bin/bash
echo Getting and building OpenSSL
INSTALLDIR=$1
FROM=$PWD
mkdir /tmp/build_openssl_$$
cd /tmp/build_openssl_$$
wget ftp://ftp.openssl.org/source/openssl-1.0.0e.tar.gz
tar xvf openssl-1.0.0e.tar.gz
cd openssl-1.0.0e
./config --prefix=$INSTALLDIR --openssldir=/usr/local/ssl
mv Makefile Makefile.ori
sed -e 's/^\(CFLAG=.*\)/\1 -fPIC -pie/' Makefile.ori >Makefile
make
mv Makefile Makefile.ori2
sed -e "s/^\(OPENSSLDIR=.*\)/OPENSSLDIR=\$(INSTALLTOP)/" Makefile.ori2 >Makefile
make install
