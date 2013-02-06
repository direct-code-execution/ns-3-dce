#!/bin/bash

cd `dirname ${BASH_SOURCE:-$0}`/../
BASEDCE=$PWD
BASE=$PWD/..
LD_LIBRARY_PATH="$BASEDCE/build/lib:$BASE/build/lib:$BASE/build/bin:$BASEDCE/build/bin:.:$LD_LIBRARY_PATH"
PKG_CONFIG_PATH="$BASE/build/lib/pkgconfig"
PATH="$BASE/build/bin:$PATH"
DCE_PATH=$BASEDCE/build/bin_dce:$BASEDCE/build/bin:$BASE/build/bin
DCE_ROOT=$HOME/dce_root:$BASEDCE/build
export LD_LIBRARY_PATH PKG_CONFIG_PATH DCE_PATH DCE_ROOT DCE_PATH BASEDCE

#VERBOSE=""

echo "Cooja (non-vdl) + Pthread:  "
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./build/bin/ns3test-dce $VERBOSE
echo "Cooja (non-vdl) + Ucontext: "
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./build/bin/ns3test-dce $VERBOSE

echo "Cooja (vdl) + Pthread:      "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE

echo "Cooja (vdl) + Ucontext:     "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE

echo "Dlm (vdl) + Pthread:        "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE
echo "Dlm (vdl) + Ucontext:       "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE
