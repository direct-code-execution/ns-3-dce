#!/bin/bash
# Set environnement for ns3 dce
cd `dirname $BASH_SOURCE`/../
BASEDCE=$PWD
cd ..
BASE=$PWD
LD_LIBRARY_PATH="$BASEDCE/build/lib:$BASE/build/lib:$BASE/build/bin:$BASEDCE/build/bin:.:$LD_LIBRARY_PATH"
PKG_CONFIG_PATH="$BASE/build/lib/pkgconfig"
PATH="$BASE/build/bin:$PATH"
DCE_PATH=$BASEDCE/build/bin_dce:$BASEDCE/build/bin:$BASE/build/bin
DCE_ROOT=$HOME/dce_root:$BASEDCE/build
export LD_LIBRARY_PATH PKG_CONFIG_PATH DCE_PATH DCE_ROOT DCE_PATH
cd $BASEDCE






