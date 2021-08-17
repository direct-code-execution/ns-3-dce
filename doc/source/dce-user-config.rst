.. include:: replace.txt
Global DCE Configurations
=========================

Parameters
----------

The DCE specifics variables are essentially two PATH like variables: so within them you may put paths separated by ':' character.

**DCE_PATH** is used by DCE to find the executable you want to launch within |ns3| simulated network. This variable is used when you reference the executable using a relative form like 'ping'.

**DCE_ROOT** is similar to DCE_PATH but it is used when you use an absolute form; for example, '/bin/bash'.

**Please pay attention** that executables that you will place in the directories indicated in the previous variables should be recompiled accordingly to the rules defined in the next chapter.

(FIXME: to be updated)

Tweaking
--------

DCE is configurable with |ns3| attributes. Refer to the following table:

.. table:: DCE attributes
   :widths: 20 20 25 35
.. tabularcolumns:: |p{3cm}|p{3cm}|p{4cm}|p{5.5cm}|


+----------------------+------------------------------------------------------------------+---------------------------+---------------------------------------+
| ATTRIBUTE NAME       | DESCRIPTION                                                      |  VALUES                   | EXAMPLE USAGE                         |
+----------------------+------------------------------------------------------------------+---------------------------+---------------------------------------+
|**FiberManagerType**  |The TaskManager is used to switch the execution context between   |**UcontextFiberManager**   |--ns3::TaskManager::FiberManagerType   |
|                      |threads and processes.                                            |the more efficient.        |                                       |
|                      |                                                                  |                           |OR                                     | 
|                      |                                                                  |                           |                                       |
|                      |                                                                  |**PthreadFiberManager**    |dceManager.SetTaskManagerAttribute()   |
|                      |                                                                  |helpful with **gdb** to see|                                       |
|                      |                                                                  |the threads. This is the   |                                       |
|                      |                                                                  |default.                   |                                       |
+----------------------+------------------------------------------------------------------+---------------------------+---------------------------------------+
|**LoaderFactory**     |The LoaderFactory is used to load the hosted binaries.            |**CoojaLoaderFactory** is  |--ns3::DceManagerHelper::LoaderFactory |
|                      |                                                                  |the default and the only   |                                       |
|                      |                                                                  |one that supports ``fork`` |$ dce-runner my-dce-ns3-script         |
|                      |                                                                  |                           |                                       |
|                      |                                                                  |                           |OR                                     |
|                      |                                                                  |                           |                                       | 
|                      |                                                                  |**DlmLoaderFactory** is the|gcc -o  my-dce-ns3-script              | 
|                      |                                                                  |more efficient.  To use it |Wl,--dynamic-linker=PATH2LDSO/ldso ... |
|                      |                                                                  |you have two ways:         |                                       |
|                      |                                                                  |                           |$ my-dce-ns3-script                    |
|                      |                                                                  |                           |--ns3::DceManagerHelper::LoaderFactory |
|                      |                                                                  |                           |                                       |
|                      |                                                                  |1/ use ``dce-runner``      |                                       |
|                      |                                                                  |2/ link using ``ldso`` as  |dceManager.SetLoader(...);             |
|                      |                                                                  |default interpreter.       |                                       |
|                      |                                                                  |                           |                                       |
+----------------------+------------------------------------------------------------------+---------------------------+---------------------------------------+

