.. include:: replace.txt

User's Guide
************
This document is for the people who want to use your application in |ns3| using DCE.

Direct Code Execution (DCE) allows us to use POSIX socket-based applications as well as Linux kernel network stack.

- Supported Platform Status
- Setup Guide
- Basic Use Cases
 - How to start to use DCE?
 - Userspace application over |ns3|
 - In-kernel protocol implementation over |ns3|
- Advanced Use Cases
 - Distributed Debugging (of Protocol Implementation)
 - Testing
- Technical Information


.. TODO
.. - what should I do if my apps doesn't run on DCE ?
.. -- how can I extend DCE ?


.. toctree::
   :maxdepth: 2

   dce-user-install
   dce-user-simulation
   dce-user-exec
   dce-user-debug
   dce-user-test
   dce-user-tech

