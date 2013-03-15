.. include:: replace.txt
Using Alternative, Fast Loader
==============================
DCE optionally supports an alternative ELF loader/linker, so-called elf-loader, in order to replace system-provided linker/loader module. The intention of the loader is to support unlimited number of instances used by **dlmopen** call, which provides DCE to load a single ELF binary to multiple different memory space. dlmopen-based loader (ns3::DlmLoaderFactory) is much faster than another default one (ns3::CoojaLOaderFactory), but few issues are remain so, this is optional.

To Speedup Run-time
-------------------
In order to use DlmLoaderFactory, you can add command-line argument of waf.

::

  ./waf --run dce-tcp-simple --dlm


if you are in the *./waf shell* mode, the following command should be used instead.

::

  ./build/bin/dce-runner ./build/bin/dce-tcp-simple

