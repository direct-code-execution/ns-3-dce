## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import Options
import os.path
import ns3waf

def options(opt):
    opt.tool_options('compiler_cc') 
    ns3waf.options(opt)

def configure(conf):
    ns3waf.check_modules(conf, ['core', 'network', 'internet'], mandatory = True)

    conf.check_tool('compiler_cc')

    conf.env.append_value('LINKFLAGS', '-pthread')
    conf.env['HAVE_DL'] = conf.check (lib='dl')

    vg_h = conf.check(header_name='valgrind/valgrind.h', mandatory=False)
    vg_memcheck_h = conf.check(header_name='valgrind/memcheck.h', mandatory=False)
    if vg_h and vg_memcheck_h:
        conf.env.append_value('CXXDEFINES', 'HAVE_VALGRIND_H')

def build(bld):
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
    module_test = [
        'test/dce-manager-test.cc',
        'test/netlink-socket-test.cc',
        ]
    uselib = ns3waf.modules_uselib(bld, ['core', 'network', 'internet'])
    ns3waf.build_module(bld, 'dce',
                        source=module_source,
                        headers=module_headers,
                        use=uselib)
