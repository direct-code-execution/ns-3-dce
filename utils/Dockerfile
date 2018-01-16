FROM ns3dce/ubuntu14.04:0.1
MAINTAINER Hajime Tazaki <thehajime@gmail.com>

# bake
RUN mkdir -p /home/ns3dce/dce-linux-dev
WORKDIR /home/ns3dce/dce-linux-dev
RUN git clone https://github.com/thehajime/bake

RUN ./bake/bake.py configure -e dce-linux-dev -e dce-umip-dev
RUN ./bake/bake.py download
RUN ./bake/bake.py build

RUN cd source/ns-3-dce && ./test.py
