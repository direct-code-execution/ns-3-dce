.. include:: replace.txt
Aspect-based Tracing
====================

Aspect-based tracing, provided by libaspect, allows us to use tracing facility with unmodified code.

One of contradictions when we use DCE is, tracing, how to put trace sources into unmodified code. While DCE gives an opportunity to use unmodified codes as simulation protocols, one might want to investigate which function is called or how many messages of a particular protocol are exchanged.

|ns3| originally has a nice feature of tracing with such a purpose, with on-demand trace connector to obtain additional information.
Instead of inserting TraceSource into the original code, DCE gives dynamic trace points with this library, based on the idea of aspect-based tracing.

For more detail, see the Chapter 6.3.2 of the `thesis <http://cutebugs.net/files/thesis.pdf>`_.

Quick Start
-----------
To put trace sources without modifying the original code, aspcpp::HookManager gives trace hooks into arbitrary source codes and functions.

::

  #include <hook-manager.h>

  HookManager hooks;
  hooks.AddHookBySourceAndFunction ("ip_input.c", "::ip_rcv", &IpRcv);
  hooks.AddHookByFunction ("::process_backlog", &ProcBacklog);
  hooks.AddHookByFunction ("::arp_xmit", &ArpXmit);

The above examples specifies file name and functions with callback functions in the simulation script.

Limitations
-----------
* July 10th, 2013: aspect-based tracing (libaspect) is in the alpha release state. It might be updated frequently.
* Callback function has no argument that it can investigate the contents of buffer that each function handles.
