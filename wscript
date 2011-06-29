## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import Options
import os.path
import ns3waf
import sys


def options(opt):
    opt.tool_options('compiler_cc') 
    ns3waf.options(opt)
    opt.add_option('--enable-kernel-stack',
                   help=('Path to the prefix where the kernel wrapper headers are installed'),
                   default=None,
                   dest='kernel_stack', type="string")

def search_file(files):
    for f in files:
        if os.path.isfile (f):
            return f
    return None

def configure(conf):
    ns3waf.check_modules(conf, ['core', 'network', 'internet'], mandatory = True)
    ns3waf.check_modules(conf, ['point-to-point', 'file-descriptor'], mandatory = False)

    conf.check_tool('compiler_cc')
    conf.check(header_name='stdint.h', define_name='HAVE_STDINT_H', mandatory=False)
    conf.check(header_name='inttypes.h', define_name='HAVE_INTTYPES_H', mandatory=False)
    conf.check(header_name='sys/inttypes.h', define_name='HAVE_SYS_INT_TYPES_H', mandatory=False)
    conf.check(header_name='sys/types.h', define_name='HAVE_SYS_TYPES_H', mandatory=False)
    conf.check(header_name='sys/stat.h', define_name='HAVE_SYS_STAT_H', mandatory=False)
    conf.check(header_name='dirent.h', define_name='HAVE_DIRENT_H', mandatory=False)

    conf.env.append_value('LINKFLAGS', '-pthread')
    conf.check (lib='dl', mandatory = True)

    vg_h = conf.check(header_name='valgrind/valgrind.h', mandatory=False)
    vg_memcheck_h = conf.check(header_name='valgrind/memcheck.h', mandatory=False)
    if vg_h and vg_memcheck_h:
        conf.env.append_value('CXXDEFINES', 'HAVE_VALGRIND_H')

    conf.start_msg('Searching C library')
    libc = search_file ([
            '/lib64/libc.so.6',
            '/lib/libc.so.6',
            ])
    if libc is None:
        conf.fatal('not found')
    else:
        conf.end_msg(libc, True)
    conf.env['LIBC_FILE'] = libc

    conf.start_msg('Searching pthread library')
    libpthread = search_file ([
            '/lib64/libpthread.so.0',
            '/lib/libpthread.so.0',
            ])
    if libpthread is None:
        conf.fatal('not found')
    else:
        conf.end_msg(libpthread, True)
    conf.env['LIBPTHREAD_FILE'] = libpthread

    conf.find_program('readversiondef', var='READVERSIONDEF', mandatory=True)

    if Options.options.kernel_stack is not None and os.path.isdir(Options.options.kernel_stack):
        conf.check(header_name='sim.h',
                   includes=os.path.join(Options.options.kernel_stack, 'sim/include'))
      #  conf.check()
        conf.env['KERNEL_STACK'] = Options.options.kernel_stack

    ns3waf.print_feature_summary(conf)

def build_netlink(bld):
    module_source = [
        'netlink/netlink-socket.cc',
        'netlink/netlink-socket-address.cc',
        'netlink/netlink-socket-factory.cc',
        'netlink/netlink-attribute.cc',
        'netlink/netlink-message.cc',
        'netlink/netlink-message-route.cc',
        ]
    module_headers = [
        'netlink/netlink-socket-factory.h'
        ]
    module = ns3waf.create_module(bld, 
                                  name='netlink',
                                  needed = ['internet', 'core'],
                                  source=module_source,
                                  headers=module_headers)

    module_tests = [
        'test/netlink-socket-test.cc',
        ]
    module.add_runner_test(needed = ['internet', 'point-to-point', 'core', 'netlink', 'dce'],
                           source=module_tests)

def dce_kw(**kw):
    d = dict(**kw)
    if os.uname()[4] == 'x86_64':
        mcmodel = ['-mcmodel=large']
    else:
        mcmodel = []
    nofortify = ['-U_FORTIFY_SOURCE']
    #debug_dl = ['-Wl,--dynamic-linker=/usr/lib/debug/ld-linux-x86-64.so.2']
    debug_dl = []
    d['cxxflags'] = d.get('cxxflags', []) + ['-fpie'] + mcmodel + nofortify
    d['cflags'] = d.get('cflags', []) + ['-fpie'] + mcmodel + nofortify
    d['linkflags'] = d.get('linkflags', []) + ['-pie'] + debug_dl
    return d

def build_dce_tests(module):
    module.add_runner_test(needed=['core', 'dce', 'internet'], source=['test/dce-manager-test.cc'])
    module.add_test(features='cxx cxxshlib', source=['test/test-macros.cc'], 
                    target='lib/test', linkflags=['-Wl,-soname=libtest.so'])

    tests = [['test-empty', []],
             ['test-sleep', []],
             ['test-nanosleep', []],
             ['test-pthread', ['PTHREAD']],
             ['test-mutex', ['PTHREAD']],
             ['test-once', ['PTHREAD']],
             ['test-pthread-key', ['PTHREAD']],
             ['test-sem', ['PTHREAD']],
             ['test-malloc', []],
             ['test-malloc-2', []],
             ['test-fd-simple', []],
             ['test-strerror', []],
             ['test-stdio', []],
             ['test-string', []],
             ['test-netdb', []],
             ['test-env', []],
             ['test-cond', ['PTHREAD']],
             ['test-timer-fd', []],
             ['test-stdlib', []],
             ['test-select', ['PTHREAD']],
             ['test-random', []],
             ['test-ioctl', []],
             ['test-fork', []],
             ['test-local-socket', ['PTHREAD']],
             ['test-poll', ['PTHREAD']],
             ['test-tcp-socket', ['PTHREAD']],
             ]
    for name,uselib in tests:
        module.add_test(**dce_kw(target='bin/' + name, source = ['test/' + name + '.cc'],
                                 use = uselib + ['lib/test']))

def build_dce_examples(module):
    dce_examples = [['udp-server', []],
                    ['udp-client', []],
                    ['udp-perf', ['m']],
                    ['tcp-server', []],
                    ['tcp-client', []],
                    ['unix-server', []],
                    ['unix-client', []],
                    ['udp-echo-server', []],
                    ['udp-echo-client', []],
                    ]
    for name,lib in dce_examples:
        module.add_example(**dce_kw(target = 'bin/' + name, 
                                    source = ['example/' + name + '.cc'],
                                    lib = lib))

    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-tcp-simple',
                       source=['example/dce-tcp-simple.cc'])

    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-udp-simple',
                       source=['example/dce-udp-simple.cc'])
    
    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point'], 
                       target='bin/dce-udp-perf',
                       source=['example/dce-udp-perf.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point'], 
                       target='bin/dce-ccnd-udp-2-nodes',
                       source=['example/ccnx/dce-ccnd-udp-2-nodes.cc'])

    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-ccnd-simple',
                       source=['example/ccnx/dce-ccnd-simple.cc'])
                       
    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-ccnd-short-stuff',
                       source=['example/ccnx/dce-ccnd-short-stuff.cc'])
                       
    module.add_example(needed = ['core', 'internet', 'dce', 'file-descriptor'],
                       target='bin/dce-tap-udp-echo',
                       source=['example/ccnx/dce-tap-udp-echo.cc'])                

    module.add_example(needed = ['core', 'internet', 'dce', 'file-descriptor'],
                       target='bin/dce-tap-ccnd',
                       source=['example/ccnx/dce-tap-ccnd.cc'])                

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point'], 
                       target='bin/dce-ccnd-linear-multiple',
                       source=['example/ccnx/dce-ccnd-linear-multiple.cc'])
                       
def build_dce_kernel_examples(module):
    module.add_example(needed = ['core', 'network', 'dce'], 
                       target='bin/dce-linux-simple',
                       source=['example/dce-linux-simple.cc'])

    module.add_example(needed = ['core', 'network', 'dce'], 
                       target='bin/dce-linux-ccnd',
                       source=['example/dce-linux-ccnd.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'wifi', 
                                 'point-to-point', 'csma', 'mobility'],
                       target='bin/dce-linux',
                       source=['example/dce-linux.cc'])


def build(bld):
    build_netlink(bld)

    if bld.env['KERNEL_STACK']:
        kernel_source = [
            'model/linux-socket-fd-factory.cc',
            'model/linux-socket-fd.cc',
            ]
        kernel_headers = [ 'model/linux-socket-fd-factory.h']
        kernel_includes = [bld.env['KERNEL_STACK']]
    else:
        kernel_source = []
        kernel_headers = []
        kernel_includes = []

    module_source = [
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
        'model/local-socket-fd.cc',
        'model/local-stream-socket-fd.cc',
        'model/local-datagram-socket-fd.cc',
        'model/local-socket-fd-factory.cc',
        'model/dce-umask.cc',
        'model/testdce.cc',
        'model/system-path.cc',
        # helper.
        'helper/dce-manager-helper.cc',
        'helper/dce-application-helper.cc',
        ]
    module_headers = [
        'model/dce-manager.h',
        'model/task-scheduler.h',
        'model/task-manager.h',
        'model/socket-fd-factory.h',
        'model/loader-factory.h',
	'model/dce-application.h',
#        'model/testdce.h',
        'helper/dce-manager-helper.h',
        'helper/dce-application-helper.h',
        ]
    module_source = module_source + kernel_source
    module_headers = module_headers + kernel_headers
    uselib = ns3waf.modules_uselib(bld, ['core', 'network', 'internet', 'netlink'])
    module = ns3waf.create_module(bld, name='dce',
                                  source=module_source,
                                  headers=module_headers,
                                  use=uselib,
                                  includes=kernel_includes,
                                  lib=['dl'])
    build_dce_tests(module)
    build_dce_examples(module)

    if bld.env['KERNEL_STACK']:
        build_dce_kernel_examples(module)

    bld.add_group('dce_version_files')

    bld(source=['model/libc-ns3.version'],
        target='model/libc.version',
        rule='%s %s | cat ${SRC[0].abspath()} - > ${TGT}' %
        (bld.env['READVERSIONDEF'], bld.env['LIBC_FILE']))

    bld(source=['model/libpthread-ns3.version'],
        target='model/libpthread.version',
        rule='%s %s | cat ${SRC[0].abspath()} - > ${TGT}' %
        (bld.env['READVERSIONDEF'], bld.env['LIBPTHREAD_FILE']))

    bld.add_group('dce_use_version_files')

    # The very small libc used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.c', 'model/libc-global-variables.c'],
              target='lib/c-ns3', cflags=['-g'],
              defines=['LIBSETUP=libc_setup'],
              linkflags=['-nostdlib', 
                         '-Wl,--version-script=' + os.path.join('model', 'libc.version'),
                         '-Wl,-soname=libc.so.6'])

    # The very small libpthread used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.c'],
              target='lib/pthread-ns3', cflags=['-g'],
              defines=['LIBSETUP=libpthread_setup'],
              linkflags=['-nostdlib', '-lc',
                         '-Wl,--version-script=' + os.path.join('model', 'libpthread.version'),
                         '-Wl,-soname=libpthread.so.0'])
