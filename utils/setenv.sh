#!/bin/bash
# Set environnement for ns3 dce
cd `dirname $BASH_SOURCE`/../..
BASE=$PWD
LD_LIBRARY_PATH="$BASE/ns-3-dce/build/lib:$BASE/build/lib:$BASE/build/bin:$BASE/ns-3-dce/build/bin:.:$LD_LIBRARY_PATH"
PKG_CONFIG_PATH="$BASE/build/lib/pkgconfig"
#PATH="$BASE/build/bin:$PATH"
DCE_PATH=$BASE/ns-3-dce/build/bin_dce
DCE_ROOT=$HOME/dce_root:$BASE/ns-3-dce/build
export LD_LIBRARY_PATH PKG_CONFIG_PATH DCE_PATH DCE_ROOT DCE_PATH
cd $BASE/ns-3-dce





