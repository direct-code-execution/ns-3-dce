#! /usr/bin/env python
## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# A list of C++ examples to run in order to ensure that they remain
# buildable and runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run, do_valgrind_run).
#
# See test.py for more information.
cpp_examples = [
    ("dce-tcp-simple", "True", "True"),
    ("dce-udp-simple", "True", "True"), 
    ("dce-udp-perf", "True", "True"), 
    ("dce-ccnd-simple", "True", "True"), 
    ("dce-ccnd-short-stuff", "True", "True"), 
#    ("dce-tap-udp-echo", "True", "True"), 
#    ("dce-tap-ccnd", "True", "True"), 
#    ("dce-tap-vlc", "True", "True"), 
#    ("dce-bash-simple", "True", "True"), 
#    ("dce-ccn-cache", "True", "True"), 
    ("dce-linux-simple", "True", "True"), 
    ("dce-linux", "True", "True"), 
    ("dce-dccp", "True", "True"), 
    ("dce-iperf", "True", "True"), 
#    ("dce-ccnd-udp-2-nodes", "True", "True"), 
    ("dce-ccnd-linear-multiple", "True", "True"), 
    ("dce-ns3-onoff", "True", "True"),  
    ("dce-ns3-onoff --rate=10kbps --proto=icmp", "True", "True"),
    ("dce-ns3-onoff --rate=10kbps --proto=udp", "True", "True"),
    ("dce-ns3-onoff --rate=10kbps --proto=tcp", "True", "True"),
    ("dce-ns3-onoff --rate=10kbps --proto=dccp", "True", "True"),
    ("dce-ns3-onoff --rate=10kbps --proto=dccp -ccid=3", "True", "True"),
    ("dce-tcp-ns3-nsc-comparison", "True", "True"), 
    ("dce-tcp-ns3-nsc-comparison --stack=dce", "True", "True"),
    ("dce-dccp", "True", "True"),
    ("dce-ping-mt1 --kernel=1", "True", "True"),
    ("dce-mt2 --kernel=1", "True", "True"),
    ("dce-mt3 --kernel=1", "True", "True"),
    ("dce-xfrm", "True", "True"),
    ("dce-ltp", "True", "True"),
]

# A list of Python examples to run in order to ensure that they remain
# runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run).
#
# See test.py for more information.
python_examples = []
