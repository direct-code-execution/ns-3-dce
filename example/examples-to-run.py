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
    ("dce-bash-simple", "True", "True"), 
#    ("dce-ccn-cache", "True", "True"), 
    ("dce-linux-simple", "True", "True"), 
    ("dce-linux", "True", "True"), 
    ("dce-dccp", "True", "True"), 
    ("dce-iperf", "True", "True"), 
#    ("dce-ccnd-udp-2-nodes", "True", "True"), 
#    ("dce-ccnd-linear-multiple", "True", "True"),
    ("dce-cradle-simple", "True", "True"),  
    ("dce-cradle-simple --rate=10kbps --proto=icmp", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=udp", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=tcp", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=dccp", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=dccp -ccid=3", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=sctp", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=tcp", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=dccp", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=dccp -ccid=3", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=sctp", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=icmp6", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=udp6", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=tcp6", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=dccp6", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=dccp6 -ccid=3", "True", "True"),
    ("dce-cradle-simple --rate=10kbps --proto=sctp6", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=tcp6", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=dccp6", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=dccp6 -ccid=3", "True", "True"),
    ("dce-cradle-simple --bulk=1 --rate=10kbps --proto=sctp6", "True", "True"),
    ("dce-tcp-ns3-nsc-comparison", "True", "True"), 
    ("dce-tcp-ns3-nsc-comparison --stack=dce", "True", "True"),
    ("dce-tcp-ns3-nsc-comparison --bulk=1", "True", "True"), 
    ("dce-tcp-ns3-nsc-comparison --stack=dce --bulk=1", "True", "True"),
    ("dce-tcp-ns3-nsc-comparison --stack=dce-dccp", "True", "True"),
    ("dce-tcp-ns3-nsc-comparison --stack=dce-dccp --bulk=1", "True", "True"),
    ("dce-ping-mt1 --kernel=1", "True", "True"),
    ("dce-mt2 --kernel=1", "True", "True"),
    ("dce-mt3 --kernel=1", "True", "True"),
    ("dce-xfrm", "False", "False"),
#    ("dce-ltp", "True", "True"),
    ("dce-cradle-mptcp", "True", "True"),
    ("dce-iperf-mptcp", "True", "True"),
    ("dce-iperf-heterogeneous-multihop", "True", "True"),
    ("dce-mptcp-handoff-v6", "True", "True"),
    ("dce-mptcp-handoff-v6 --errRate=0.05", "True", "True"),
    ("dce-mptcp-handoff-v6 --errRate=0.1", "True", "True"),
    ("dce-mptcp-handoff-v6 --errRate=0.2", "True", "True"),
    ("dce-mptcp-handoff-v6 --errRate=0.3", "True", "True"),
    ("dce-mptcp-handoff-v6 --errRate=0.4", "True", "True"),
    ("dce-mptcp-handoff-v6 --errRate=0.5", "True", "True"),
    ("dce-mptcp-handoff-v6 --errRate=0.8", "True", "True"),
    ("dce-mptcp-handoff-v4v6", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --errRate=0.05", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --errRate=0.1", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --errRate=0.2", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --errRate=0.3", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --errRate=0.4", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --errRate=0.5", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --errRate=0.8", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1 --errRate=0.05", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1 --errRate=0.1", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1 --errRate=0.2", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1 --errRate=0.3", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1 --errRate=0.4", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1 --errRate=0.5", "True", "True"),
    ("dce-mptcp-handoff-v4v6 --v6Primary=1 --errRate=0.8", "True", "True"),
#    ("dce-mptcp-lte-wifi", "True", "True"),
    ("dce-httpd", "True", "True"),
#    ("dce-wifi-ccnx", "True", "True"),
    ("dce-sctp-simple", "True", "True"),
    ("dce-freebsd", "True", "True"),
]

# A list of Python examples to run in order to ensure that they remain
# runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run).
#
# See test.py for more information.
python_examples = []
