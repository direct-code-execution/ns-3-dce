.. include:: replace.txt
DCE - POLL IMPLEMENTATION
*************************

Introduction
============

The implementation of the poll system call is inspired by the Linux kernel, therefore we well study first the kernel poll implementation.


Kernel implementation
---------------------
Firstly in the kernel every type of file descriptor (file, socket, pipe ...) must provide a function named poll and conform to this prototype:

.. highlight:: c++
::

  int poll(struct file *file, poll_table *pwait);
  
Where **file** is a pointer to a structure representing the file (it looks like **this** in C++) and **pwait** is a pointer to a poll table, **pwait** may be NULL, we will see later why.
The return integer of this function is a mask of poll events which have already occured on the corresponding file descriptor.
The behavior of this function is as follows:
1. It is not a blocking function, it immediately returns the mask of events regardless of event desired by the caller of poll. 
2. if **pwait** is not NULL then it add **pwait** in the wait queue of **file**, and secondly a pointer to the wait queue is stored in the poll table **pwait**.

Thus an event on the file will ascend to the poll, and in the opposite direction when the poll ends it can de-register itself from the wait queue of the file.
   
Now that we know the function **poll** of **file**, we can study the **poll** system call, here the following pseudo code commented:

.. highlight:: c++
::

  POLL( .... )
  {
     poll_table table; // This table will contain essentially the list of wait queues that need to wake me,
                       // and also information about the current thread in order to be awakened.
     poll_table *pwait=&table; // pointer to current poll table.

     while (true)
     {
        foreach( fd ) // For each file descriptor ...
        {
	       file *file = get_file(fd); // Retrieve **file** data structure corresponding to fd.

           if (!file) 
             mask = POLLNVAL; // fd does not correspond to an open file.
           else
	         mask = file->poll (file, pwait); // During the first loop pwait is not NULL.
       
           if (mask)
	       {
	         count++; // Increases the number of responses
             pwait = NULL; // Once we have at least one response POLL should not be blocking, 
                           // so we nullify the pointer to the poll table in order to not register the poll table to more file descriptors.
 	       }
        }
        pwait = NULL; // For the next loops we must not re-register to the wait queue of files.

        if (count) break; // we have a result.
        if (timeout) break; // it is too late.
      
        Wait(timeout); // Put to sleep until awakening from a file or because of the time limit.              
     }
     poll_freewait(&table); // Removes the reference to the poll table from each file's wait queue.

     return count;   
  }    

DCE implementation
------------------

As we have already seen, the poll will look like that of the kernel.
Firstly we create a virtual method named Poll in the class UnixFd.
This method will do the same job that the function **poll** seen early in the struct file of the kernel linux implementation.
Before writing the function dce_poll which is our implementation of poll we need to create some classes for mimic the role of the poll table and the wait queues.

So we add 2 sources files named wait-queue.h and wait-queue.cc in order to implements poll table and wait queue.

It is also on this occasion that I deleted all the objects used to wait which was allocated on the stack, and I replaced by objects 
allocated in the heap. 
Concerning the dce-poll function it looks like the kernel one with some differences.
The more important difference is that the PollTable cannot be allocated on the stack so it cannot be a local variable, so the PollTable
object is allocated with a C++ new. I guess you're wondering why the poll table cannot be allocated on the stack, it is because of the fork implementation of DCE.
Indeed, if a process makes a fork, this creates another stack which use the same memory addresses, thus another thread of the same process cannot use an object allocated on this stack, and when a event of a file want to wake up the poll thread it will use especially this poll table. So allocating the Poll Table in the heap generates a side effect which is that we need to release this memory if another thread call exit while we are within the dce-poll. So we need to register the Poll Table somewhere in a DCE data, and the DCE place choosen is the thread struct (in file **model/process.h**), because each thread can be in doing a poll. Thus there is a new field in struct thread which is:

.. highlight:: c++
::

	PollTable *pollTable; // No 0 if a poll is running on this thread
	
There is another reason to have this field, this reason arises from the fact that a file descriptor can be shared by multiple processes 
(thanks to dup fork ...), thus when a process exit while doing a poll, we need to deregister from the corresponding wait queues referred by the poll table.

Poll kernel implementation
--------------------------

Concerning the kernel implementation the dce-poll method is the same but the difference comes from the Poll method specialized 
implementation of the class inherited from UnixFd and which correspond to a File Descriptor open with the help of the Kernel Linux.
For example the class LinuxSocketFd  represents a socket which is opened in the kernel, therefore the method **poll** of LinuxSocketFdFactory will do much work.

Now look at the interface between DCE and the kernel, in the direction DCE to kernel, we use 2 functions which are **sock_poll** and
**sock_pollfreewait**, and in the other direction there is **sim_poll_event**.
**sock_poll** obviously has the same semantics as the kernel poll.
**sock_poll** has the following signature:
 
.. highlight:: c++
::
 
    void sock_poll (struct SimSocket *s, void *ret);
    
where **s** represents the socket int the kernel and **ret** is a pointer to a data structure of type **struct poll_table_ref**:

.. highlight:: c++
::
 
  struct poll_table_ref
  {
    int ret;
    void *opaque;
  };

This structure allows the kernel to pass a reference to the poll table DCE via the opaque field.
This reference will be used by the kernel only to warn DCE that event just happened on socket, this using the function **sim_poll_event (void \*ref)**.
In return this function modifies the value of opaque and assign it a pointer to a core structure which represents an 
entry in the wait queue of the socket. This value will be used by DCE for unregister it from the wait queue using the function 
**sock_pollfreewait function (void * ref)**.
The field **ret** is also affected in return and it contain the mask of poll events which have 
already occurred on the corresponding socket.
Most of the kernel code is in the file sim-socket.c it consists of two structures, and the following functions:

+-------------------------+--------------------------------------------------------------------------------------------------------+
| NAME                    |  DESCRIPTION                                                                                           |
+-------------------------+--------------------------------------------------------------------------------------------------------+
| sim_pollwake            |  Function called by the kernel when the arrival of an event on the socket, if the event is expected    |
|                         |  by DCE, the function forwards it to DCE.                                                              |
+-------------------------+--------------------------------------------------------------------------------------------------------+
| sim_pollwait            |  Function called by the kernel, its role is to register the poll table in the wait queue.              |
|                         |                                                                                                        |
+-------------------------+--------------------------------------------------------------------------------------------------------+
| sim_sock_poll           |  Function called by DCE, it is the interface between the DCE's poll and the kernel's poll.             |
|                         |                                                                                                        |
+-------------------------+--------------------------------------------------------------------------------------------------------+
| sim_sock_pollfreewait   |  Function called by DCE allows it to unregister from the wait queue.                                   |
|                         |                                                                                                        |
+-------------------------+--------------------------------------------------------------------------------------------------------+
| struct poll_table_ref   |  This is the same struct as that of DCE.                                                               |
|                         |                                                                                                        |
+-------------------------+--------------------------------------------------------------------------------------------------------+
| struct sim_ptable_entry |  This is used for the entry in the wait queue of the socket.                                           |
|                         |                                                                                                        |
+-------------------------+--------------------------------------------------------------------------------------------------------+

# TODO add example , gdb breakpoint to follow the behavior in live
