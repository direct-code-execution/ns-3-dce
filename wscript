## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import Options
import os.path
import ns3waf
import sys
import types

def options(opt):
    opt.tool_options('compiler_cc') 
    ns3waf.options(opt)
    opt.add_option('--enable-kernel-stack',
                   help=('Path to the prefix where the kernel wrapper headers are installed'),
                   default=None,
                   dest='kernel_stack', type="string")
    opt.add_option('--enable-vdl-loader',
                   help=('Enable the build of dce-runner.'),
                   dest='enable_vdl_loader', action='store_true',
                   default=False)               
	
def search_file(files):
    for f in files:
        if os.path.isfile (f):
            return f
    return None

def configure(conf):
    ns3waf.check_modules(conf, ['core', 'network', 'internet'], mandatory = True)
    ns3waf.check_modules(conf, ['point-to-point', 'tap-bridge', 'netanim'], mandatory = False)
    ns3waf.check_modules(conf, ['wifi', 'point-to-point', 'csma', 'mobility'], mandatory = False)
    ns3waf.check_modules(conf, ['point-to-point-layout'], mandatory = False)
    conf.check_tool('compiler_cc')
    conf.check(header_name='stdint.h', define_name='HAVE_STDINT_H', mandatory=False)
    conf.check(header_name='inttypes.h', define_name='HAVE_INTTYPES_H', mandatory=False)
    conf.check(header_name='sys/inttypes.h', define_name='HAVE_SYS_INT_TYPES_H', mandatory=False)
    conf.check(header_name='sys/types.h', define_name='HAVE_SYS_TYPES_H', mandatory=False)
    conf.check(header_name='sys/stat.h', define_name='HAVE_SYS_STAT_H', mandatory=False)
    conf.check(header_name='dirent.h', define_name='HAVE_DIRENT_H', mandatory=False)

    conf.env.prepend_value('LINKFLAGS', '-Wl,--no-as-needed')
    conf.env.append_value('LINKFLAGS', '-pthread')
    conf.check (lib='dl', mandatory = True)
    conf.check_cc(fragment='int main() {__get_cpu_features();}\n', msg='Checking for glibc get_cpu_features', define_name='HAVE_GETCPUFEATURES', mandatory=False)
     
    vg_h = conf.check(header_name='valgrind/valgrind.h', mandatory=False)
    vg_memcheck_h = conf.check(header_name='valgrind/memcheck.h', mandatory=False)
    if vg_h and vg_memcheck_h:
        conf.env.append_value('CXXDEFINES', 'HAVE_VALGRIND_H')

    conf.start_msg('Searching C library')
    libc = search_file ([
            '/lib64/libc.so.6',
            '/lib/libc.so.6',
            '/lib/x86_64-linux-gnu/libc.so.6',
            '/lib/i386-linux-gnu/libc.so.6',
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
            '/lib/x86_64-linux-gnu/libpthread.so.0',
            '/lib/i386-linux-gnu/libpthread.so.0',
            ])
    if libpthread is None:
        conf.fatal('not found')
    else:
        conf.end_msg(libpthread, True)
    conf.env['LIBPTHREAD_FILE'] = libpthread

    conf.start_msg('Searching rt library')
    librt = search_file ([
            '/lib64/librt.so.1',
            '/lib/librt.so.1',
            '/lib/x86_64-linux-gnu/librt.so.0',
            '/lib/i386-linux-gnu/librt.so.1',
            ])
    if librt is None:
        conf.fatal('not found')
    else:
        conf.end_msg(librt, True)
    conf.env['LIBRT_FILE'] = librt

    conf.find_program('readversiondef', var='READVERSIONDEF', mandatory=True)

    if Options.options.kernel_stack is not None and os.path.isdir(Options.options.kernel_stack):
        conf.check(header_name='sim.h',
                   includes=os.path.join(Options.options.kernel_stack, 'sim/include'))
      #  conf.check()
        conf.env['KERNEL_STACK'] = Options.options.kernel_stack

    conf_myscripts(conf)
    
    # Decide if VDL
    if Options.options.enable_vdl_loader:
        # Tests were explicitly enabled. 
        conf.env['ENABLE_VDL'] = True
    	conf.recurse(os.path.join('utils'))
    else:
        # Tests were explicitly disabled. 
        conf.env['ENABLE_VDL'] = False
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
        'netlink/netlink-socket-factory.h',
        'netlink/netlink-socket-address.h',
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
    d['linkflags'] = d.get('linkflags', []) + ['-pie'] + ['-lrt'] + debug_dl
    return d

def build_dce_tests(module, kern):
    if kern:
        module.add_runner_test(needed=['core', 'dce', 'internet'],  source=['test/dce-manager-test.cc', 'test/with-kernel.cc'])
    else:
        module.add_runner_test(needed=['core', 'dce', 'internet'], source=['test/dce-manager-test.cc','test/without-kernel.cc'])
    	    
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
             ['test-exec', []],
             ['test-exec-target-1', []],
             ['test-raw-socket', []],
             ['test-fork-fd', []],
             ['test-iperf', []],
             ['test-name', []],
             ['test-pipe', []],
             ['test-dirent', []],
             ['test-socket', []],
             ['test-bug-multi-select', []],
             ['test-tsearch', []],
             ['test-signal', []],
             ]
    for name,uselib in tests:
        module.add_test(**dce_kw(target='bin_dce/' + name, source = ['test/' + name + '.cc'],
                                 use = uselib + ['lib/test']))

def build_dce_examples(module):
    dce_examples = [['udp-server', []],
                    ['udp-client', []],
                    ['udp-perf', ['m']],
                    ['tcp-server', []],
                    ['tcp-client', []],
                    ['tcp-loopback', []],
                    ['unix-server', []],
                    ['unix-client', []],
                    ['udp-echo-server', []],
                    ['udp-echo-client', []],
#                    ['little-cout', []],
                    ]
    for name,lib in dce_examples:
        module.add_example(**dce_kw(target = 'bin_dce/' + name, 
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

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
                       target='bin/dce-ccnd-udp-2-nodes',
                       source=['example/ccnx/dce-ccnd-udp-2-nodes.cc', 'example/ccnx/misc-tools.cc'])

    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-ccnd-simple',
                       source=['example/ccnx/dce-ccnd-simple.cc'])
                       
    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-ccnd-short-stuff',
                       source=['example/ccnx/dce-ccnd-short-stuff.cc'])
                       
    module.add_example(needed = ['core', 'internet', 'dce', 'tap-bridge', 'point-to-point', 'csma'],
                       target='bin/dce-tap-udp-echo',
                       source=['example/ccnx/dce-tap-udp-echo.cc'])                

    module.add_example(needed = ['core', 'internet', 'dce', 'tap-bridge', 'csma' ], 
                       target='bin/dce-tap-ccnd',
                       source=['example/ccnx/dce-tap-ccnd.cc'])       
                       
    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
                       target='bin/dce-ccnd-linear-multiple',
                       source=['example/ccnx/dce-ccnd-linear-multiple.cc', 'example/ccnx/misc-tools.cc'])
                       
    module.add_example(needed = ['core', 'internet', 'dce', 'tap-bridge', 'csma' ], 
                       target='bin/dce-tap-vlc',
                       source=['example/ccnx/dce-tap-vlc.cc'])       
    
#    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
#                       target='bin/dce-ping',
#                       source=['example/dce-ping.cc', 'example/ccnx/misc-tools.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim', 'csma'], 
                       target='bin/dce-iperf',
                       source=['example/dce-iperf.cc', 'example/ccnx/misc-tools.cc'])
    
    module.add_example(needed = ['core', 'internet', 'dce' ], 
                       target='bin/dce-bash-simple',
                       source=['example/bash/dce-bash-simple.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
                       target='bin/dce-ccn-cache',
                       source=['example/ccnx/dce-ccn-cache.cc', 'example/ccnx/misc-tools.cc'])
                       
#    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
#                       target='bin/dce-xorp-simple',
#                       source=['example/xorp/dce-xorp-simple.cc', 'example/ccnx/misc-tools.cc'])
                                              
#    module.add_example(needed = ['core', 'internet', 'dce', 'csma' ], 
#                       target='bin/dce-udp-multicast',
#                       source=['example/dce-udp-multicast.cc'])
#    module.add_example(needed = ['core', 'dce', ], 
#                       target='bin/dce-cout-bug',
#                       source=['example/dce-cout-bug.cc'])
                                                                
def build_dce_kernel_examples(module):
    module.add_example(needed = ['core', 'network', 'dce'], 
                       target='bin/dce-linux-simple',
                       source=['example/dce-linux-simple.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'wifi', 'point-to-point', 'csma', 'mobility' ],
                       target='bin/dce-linux',
                       source=['example/dce-linux.cc'])

# Add a script to build system 
def build_a_script(bld, name, needed = [], **kw):
    external = [i for i in needed if not i == name]
    if not ns3waf.modules_found(bld, external):
    	return
    kw['use'] = kw.get('use', []) + ns3waf.modules_uselib(bld, needed)
    if 'features' not in kw:
        kw['features'] = 'cxx cxxprogram'
    bld(**kw)

# Add directories under myscripts dir
def add_myscripts(bld):
    for dir in os.listdir('myscripts'):
        if dir.startswith('.') or dir == 'CVS':
            continue
        if os.path.isdir(os.path.join('myscripts', dir)):
             bld.add_subdirs(os.path.join('myscripts', dir))

# Configure directories under myscripts dir
def conf_myscripts(conf):
    for dir in os.listdir('myscripts'):
        if dir.startswith('.') or dir == 'CVS':
            continue
        if os.path.isdir(os.path.join('myscripts', dir)):
             conf.recurse(os.path.join('myscripts', dir))

	
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
        'model/libc-dce.cc',
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
        'model/dce-time.cc',
        'model/dce-stat.cc',
        'model/dce-syslog.cc',
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
        'model/dce-misc.cc',
        'model/dce-node-context.cc',
        'model/dce-wait.cc',
        'model/wait-queue.cc',
        'model/file-usage.cc',
        'model/dce-poll.cc',
        'model/ipv4-dce-routing.cc',
        'model/dce-credentials.cc',
        'model/dce-pwd.cc',
        'model/pipe-fd.cc',
        'model/fifo-buffer.cc',
        'model/dce-dirent.cc',
        'model/dce-at.cc',
        'model/exec-utils.cc',
        'model/linux/ipv4-linux.cc',
        # helper.
        'helper/ipv4-dce-routing-helper.cc',
        'helper/dce-manager-helper.cc',
        'helper/dce-application-helper.cc',
        'helper/ccn-client-helper.cc',
        'helper/linux-stack-helper.cc',
        ]
    module_headers = [
        'model/dce-manager.h',
        'model/task-scheduler.h',
        'model/task-manager.h',
        'model/socket-fd-factory.h',
        'model/loader-factory.h',
        'model/dce-application.h',
        'model/ipv4-dce-routing.h',
        'model/linux/ipv4-linux.h',
        'helper/dce-manager-helper.h',
        'helper/dce-application-helper.h',
        'helper/ccn-client-helper.h',
        'helper/linux-stack-helper.h',
        'helper/ipv4-dce-routing-helper.h',
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
#                                  lib=['dl','efence'])
    build_dce_tests(module, bld.env['KERNEL_STACK'])
    build_dce_examples(module)

    if bld.env['KERNEL_STACK']:
        build_dce_kernel_examples(module)
    
    bld.build_a_script = types.MethodType(build_a_script, bld)

    add_myscripts(bld)

    bld.add_group('dce_version_files')

    bld(source=['model/libc-ns3.version'],
        target='model/libc.version',
        rule='%s %s | cat ${SRC[0].abspath()} - > ${TGT}' %
        (bld.env['READVERSIONDEF'], bld.env['LIBC_FILE']))

    bld(source=['model/libpthread-ns3.version'],
        target='model/libpthread.version',
        rule='%s %s | cat ${SRC[0].abspath()} - > ${TGT}' %
        (bld.env['READVERSIONDEF'], bld.env['LIBPTHREAD_FILE']))

    bld(source=['model/librt-ns3.version'],
        target='model/librt.version',
        rule='%s %s | cat ${SRC[0].abspath()} - > ${TGT}' %
        (bld.env['READVERSIONDEF'], bld.env['LIBRT_FILE']))

    bld.add_group('dce_use_version_files')

    # The very small libc used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc', 'model/libc-global-variables.cc'],
              target='lib/c-ns3', cflags=['-g'],
              defines=['LIBSETUP=libc_setup'],
              linkflags=['-nostdlib', 
                         '-Wl,--version-script=' + os.path.join('model', 'libc.version'),
                         '-Wl,-soname=libc.so.6'])

    # The very small libpthread used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc'],
              target='lib/pthread-ns3', cflags=['-g'],
              defines=['LIBSETUP=libpthread_setup'],
              linkflags=['-nostdlib', '-lc',
                         '-Wl,--version-script=' + os.path.join('model', 'libpthread.version'),
                         '-Wl,-soname=libpthread.so.0'])

    # The very small librt used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc'],
              target='lib/rt-ns3', cflags=['-g'],
              defines=['LIBSETUP=librt_setup'],
              linkflags=['-nostdlib', '-lc',
                         '-Wl,--version-script=' + os.path.join('model', 'librt.version'),
                         '-Wl,-soname=librt.so.1'])

    if bld.env['ENABLE_VDL']:                     
        bld.add_subdirs(['utils'])
