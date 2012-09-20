#!/bin/sh

. ./utils/setenv.sh
#VERBOSE=""

echo -n "Cooja (non-vdl) + Pthread:  "
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./build/bin/ns3test-dce $VERBOSE
echo -n "Cooja (non-vdl) + Ucontext: "
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./build/bin/ns3test-dce $VERBOSE

echo -n "Cooja (vdl) + Pthread:      "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE

echo -n "Cooja (vdl) + Ucontext:     "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE

echo -n "Dlm (vdl) + Pthread:        "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE
echo -n "Dlm (vdl) + Ucontext:       "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./build/bin/ns3test-dce-vdl $VERBOSE
