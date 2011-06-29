#!/bin/bash
# Set environnement for ns3 dce
cd `dirname $0` 
BASE=$PWD
LD_LIBRARY_PATH="$BASE/ns-3-dce/build/lib:$BASE/build/lib:$BASE/build/bin:$BASE/ns-3-dce/build/bin:."
PKG_CONFIG_PATH="$BASE/build/lib/pkgconfig"
PATH="$PATH:$BASE/build/bin"
export LD_LIBRARY_PATH PKG_CONFIG_PATH PATH
cd $BASE/ns-3-dce




