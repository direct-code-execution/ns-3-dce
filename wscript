## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os

def create_dce_program(bld, name, is_c_only = False):
    if not is_c_only:
        program = bld.new_task_gen('cxx', 'program')
        var = 'CXXFLAGS'
    else:
        program = bld.new_task_gen('cc', 'program')
        var = 'CCFLAGS'
    program.env.append_value(var, '-fpie')
    program.env.append_value(var, '-U_FORTIFY_SOURCE')
    if os.uname()[4] == 'x86_64':
        program.env.append_value(var, '-mcmodel=large')
#    program.env.append_value('LINKFLAGS', '-Wl,--dynamic-linker=/usr/lib/debug/ld-linux-x86-64.so.2')
    program.is_ns3_program = True
    program.name = name
    program.target = name
    program.env.append_value('LINKFLAGS', '-pie')
    return program

def add_build_dir(conf,d):
    build_dir = os.path.abspath(os.path.join(conf.blddir, conf.env.variant(), 'src', 'dce', d))
    conf.env.append_value('NS3_MODULE_PATH', build_dir)


def set_options(opt):
    opt.sub_options('model')

def configure(conf):
    add_build_dir(conf, 'model')
    add_build_dir(conf, 'vdl')
    add_build_dir(conf, 'test')
    add_build_dir(conf, 'example')
    conf.sub_config('model')
    conf.sub_config('vdl')

def build(bld):
    import types
    bld.create_dce_program = types.MethodType(create_dce_program, bld)
    bld.add_subdirs(['model', 'helper', 'test', 'vdl', 'example', 'utils'])
