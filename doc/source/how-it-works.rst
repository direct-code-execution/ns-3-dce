.. include:: replace.txt

.. _how-it-works:

How It Works
************

If you are interested to know why DCE exists and how it can work, you should read this document `Experimentation Tools for Networking Research <http://cutebugs.net/files/thesis.pdf>`_ (and in particular the chapter 4) written by the principal author of DCE, Mathieu Lacage. Also you can read the sources too.


Introduction
============

You know that the goal of DCE is to execute actual binaries within |ns3|.
More precisely a binary executed by DCE perceives time and space of |ns3|, rather than the real environment.

To do this, DCE does a job equivalent to an operating system like:

1. DCE loads in memory the code and data of executable,
2. DCE plays the role of intermediary between the executable and the environment through the systems functions called by executables,
3. DCE manages and monitors the execution of processes and handles liberate the memory and close open files when the stop process.
4. DCE manages the scheduling of the various virtual processes and threads.

Main classes and main data structures
=====================================

DceManager
----------

The **DceManager** is somewhat the entry point of DCE. It will create virtual processes and manages their execution.
There is an instance of **DceManager** which is associated to each node which need to virtualize the execution of a process.
In reality, the developer uses the classes **DceManagerHelper** and **DceApplicationHelper**. 

I invite you to look at the source code *dce-manager.cc* and *dce-manager.h* and particularly the public methods **Start**, **Stop**, **Exit**, **Wakeup**, **Wait** and **Yield**;
the following private methods are also important: **CreateProcess**, **PrepareDoStartProcess**, **DoStartProcess**, **AllocatePid**, **TaskSwitch**, **CleanupThread** and **LoadMain**.
The **Start** method is called when starting the executable, if you look at, it begins by initializing an object of type **struct Process**.
**struct Process** is very important, it contains information about the virtual processes that DCE creates, this type is described below.
**Start** then also initializes a structure of type **struct thread**, it represents the principal thread in which the **main** entry of the executable will run.
Finally **Start** asks the **TaskManager** to create a new **Task** and to start this one using the method **DceManager::DoStartProcess** as the entry point.

Class **TaskManager** is a major class of DCE, it is described below.

Process
-------

**struct process** contains everything you need to describe a process, I invite you to study the source code in the file *process.h*.

This structure contains references to standard objects for example a list of open files via a vector of FILE \*, but it contains
also the references to objects useful to manage DCE threads, the memory allocated by the process...

Field **openFiles** represents the open file descriptors of the process, the key is the fd and the value a pointer to an object of type DCE **FileUsage**.
The field **threads** contains all the threads in the process see description below.
Field **loader** is a pointer to the **Loader** used to load the corresponding code.

The **alloc** field is important it is a pointer to the memory allocator used to allocate the memory used by this process, 
at the end of the process it will liberate all the memory allocated so as simple and efficient.

Thread
------

**struct thread** represents a thread, It contains various fields including a pointer to the **process** to which it belongs, and also a pointer to **Task** object described later.

Taskmanager and Task
---------------------

The **TaskManager** manages the Tasks, ie the threads of virtualized processes by DCE.
It allows you to create new task. 
It controls the activity of the task by the following methods: **Stop**, **Wakeup**, **Sleep** and **Yield**.
A **Task** possesses a stack which contains the call stack functions.
There is one instance of **TaskManager** per node.
The implementation of **TaskManager** is based on a class of type **FiberManager** described below.

FiberManager
------------

**FiberManager** actually implements the creation of the execution contexts.
These contexts are called fiber.
**FiberManager** offers the following:

1. Create a context, it returns a fiber
2. Delete a fiber
3. Yield hand to another fiber

DCE provides two implementations:

1. **PthreadFiberManager**, which is based on the pthread library,
2. **UcontextFiberManager** which is based on the POSIX API functions offered by ucontext.h: **makecontext**, **getcontext** and **setcontext**.

I invite you to watch the corresponding man.

LoaderFactory and Loader
------------------------

The **Loader** is a very important object of DCE. A DCE **Loader** loads the executable code in memory of a special way,
load several times the same executable, while isolating each of the other executable.
The **Loader** must link the executable loaded with the 3 emulated libraries, i.e., lib C, lib pthread and lib rt.
The same way the libraries used by the executable must also be linked with the emulated libraries.


DCE offers several actually Loader:

1. **CoojaLoader**: it has the following characteristics: it loads into memory only a copy of the code, by cons it duplicates data (i.e., global variables and static). For each change of context there are 2 memory copies: backup data of the current context then restoration of context memory that will take control. Comment: it is rather reliable, the size of the copied memory size depends on the total static and global variables, and in general there is little, in a well-designed executable. 
2. **DlmLoader**: Uses a specialized loader to not duplicate the code but only the data but without special operations to do when changing context. Comment: offers the best performance in memory and cpu, but not very reliable especially during the unloading phase.

.. 2. **CopyLoader**: This is the simplest, it copies each executable and libraries before loading them in order that dlopen loads a new copy of the code and data. Comment: there is no operation during the particular context changes, but the memory consumption is important.


Follow a very simple example
============================

After theory, do a bit of practice. 
Follow the execution of very simple example.

You can find the used sample under the directory named **myscripts/sleep**.
This executable used by the scenario do only a sleep of ten seconds:

.. literalinclude:: ../../myscripts/sleep/tenseconds.c
    :language: c++


The |ns3|/DCE scenario execute **tenseconds** one time starting at time zero:

.. literalinclude:: ../../myscripts/sleep/dce-sleep.cc
    :language: c++
    :linenos:

First we can launch the binary with :command:`$ ./build/bin_dce/tenseconds`. 
After ~10 seconds you retrieve the prompt.

Then we can try the DCE scenario:
.. code-block:: sh

  $ ./build/myscripts/sleep/bin/dce-sleep

This time the test is almost instantaneous, because the scenario is very simple and it uses the simulated time. 

Same test by activating logs:

.. code-block:: console
	:caption: Log messages generated by dce-sleep

	$ NS_LOG=DefaultSimulatorImpl ./build/myscripts/sleep/bin/dce-sleep 
	DefaultSimulatorImpl:DefaultSimulatorImpl(0x6928c0)
	DefaultSimulatorImpl:SetScheduler(0x6928c0, ns3::MapScheduler[])
	0s -1 DefaultSimulatorImpl:ScheduleWithContext(0x6928c0, 0, 0, 0x692ab0)
	0s -1 DefaultSimulatorImpl:ScheduleWithContext(0x6928c0, 0, 0, 0x695220)
	0s -1 DefaultSimulatorImpl:Stop(0x6928c0, 30000000000)
	0s -1 DefaultSimulatorImpl:Schedule(0x6928c0, 30000000000, 0x692c10)
	0s -1 DefaultSimulatorImpl:Run(0x6928c0)
	0s -1 DefaultSimulatorImpl:ProcessOneEvent(): [LOGIC] handle 0
	0s 0 DefaultSimulatorImpl:Schedule(0x6928c0, 0, 0x695630)
	0s 0 DefaultSimulatorImpl:ProcessOneEvent(): [LOGIC] handle 0
	0s 0 DefaultSimulatorImpl:ProcessOneEvent(): [LOGIC] handle 0
	0s 0 DefaultSimulatorImpl:ProcessOneEvent(): [LOGIC] handle 0
	0s 0 DefaultSimulatorImpl:Schedule(0x6928c0, 10000000000, 0x6954c0)
	0s 0 DefaultSimulatorImpl:ProcessOneEvent(): [LOGIC] handle 10000000000
	10s 0 DefaultSimulatorImpl:ProcessOneEvent(): [LOGIC] handle 10000000000
	10s 0 DefaultSimulatorImpl:ProcessOneEvent(): [LOGIC] handle 30000000000
	30s -1 DefaultSimulatorImpl:Stop(0x6928c0)
	DefaultSimulatorImpl:Destroy(0x6928c0)
	DefaultSimulatorImpl:Destroy(): [LOGIC] handle destroy 0x6928a0
	DefaultSimulatorImpl:DoDispose(0x6928c0)
	DefaultSimulatorImpl:~DefaultSimulatorImpl(0x6928c0)


We can see that an event occurs at 30s it is the end of the simulation corresponding to the line:

::

   Simulator::Stop (Seconds(30.0));

We can also see that at 10s an event occurs, this is the end of our **sleep(10)**.

Now we do the same experiment using the debugger:

.. code-block:: console
	:caption: a GNU debugger session on dce-sleep binary

	$ gdb ./build/myscripts/sleep/bin/dce-sleep
	(gdb) b ns3::DceManager::DoStartProcess
	(gdb) b ns3::DceManager::Start
	(gdb) run
	Breakpoint 4, ns3::DceManager::Start (this=0x630c50, name=..., ....  at ../model/dce-manager.cc:403
	403	  NS_LOG_FUNCTION (this << name << stackSize << args.size () << envs.size ());
	(gdb) bt
	#0  ns3::DceManager::Start (this=0x630c50, name=.....) at ../model/dce-manager.cc:403
	#1  0x00007ffff7cb5e19 in ns3::DceApplication::StartApplication (this=0x633520) at ../model/dce-application.cc:79
	#2  0x00007ffff71dea6e in ns3::EventMemberImpl0::Notify (this=0x633650) at ./ns3/make-event.h:94
	#3  0x00007ffff76148af in ns3::EventImpl::Invoke (this=0x633650) at ../src/core/model/event-impl.cc:45
	#4  0x00007ffff76194c3 in ns3::DefaultSimulatorImpl::ProcessOneEvent (this=0x6308e0) at ../src/core/model/default-simulator-impl.cc:140
	#5  0x00007ffff761986a in ns3::DefaultSimulatorImpl::Run (this=0x6308e0) at ../src/core/model/default-simulator-impl.cc:193
	#6  0x00007ffff76155dd in ns3::Simulator::Run () at ../src/core/model/simulator.cc:160
	#7  0x00000000004075af in main (argc=1, argv=0x7fffffffdaa8) at ../myscripts/sleep/dce-sleep.cc:25
	(gdb) info thread
	Id   Target Id         Frame 
	* 1    Thread 0x7ffff6600740 (LWP 7977) "dce-sleep" ns3::DceManager::Start (this=0x630c50, .... ) at ../model/dce-manager.cc:403

You can notice that:

1. We have two breakpoints.
2. After run the first stop is in **ns3::DceManager:Start**.
3. At this time there is only one thread.
4. We are currently processing an event, this event was scheduled by the call **apps.Start (Seconds (0.0));** of our scenario.

Now we continue our execution:

.. code-block:: console

  (gdb) continue
  Continuing.
  [New Thread 0x7ffff65fc700 (LWP 8159)]
  [Switching to Thread 0x7ffff65fc700 (LWP 8159)]
  
  Breakpoint 3, ns3::DceManager::DoStartProcess (context=0x633d50) at ../model/dce-manager.cc:274
  274	  Thread *current = (Thread *)context;
  (gdb) info thread
    Id   Target Id         Frame 
  * 2    Thread 0x7ffff65fc700 (LWP 8159) "dce-sleep" ns3::DceManager::DoStartProcess (context=0x633d50) at ../model/dce-manager.cc:274
    1    Thread 0x7ffff6600740 (LWP 7977) "dce-sleep" pthread_cond_wait@@GLIBC_2.3.2 () at ../nptl/sysdeps/unix/sysv/linux/x86_64/pthread_cond_wait.S:162
  (gdb) bt
  #0  ns3::DceManager::DoStartProcess (context=0x633d50) at ../model/dce-manager.cc:274
  #1  0x00007ffff7d21b90 in ns3::TaskManager::Trampoline (context=0x633bd0) at ../model/task-manager.cc:267
  #2  0x00007ffff7d1da87 in ns3::PthreadFiberManager::Run (arg=0x634040) at ../model/pthread-fiber-manager.cc:402
  #3  0x00000034be206ccb in start_thread (arg=0x7ffff65fc700) at pthread_create.c:301
  #4  0x00000034bd6e0c2d in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:115

You can notice that:

1. Now there is a second thread
2. Gdb break execution in **ns3::DceManager::DoStartProcess** in the context of the second thread

This second thread is the thread corresponding to the main thread of our hosted executable **tenseconds**, 
if you look at **ns3::DceManager::DoStartProcess** you can notice that we are on the point of calling the main of **tenseconds**:

.. code-block:: c++
	:caption: Source code that calls program's *main* function
	:linenos:

	void
	DceManager::DoStartProcess (void *context)
	{
	Thread *current = (Thread *)context;
	int (*main)(int, char **) = PrepareDoStartProcess (current);
	int retval = 127;

	if (main)
		{
		StartProcessDebugHook ();
		retval = main (current->process->originalArgc, current->process->originalArgv);
		}
	dce_exit (retval);
	}

You can also see that the pointer to the **main** is the result of the method **ns3::DceManager::PrepareDoStartProcess**.
Now we can put a breakpoint before the sleep of **tenseconds** and follow the code of sleep:

.. code-block:: console

    (gdb) break tenseconds.c:5
    (gdb) continue
    Breakpoint 1, main (c=1, v=0x630b30) at ../myscripts/sleep/tenseconds.c:5
    5	  sleep (10);
    (gdb) list
    (gdb) step
    sleep () at ../model/libc-ns3.h:193
    193	DCE (sleep)
    (gdb) step
    dce_sleep (seconds=10) at ../model/dce.cc:226
    226	  Thread *current = Current ();
    (gdb) list
    224	unsigned int dce_sleep (unsigned int seconds)
    225	{
    226	  Thread *current = Current ();
    227	  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
    228	  NS_ASSERT (current != 0);
    229	  current->process->manager->Wait (Seconds (seconds));
    230	  return 0;
    231	}
    (gdb)  bt
    #0  dce_sleep (seconds=10) at ../model/dce.cc:226
    #1  0x00007ffff62cdcb9 in sleep () at ../model/libc-ns3.h:193
    #2  0x00007ffff5c36725 in main (c=1, v=0x630b30) at ../myscripts/sleep/tenseconds.c:5
    #3  0x00007ffff7c9b0bb in ns3::DceManager::DoStartProcess (context=0x633d50) at ../model/dce-manager.cc:281
    #4  0x00007ffff7d21b90 in ns3::TaskManager::Trampoline (context=0x633bd0) at ../model/task-manager.cc:267
    #5  0x00007ffff7d1da87 in ns3::PthreadFiberManager::Run (arg=0x634040) at ../model/pthread-fiber-manager.cc:402
    #6  0x00000034be206ccb in start_thread (arg=0x7ffff65fc700) at pthread_create.c:301
    #7  0x00000034bd6e0c2d in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:115
    (gdb) info thread
    Id   Target Id         Frame 
    * 2    Thread 0x7ffff65fc700 (LWP 15233) "dce-sleep" dce_sleep (seconds=10) at ../model/dce.cc:226
    1    Thread 0x7ffff6600740 (LWP 15230) "dce-sleep" pthread_cond_wait@@GLIBC_2.3.2 () at ../nptl/sysdeps/unix/sysv/linux/x86_64/pthread_cond_wait.S:162
    (gdb) 

We can notice that **sleep** call **dce_sleep** which call **Wait**, this **Wait** method is from the class **TaskManager**.
**TaskManager** is a major class of **DCE** and we will detail it below.
Basically **Wait** schedules and event in |ns3| event queue (in order to be woken up after sleep time) and give the control to another **Task**.   
Now we can put a breakpoint in **ns3::DefaultSimulatorImpl::ProcessOneEvent** and see the time advance up to 10s:

.. code-block:: console

    gdb) b ns3::DefaultSimulatorImpl::ProcessOneEvent
    Breakpoint 2 at 0x7ffff7619207: file ../src/core/model/default-simulator-impl.cc, line 131.
    (gdb) c
    Continuing.
    [Switching to Thread 0x7ffff6600740 (LWP 3942)]
    Breakpoint 2, ns3::DefaultSimulatorImpl::ProcessOneEvent (this=0x6308e0) at ../src/core/model/default-simulator-impl.cc:131
    warning: Source file is more recent than executable.
    131	  Scheduler::Event next = m_events->RemoveNext ();
    (gdb) n
    133	  NS_ASSERT (next.key.m_ts >= m_currentTs);
    (gdb) n
    134	  m_unscheduledEvents--;
    (gdb) n
    136	  NS_LOG_LOGIC ("handle " << next.key.m_ts);
    (gdb) n
    137	  m_currentTs = next.key.m_ts;
    (gdb) n
    138	  m_currentContext = next.key.m_context;
    (gdb) p m_currentTs
    $1 = 10000000000
    (gdb) 

This next event will wake the thread 2 will therefore complete the **sleep** of our scenario.

In summary we saw briefly that DCE uses the events of |ns3| to schedule the execution between different tasks.

.. TODO
.. ====

.. Explain how to implement a missing method. Show the error message then implement it ...


