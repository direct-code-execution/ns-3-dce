.. include:: replace.txt
Technical Information
=====================

.. toctree::
   :maxdepth: 1

   dce-user-internal

Limitations
-----------

- Currently the POSIX API (libc) is not fully supported by DCE. However there are already about **400 methods** supported. As the goal of DCE is to allow to execute network applications, many methods related to the network are supported for example ``socket``, ``connect``,  ``bind``, ``listen``, ``read``, ``write``, ``poll``, ``select``. The next chapter list the applications well tested using DCE. 
- Some methods are not usable with all options of DCE. For more details refer to chapter **Coverage API** that lists all the supported methods.
- The scheduler is not as advanced as that of a kernel, for example if an infinite loop in a hosted application, DCE can not get out, but this should not happen in applications written correctly.


API Coverage
------------
Below there is the list of the systems calls supported by DCE, the column named **Type** represents how the system call is implemented ie:

1. **DCE** the method is fully rewritten, 
2. **NATIVE** the real corresponding system call is used.

.. csv-table:: API Coverage
   :header: "System Call Name","Domain","Include file","Type","Remarks"
   :widths: 50,10,10,7,23
   :file: api_cov.csv

