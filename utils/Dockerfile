FROM ubuntu:12.04
MAINTAINER Hajime Tazaki <thehajime@gmail.com>

RUN apt-get update
RUN apt-get install -y mercurial libexpat1-dev git-core build-essential bison flex libssl-dev libdb-dev libpcap-dev libc6-dbg pkg-config

WORKDIR /var/lib/dce-linux-dev
# bake
RUN mkdir -p /var/lib/dce-linux-dev
RUN hg clone http://code.nsnam.org/bake

RUN ./bake/bake.py configure -e dce-linux-dev
RUN ./bake/bake.py download
RUN ./bake/bake.py build

RUN cd source/ns-3-dce && ./test.py
