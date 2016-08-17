.. include:: replace.txt
Tuning System Limits
====================

When dealing with large or complex models, you can easily reach the limits of your system. 
For example, you cannot open more than a fixed number of files. 
You can try the command *"limit –a"* to check them.

File limits: "Could not open ..."
.................................

You may see the following error:

::

  msg="Could not open "/var"", file=../model/dce-manager.cc, line=149
  terminate called without an active exception

This error masks error "24 Too many open files". The cause of this is that the
simulation process exceeded the limit of open files per process.
Check the limit of open files per process with "ulimit -n"
To solve it, you can edit file */etc/security/limits.conf* and add the following lines at the end:

::

  *         hard    nofile      65536
  *         soft    nofile      65536

or

::

  myuser         hard    nofile      65536
  myuser         soft    nofile      65536



Processes limit: "Resource temporarily unavailable"
...................................................

In this case you may see the an error like the following:

::

  assert failed. cond="error == 0", msg="error=Resource temporarily unavailable", 
  file=../model/pthread-fiber-manager.cc, line=321
  terminate called without an active exception


*pthread-fibder-manager* invokes *pthread_create* this is what
raises the "Resource temporarily unavailable".
This problem might be triggered because the maximum number of user processes
is not big enough. Use *"ulimit -u"* to check this limit.
To solve it, you can edit file */etc/security/limits.conf* and add the following lines at the end:

::

  *         hard    nproc     65536
  *         soft    nproc      65536

or

::

  myuser         hard    nproc     65536



Stack size
..........


DCE directly manages the stack of the processes running on it, 
assigning it a default value 8192. For complex executables
this value is too small, and may raise 'stack overflow' exceptions,
or in other cases it may originate inconsistent values. For example,
a value passed to a function changes without apparent reason when the program enters in 
that function.
The value of the stack size can be changed with the SetStackSize instruction:

.. code-block:: c++

  DceApplicationHelper dce;
  dce.SetStackSize (1<<20);


