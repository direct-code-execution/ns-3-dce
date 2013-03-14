.. include:: replace.txt
Installation
************

In order to install DCE you must follow the tutorial :ref:`build-dce`.


Installation result
-------------------

The result of the installation process is the creation of libraries from source of DCE and that of |ns3| and also some tools and sources of an optional Linux kernel if you have also chosen to use the stack of a Linux kernel. 
Below you will find the main directories:

::

  ├── build                Target of |ns3| Core and DCE compilation
  │   ├── bin
  │   ├── include
  │   └── lib
  │
  ├── ns-3-dce             DCE main tree 
  │   ├── build            Target of DCE compilation
  │   │   ├── bin
  │   │   ├── bin_dce
  │   │   ├── lib
  │   ├── doc              Documentation source
  │   ├── example          Some exemple of scenarios using DCE
  │   │   ├── bash
  │   │   └── ccnx
  │   ├── helper           Source code of DCE
  │   ├── model            Source code of DCE
  │   ├── netlink          Source code of DCE
  │   ├── ns3waf          
  │   ├── test             Source code of DCE unit tests
  │   └── utils            Utility scripts for DCE build and use
  │
  ├── ns-3-dev             |ns3| CORE Main tree
  │   ├── bindings         |ns3| Core python bindings
  │   ├── build            Target of |ns3| Core compilation
  │   ├── doc              Documentation source
  │   ├── examples         Many exemple of  scenarios using |ns3|
  │   ├── ns3              empty directory
  │   ├── scratch          Scratch directory usable to write scripts
  │   ├── src              |ns3| source code
  │   ├── utils            Utilities
  │   └── waf-tools        "Makefile" tool like
  │
  └── readversiondef       Little utility used by DCE to create dynamic libraries version tags. Used only during compilation of DCE.
  
