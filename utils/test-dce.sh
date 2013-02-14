#!/bin/bash

cd `dirname ${BASH_SOURCE:-$0}`/../

#VERBOSE=""

echo -n "Copy (non-vdl) + Pthread:   "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CopyLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./test.py -n -c unit |grep TestS
echo -n "Copy (non-vdl) + Ucontext:  "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CopyLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./test.py -n -c unit |grep TestS 

echo -n "Cooja (non-vdl) + Pthread:  "
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./test.py -n -c unit |grep TestS 
echo -n "Cooja (non-vdl) + Ucontext: "
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./test.py -n -c unit |grep TestS 

echo -n "Copy (vdl) + Pthread:       "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CopyLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./test.py -z ns3test-dce-vdl -n -c unit |grep TestS 

echo -n "Copy (vdl) + Ucontext:      "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CopyLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./test.py -z ns3test-dce-vdl -n -c unit |grep TestS 

echo -n "Cooja (vdl) + Pthread:      "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./test.py -z ns3test-dce-vdl -n -c unit |grep TestS 

echo -n "Cooja (vdl) + Ucontext:     "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./test.py -z ns3test-dce-vdl -n -c unit |grep TestS 

echo -n "Dlm (vdl) + Pthread:        "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[];ns3::TaskManager::FiberManagerType=PthreadFiberManager' ./test.py -z ns3test-dce-vdl -n -c unit |grep TestS 
echo -n "Dlm (vdl) + Ucontext:       "
NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./test.py -z ns3test-dce-vdl -n -c unit |grep TestS 
