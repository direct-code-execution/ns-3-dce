## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import ns3waf
import os

def configure(conf):
    ns3waf.check_modules(conf, ['core', 'internet', 'point-to-point', 'netanim'], mandatory = True)

def build(bld):
#    linkerpath=os.path.abspath(os.path.join('..', 'elf-loader','ldso'))
    bld.build_a_script('dce', needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim' ],
				  target='bin/dce-ping',
				  source=['dce-ping.cc', 'misc-tools.cc'],
#				  linkflags=['-Wl,--dynamic-linker=' + linkerpath ]
#				  linkflags=['-L/usr/local/lib'],
#				  lib=['foolib']
				  )

