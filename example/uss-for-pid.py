#!/usr/bin/env python

import subprocess
import sys
import re

pid = sys.argv[1]

smem = subprocess.Popen (['smem', '-c', 'pid uss'],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT,
                         stdin=subprocess.PIPE)
(out, err) = smem.communicate ('')

reg = re.compile('([0-9]+)[^0-9]+([0-9]+)')
for line in out.split('\n'):
    m = reg.search (line)
    if m is not None:
        if m.group(1) == pid:
            print m.group(2)
            sys.exit (0)
