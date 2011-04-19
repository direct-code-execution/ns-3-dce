## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import Options
import os.path
import ns3waf

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

    conf.check_tool('compiler_cc')

    conf.env.append_value('LINKFLAGS', '-pthread')
    conf.env['HAVE_DL'] = conf.check (lib='dl')

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

    if Options.options.kernel_stack is not None and os.path.isdir(Options.options.kernel_stack):
        conf.check(header_name='sim/sim.h',
                   includes=os.path.join(Options.options.kernel_stack, 'include'))
        conf.check()
        conf.env['KERNEL_STACK'] = Options.options.kernel_stack

def build(bld):
    if bld.env['KERNEL_STACK']:
        kernel_source = [
            'linux-socket-fd-factory.cc',
            'linux-socket-fd.cc',
            ]
        kernel_headers = [ 'linux-socket-fd-factory.h']
        kernel_includes = [os.path.join(bld.env['KERNEL_STACK'], 'include')]
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
    module_headers = [
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
    module_source = module_source + kernel_source
    module_headers = module_headers + kernel_headers
    module_test = [
        'test/dce-manager-test.cc',
        'test/netlink-socket-test.cc',
        ]
    uselib = ns3waf.modules_uselib(bld, ['core', 'network', 'internet'])
    ns3waf.build_module(bld, name='dce',
                        source=module_source,
                        headers=module_headers,
                        use=uselib,
                        includes=kernel_includes)

    bld.add_group('dce_version_files')

    bld(source=['model/libc-ns3.version'],
            target='model/libc.version',
            rule='readversiondef ' + bld.env['LIBC_FILE'] + ' |' \
                'cat ${SRC[0].abspath()} - > ${TGT}')

    bld(source=['model/libpthread-ns3.version'],
            target='model/libpthread.version',
            rule='readversiondef ' + bld.env['LIBPTHREAD_FILE'] + ' |' \
                'cat ${SRC[0].abspath()} - > ${TGT}')

    bld.add_group('dce_use_version_files')

    # The very small libc used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.c', 'model/libc-global-variables.c'],
              target='c-ns3', cflags=['-g'],
              defines=['LIBSETUP=libc_setup'],
              linkflags=['-nostdlib', 
                         '-Wl,--version-script=' + os.path.join('model', 'libc.version'),
                         '-Wl,-soname=libc.so.6'])

    # The very small libpthread used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.c'],
              target='pthread-ns3', cflags=['-g'],
              defines=['LIBSETUP=libpthread_setup'],
              linkflags=['-nostdlib', '-lc',
                         '-Wl,--version-script=' + os.path.join('model', 'libpthread.version'),
                         '-Wl,-soname=libpthread.so.0'])
