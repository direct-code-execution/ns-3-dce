## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os

import Options
import os.path

class CouldNotFindFile:
    pass

def search_file(files):
    for f in files:
        if os.path.isfile (f):
            return f
    raise CouldNotFindFile ()




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
    opt.tool_options('compiler_cc') 
    opt.add_option('--with-linux-stack',
                   help=('Path to the linux module'),
                   default=None,
                   dest='linux_stack', type="string")

def configure(conf):
    add_build_dir(conf, 'model')
    add_build_dir(conf, 'vdl')
    add_build_dir(conf, 'test')
    add_build_dir(conf, 'example')

    conf.check_tool('compiler_cc')
    conf.check(function_name='makecontext', 
               header_name='ucontext.h',
               mandatory=True)
    conf.env.append_value('LINKFLAGS', '-pthread')
    conf.env['HAVE_DL'] = conf.check (lib='dl')

    if Options.options.linux_stack is not None and os.path.isdir(Options.options.linux_stack):
        conf.check_message("linux stack location", '', True, 
                           ("%s (given)" % Options.options.linux_stack))
        conf.report_optional_feature("linux stack", "Linux stack", True,
                                     "enabled (found in %s)" % Options.options.linux_stack)
        conf.env['LINUX_STACK'] = os.path.abspath(Options.options.linux_stack)
        conf.env.append_value('NS3_MODULE_PATH', conf.env['LINUX_STACK'])
    else:
        conf.check_message("linux stack location", '', False)
        conf.report_optional_feature("linux stack", "Linux stack", False,
                                     "linux stack not found")

    vg_h = conf.check(header_name='valgrind/valgrind.h')
    vg_memcheck_h = conf.check(header_name='valgrind/memcheck.h')
    if vg_h and vg_memcheck_h:
        conf.env.append_value('CXXDEFINES', 'HAVE_VALGRIND_H')

    conf.sub_config('vdl')

def build(bld):
    import types
    bld.create_dce_program = types.MethodType(create_dce_program, bld)
    # beeeurk: most deps below are un-needed.
    dce = bld.create_ns3_module('dce', ['core', 'network', 'internet', 'point-to-point'])
    dce.source = [
        'model/dce-manager.cc',
	'model/dce-application.cc',
        'model/dce.cc',
        'model/dce-signal.cc',
        'model/libc-dce.c',
        'model/utils.cc',
        'model/unix-fd.cc',
        'model/unix-file-fd.cc',
        'model/unix-socket-fd.cc',
        'model/unix-datagram-socket-fd.cc',
        'model/unix-stream-socket-fd.cc',
        'model/unix-timer-fd.cc',
        'model/dce-fd.cc',
        'model/dce-stdio.cc',
        'model/dce-pthread.cc',
        'model/dce-stdlib.cc',
        'model/dce-debug.cc',
        'model/dce-semaphore.cc',
        'model/dce-pthread-mutex.cc',
        'model/dce-cxa.cc',
        'model/dce-netdb.cc',
        'model/dce-string.cc',
        'model/dce-env.cc',
        'model/dce-pthread-cond.cc',
        'model/dce-timerfd.cc',
        'model/dce-stat.cc',
        'model/dce-global-variables.cc',
        'model/cmsg.cc',
        'model/waiter.cc',
        'model/kingsley-alloc.cc',
        'model/dce-alloc.cc',
        'model/fiber-manager.cc',
        'model/ucontext-fiber-manager.cc',
        'model/pthread-fiber-manager.cc',
        'model/task-manager.cc',
        'model/task-scheduler.cc',
        'model/rr-task-scheduler.cc',
        'model/loader-factory.cc',
        'model/elf-dependencies.cc',
        'model/elf-cache.cc',
        'model/cooja-loader-factory.cc',
        'model/copy-loader-factory.cc',
        'model/dlm-loader-factory.cc',
        'model/socket-fd-factory.cc',
        'model/ns3-socket-fd-factory.cc',
        # netlink code
        'model/netlink-socket.cc',
        'model/netlink-socket-address.cc',
        'model/netlink-socket-factory.cc',
        'model/netlink-attribute.cc',
        'model/netlink-message.cc',
        'model/netlink-message-route.cc',
        # helper.
        'helper/dce-manager-helper.cc',
        'helper/dce-application-helper.cc',
        ]
    dce.uselib = 'DL'

    headers = bld.new_task_gen('ns3header')
    headers.module = 'dce'
    headers.source = [
        'model/dce-manager.h',
        'model/task-scheduler.h',
        'model/task-manager.h',
        'model/socket-fd-factory.h',
        'model/loader-factory.h',
	'model/dce-application.h',
        'helper/dce-manager-helper.h',
        'helper/dce-application-helper.h',
# needed only for test module.
        'model/netlink-message.h',
        'model/netlink-message-route.h',
        'model/netlink-attribute.h',
        'model/netlink-socket-address.h',
        ]

    if dce.env['LINUX_STACK']:
        dce.source.extend([
                'model/linux-socket-fd-factory.cc',
                'model/linux-socket-fd.cc',
                ])
        dce.includes = dce.env['LINUX_STACK']
        headers.source.extend ([
                'model/linux-socket-fd-factory.h'])


    rvd = bld.new_task_gen('cc', 'program')
    rvd.name = 'readversiondef'
    rvd.target = 'readversiondef'
    rvd.source = ['model/readversiondef.c']

    bld.add_group('dce_version_files')

    libc = search_file ([
            '/lib64/libc.so.6',
            '/lib/libc.so.6',
            ])
    libpthread = search_file ([
            '/lib64/libpthread.so.0',
            '/lib/libpthread.so.0',
            ])

    bld.new_task_gen(source=['readversiondef', 'model/libc-ns3.version'],
                     target='libc.version',
                     rule='${SRC[0].abspath(env)} ' + libc + ' |' \
                         'cat ${SRC[1].abspath()} - > ${TGT}')

    bld.new_task_gen(source=['readversiondef', 'model/libpthread-ns3.version'],
                     target='libpthread.version',
                     rule='${SRC[0].abspath(env)} ' + libpthread + ' |' \
                         'cat ${SRC[1].abspath()} - > ${TGT}')

    bld.add_group('dce_use_version_files')

    # The very small libc used to replace the glibc
    # and forward to the dce_* code
    module = bld.new_task_gen('cc', 'shlib')
    module.name = 'c-ns3'
    module.target = module.name
    module.env.append_value('CCFLAGS', module.env['shlib_CCFLAGS'])
    module.env.append_value('CCFLAGS', '-g')
    module.env.append_value('CCDEFINES', 'LIBSETUP=libc_setup')
    module.env.append_value('LINKFLAGS', '-nostdlib')
    module.env.append_value('LINKFLAGS', 
                            '-Wl,--version-script=' + bld.path.find_or_declare('libc.version').bldpath(bld.env))
    module.env.append_value('LINKFLAGS', '-Wl,-soname=libc.so.6')
    module.source = ['model/libc.c', 'model/libc-global-variables.c']

    # The very small libpthread used to replace the glibc
    # and forward to the dce_* code
    module = bld.new_task_gen('cc', 'shlib')
    module.name = 'pthread-ns3'
    module.target = module.name
    module.env.append_value('CCFLAGS', module.env['shlib_CCFLAGS'])
    module.env.append_value('CCFLAGS', '-g')
    module.env.append_value('CCDEFINES', 'LIBSETUP=libpthread_setup')
    module.env.append_value('LINKFLAGS', '-nostdlib')
    module.env.append_value('LINKFLAGS', '-lc')
    module.env.append_value('LINKFLAGS', 
                            '-Wl,--version-script=' + bld.path.find_or_declare('libpthread.version').bldpath(bld.env))
    module.env.append_value('LINKFLAGS', '-Wl,-soname=libpthread.so.0')
    module.source = ['model/libc.c']

    # The following is needed to debug loader errors on ubuntu when you install libc6-dbg.
    #obj.env.append_value('LINKFLAGS', '-Wl,--dynamic-linker=/usr/lib/debug/ld-linux.so.2')

    bld.set_group(0)

    dce_test = bld.create_ns3_module_test_library('dce')
    dce_test.source = [
        'test/dce-manager-test.cc',
        'test/netlink-socket-test.cc',
        ]

    if (bld.env['ENABLE_EXAMPLES']):
        bld.add_subdirs(['example'])

    bld.add_subdirs(['vdl', 'test', 'utils'])
