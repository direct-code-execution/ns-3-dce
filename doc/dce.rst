Direct Code Execution
*********************

System packages
===============

Ubuntu 1004-32:::

  sudo apt-get install g++ libc-dbg mercurial git-core

Basic install (no Linux stack)
==============================

First, let's get the ns-3-dce source code and configure it:::

  hg clone http://code.nsnam.org/mathieu/ns-3-dce
  cd ns-3-dce
  ./waf configure --disable-python

The above should output something like this:::

  Linux stack                   : not enabled (linux stack not found)
  Magic Elf Loader              : enabled

Then, we can build it which should take some time:::

  ./waf 

Now, we can check that it works by running the associated tests:::

  ./test.py -n -s process-manager

Which should output something like this:::

  PASS: TestSuite process-manager
  1 of 1 tests passed (1 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)

If it does not work, please, report this problem.

Now, let's run a simple example:::

  ./waf shell
  ./build/debug/src/dce/examples/dce-tcp-simple

This program creates a single ns-3 simulation Node with an IPv4/TCP stack
and a loopback device. It also creates a single process from the binary tcp-loopback 
whose source code lives in tcp-loopback.cc. This process creates a client and a server
thread. The client thread connects to the server thread and sends a bunch of data to
this server. When the simulation completes, you can look at the standard output of 
this process which interleaves the client and server output with the following command:::

  cat cat files-0/var/log/*/stdout

Where files-0 is the directory where all files for node 0 are located.

Another simple example is dce-udp-simple:::

  ./build/debug/src/dce/examples/dce-udp-simple

Which still involves only one node, but two udp processes on the same node sending
data through the loopback device. If it completes well, you should be able to do:::

  cat files-0/var/log/*/stdout

and get this:::

  did read all buffers
  did write all buffers

Other things that you can now do involve playing with the different thread
and loader implementations to check that they all work. First, let's see the impact
of the faster but harder to debug ucontext-based fiber manager:::

  time -p build/debug/src/dce/example/dce-udp-simple --ns3::TaskManager::FiberManagerType=UcontextFiberManager
  real 31.08
  user 29.83
  sys 0.60

versus the default fiber manager which is nicer to run under gdb:::

  time -p ./build/debug/src/dce/example/dce-udp-simple
  real 36.83
  user 30.00
  sys 5.90

To use the so-called dlm loader, you need to run your simulation scenario with a special
runner program which takes care of setting up the DlmLoaderFactory for you:::

  time -p ./build/debug/src/dce/utils/dce-runner ./build/debug/src/dce/example/dce-udp-simple 
  real 32.97
  user 26.67
  sys 5.84

versus the default loader:::

  time -p ./build/debug/src/dce/example/dce-udp-simple
  real 36.83
  user 30.00
  sys 5.90

If you put it all together, it's obvious how you can make your code magically faster:::

  time -p ./build/debug/src/dce/utils/dce-runner ./build/debug/src/dce/example/dce-udp-simple --ns3::TaskManager::FiberManagerType=UcontextFiberManager
  real 23.60
  user 23.08
  sys 0.34

Or, summarized:

+-------------------+------------------------+------------+
|                   | Cooja loader (default) | Dlm loader |
+-------------------+------------------------+------------+
| pthread (default) |   36.8s                |   33s      |
+-------------------+------------------------+------------+
| ucontext          |   31.1s                |   23s      |
+-------------------+------------------------+------------+


Gdb
===

Install
-------

Although it is not strictly necessary, it is recommended that you recompile a CVS Gdb 
for use with ns-3-dce. First, download:::

  cvs -d :pserver:anoncvs@sourceware.org:/cvs/src login
  {enter "anoncvs" as the password}
  cvs -d :pserver:anoncvs@sourceware.org:/cvs/src co gdb

Note that you might consider looking at http://sourceware.org/gdb/current/ to obtain
more efficient (cpu/bandwidth-wise) download instructions.

Anyway, now, you can build:::

  cd gdb
  ./configure
  make

And, then, invoke the version of gdb located in gdb/gdb instead of your system-installed gdb
whenever you need to debug a DCE-based program.

Using
-----

If you use gdb (a CVS or stable version), do not forget to execute the following command prior
to running any DCE-based program:::

  (gdb) handle SIGUSR1 nostop
  Signal        StopPrintPass to programDescription
  SIGUSR1       NoYesYesUser defined signal 1
  (gdb)

An alternate way to do this and avoid having to repeat this command ad-nauseam
involves creating a .gdbinit file in your ns-3-dce directory and putting this
inside:::

  handle SIGUSR1 nostop

Helpful debugging hints
-----------------------

There are a couple of functions which are useful to put breakpoints into:

 * ns3::DceManager::StartProcessDebugHook






