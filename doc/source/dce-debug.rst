.. include:: replace.txt
Debugging your protocols with DCE
*********************************

Gdb
===
It is possible to use gdb to debug a script DCE/|ns3|. As explained somewhere in the execution of a script is monoprocess, then you can put breakpoints in both sources of DCE and those of binaries hosted by DCE.

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

(FIXME: simple session using dce_debug_nodeid () in gdb)



Valgrind
========
(FIXME: simple session using dce_debug_nodeid () in gdb)
