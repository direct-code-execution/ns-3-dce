## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os

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
    bld.add_subdirs(['model', 'helper', 'test', 'vdl', 'example', 'utils'])
