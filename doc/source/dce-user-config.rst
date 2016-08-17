.. include:: replace.txt
Global DCE Configurations
=========================

Parameters
----------

The DCE specifics variables are essentially two PATH like variables: so within them you may put paths separated by ':' character.

**DCE_PATH** is used by DCE to find the executable you want to launch within |ns3| simulated network. This variable is used when you reference the executable using a relative form like 'ping'.

**DCE_ROOT** is similar to DCE_PATH but it is used when you use an absolute form for exemple '/bin/bash'.

**Please pay attention** that executables that you will place in the directories indicated in the previous variables should be recompiled accordingly to the rules defined in the next chapter.

(FIXME: to be updated)

Tweaking
--------

DCE is configurable with NS3 Attributes. Refer to the following table:

+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+
| ATTRIBUTE NAME       | DESCRIPTION                                                      |  VALUES                   | EXAMPLES                                                           |
+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+
|**FiberManagerType**  |The TaskManager is used to switch the execution context between   |**UcontextFiberManager**   |``--ns3::TaskManager::FiberManagerType=UcontextFiberManager``       |
|                      |threads and processes.                                            |the more efficient.        |                                                                    |
|                      |                                                                  |                           |``dceManager.SetTaskManagerAttribute("FiberManagerType",            |
|                      |                                                                  |                           |StringValue("UcontextFiberManager"));``                             |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |**PthreadFiberManager**    |``--ns3::TaskManager::FiberManagerType=PthreadFiberManager``        |
|                      |                                                                  |helpful with **gdb** to see|                                                                    |
|                      |                                                                  |the threads. This is the de|                                                                    |
|                      |                                                                  |fault.                     |                                                                    |
+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+
|**LoaderFactory**     |The LoaderFactory is used to load the hosted binaries.            |**CoojaLoaderFactory** is  |``--ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[]``|
|                      |                                                                  |the default and the only   |                                                                    |
|                      |                                                                  |one that supports ``fork``.|``$ dce-runner my-dce-ns3-script``                                  |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |                           |OR                                                                  |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |**DlmLoaderFactory** is the|``gcc -o  my-dce-ns3-script Wl,--dynamic-linker=PATH2LDSO/ldso ...``|
|                      |                                                                  |more efficient. To use it  |                                                                    |
|                      |                                                                  |you have two ways:         |``$ my-dce-ns3-script --ns3::DceManagerHelper::LoaderFactory=ns3::Dl|
|                      |                                                                  |                           |mLoaderFactory[]``                                                  |
|                      |                                                                  |1/ use ``dce-runner``      |                                                                    |
|                      |                                                                  |2/ link using ``ldso`` as  |``dceManager.SetLoader("ns3::DlmLoaderFactory");``                  |
|                      |                                                                  |default interpreter.       |                                                                    |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |                           |                                                                    |
+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+

