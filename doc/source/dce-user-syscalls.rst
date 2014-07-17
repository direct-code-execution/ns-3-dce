.. include:: replace.txt
.. raw:: html

How to add system calls ?
=========================

Introduction
------------
If your applications running with DCE are not able to run due to missing function symbols, you need to add the function call or system call to DCE by hand. The POSIX API coverage of DCE is growing day by day, but your contribution is definitely helpful not only for your case, but also for someone will use in future.

More specifically, if you faced the following error when you executed, you need to add a function call to DCE. In the following case, a symbol **strfry** not defined in DCE is detected during the execution of the simulation.

::
  
  % ./waf --run dce-udp-perf
  'build' finished successfully (0.704s)
  /home/tazaki/hgworks/dce-dev/source/ns-3-dce/build/bin/dce-udp-perf: **relocation error: elf-cache/0/udp-perf: symbol strfry**, version GLIBC_2.2.5 not defined in file 0002.so.6 with link time reference
  Command ['/home/tazaki/hgworks/dce-dev/source/ns-3-dce/build/bin/dce-udp-perf'] exited with code 127


Types of symbol
---------------
There are two types of symbols that is defined in DCE.

* NATIVE
 NATIVE symbol is a symbol that DCE doesn't care about the behavior. So this type of symbol is redirected to the one provided by underlying host operating system (i.e., glibc).

* DCE
 DCE symbol is a symbol that DCE reimplements its behavior instead of using the underlying system's one. For instance, ``socket()`` call used in an application redirected to DCE to cooperate with |ns3| or Linux network stack managed by DCE. ``malloc()`` is also this kind.

In general (but not strictly), if a call is related to a kernel resource (like NIC, clock,
etc), it should use ``DCE`` macro. Otherwise (like strcmp, atoi etc), the call should use ``NATIVE``.



Files should be modified
------------------------
In order to add function calls or system calls that DCE can handle, you need to modify the following files.

* model/libc-ns3.h
 This is the first file that you need to edit. You may lookup the symbol that you're going to add and once you can't find it, add the following line. 

 NATIVE (strfry)

 This is the case of the symbol ``strfry()``, which we don't have to reimplement. But you may need to add include file that defines the symbol (strfry()) at model/libc-dce.cc.

 If the symbol needs to reimplemented for DCE, you may add as follows.

 DCE (socket)

* model/dce-abc.cc
 In case of DCE symbol, you're going to introduce DCE redirected function. We use naming convention with prefix of *dce_* to the symbol (i.e., dce_socket) to define new symbol and add the implementation in a .cc file. The following is the example of ``dce_socket()`` implementation.

 We implemented dce_socket() function in the file **model/dce-fd.cc**. 

  ``int dce_socket (int domain, int type, int protocol)``

 In the function, we carefully fill the function contents to cooperate with |ns3|. The below line is creating DCE specific socket instance (i.e., |ns3| or DCE Linux) instead of calling system call allocating kernel space socket resources.

  ``UnixFd *socket = factory->CreateSocket (domain, type, protocol);``

 Other function calls such as file system related functions (e.g., read, fopen), time reheated features (e.g., gettimeofday, clock_gettime), signal/process utilities (e.g., getpid, sigaction), and thread library (e.g., pthread_create). All these functions should be DCE since DCE core reimplements these feature instead of using underlying host system.

* model/dce-abc.h
 Once you got implemented the new redirected function, you may add the function prototype declaration to refer from other source files. ``dce_socket()`` is added to model/sys/dce-socket.h.
