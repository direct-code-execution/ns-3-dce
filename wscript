## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import os.path
import ns3waf
import sys
import types
# local modules
import wutils
import subprocess
from waflib import Utils, Scripting, Configure, Build, Options, TaskGen, Context, Task, Logs, Errors
from waflib.Errors import WafError

def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    ns3waf.options(opt)
    opt.add_option('--enable-kernel-stack',
                   help=('Path to the prefix where the kernel wrapper headers are installed'),
                   default=None,
                   dest='kernel_stack', type="string")
    opt.add_option('--enable-mpi',
                   help=('Enable MPI and distributed simulation support'),
                   dest='enable_mpi', action='store_true',
                   default=False)
    opt.add_option('--enable-opt',
                   help=('Enable use of DCE and NS-3 optimized compilation'),
                   dest='enable_opt', action='store_true',
                   default=False)    
    opt.add_option('--with-ns3',
                   help=('Specify the installed directory of ns-3-dev'),
                   dest='with_ns3', type='string',
                   default=None)    
    opt.add_option('--with-elf-loader',
                   help=('Specify the installed directory of elf-loader'),
                   dest='with_elf_loader', type='string',
                   default=None)
    opt.add_option('--with-libaspect',
                   help=('Specify the installed directory of libaspect.so'),
                   dest='with_libaspect', type='string',
                   default=None)
    opt.add_option('--cwd',
                   help=('Set the working directory for a program.'),
                   action="store", type="string", default=None,
                   dest='cwd_launch')
    opt.add_option('--command-template',
                   help=('Template of the command used to run the program given by --run;'
                         ' It should be a shell command string containing %s inside,'
                         ' which will be replaced by the actual program.'),
                   type="string", default=None, dest='command_template')
    opt.add_option('--run',
                   help=('Run a locally built program; argument can be a program name,'
                         ' or a command starting with the program name.'),
                   type="string", default='', dest='run')
    opt.add_option('--pyrun',
                   help=('Run a python program using locally built dce and ns3 python modules;'
                         ' argument is the path to the python program, optionally followed'
                         ' by command-line options that are passed to the program.'),
                   type="string", default='', dest='pyrun')

    opt.add_option('--dlmloader',
                   help=('Run with DlmLoaderFactory,'),
                   action="store_true", default=False, dest='dlm')
    opt.add_option('--visualize',
                   help=('Modify --run arguments to enable the visualizer'),
                   action="store_true", default=False, dest='visualize')
    opt.add_option('--valgrind',
                   help=('Change the default command template to run programs and unit tests with valgrind'),
                   action="store_true", default=False,
                   dest='valgrind')
    opt.recurse('bindings/python')
                                  
def search_file(files):
    for f in files:
        if os.path.isfile (f):
            return f
    return None

def configure(conf):

    conf.load('relocation', tooldir=['waf-tools'])
    conf.load('compiler_c')
    cc_string = '.'.join(conf.env['CC_VERSION'])
    conf.msg('Checking for cc version',cc_string,'GREEN')
    conf.load('compiler_cxx')

    conf.load('clang_compilation_database', tooldir=['waf-tools'])

    if Options.options.with_ns3 is not None and os.path.isdir(Options.options.with_ns3):
        conf.env['NS3_DIR']= os.path.abspath(Options.options.with_ns3)
        if not 'PKG_CONFIG_PATH' in os.environ:
            os.environ['PKG_CONFIG_PATH']= os.path.join(Options.options.with_ns3, 'lib', 'pkgconfig')
            os.environ['PKG_CONFIG_PATH']+= ":" + os.path.join(Options.options.with_ns3, 'lib64', 'pkgconfig')
        else:
            os.environ['PKG_CONFIG_PATH']+= ":" + os.path.join(Options.options.with_ns3, 'lib', 'pkgconfig')
            os.environ['PKG_CONFIG_PATH']+= ":" + os.path.join(Options.options.with_ns3, 'lib64', 'pkgconfig')

        conf.env.append_value('NS3_EXECUTABLE_PATH', os.path.join(conf.env['NS3_DIR'], 'bin'))

    ns3waf.check_modules(conf, ['core', 'network', 'internet'], mandatory = True)
    ns3waf.check_modules(conf, ['point-to-point', 'tap-bridge', 'netanim'], mandatory = False)
    ns3waf.check_modules(conf, ['wifi', 'point-to-point', 'csma', 'mobility'], mandatory = False)
    ns3waf.check_modules(conf, ['point-to-point-layout'], mandatory = False)
    ns3waf.check_modules(conf, ['mpi', 'lte'], mandatory = False)
    ns3waf.check_modules(conf, ['visualizer'], mandatory = False)
    ns3waf.check_modules(conf, ['applications'], mandatory = False)
    ns3waf.check_modules(conf, ['fd-net-device'], mandatory = False)
    conf.check(header_name='stdint.h', define_name='HAVE_STDINT_H', mandatory=False)
    conf.check(header_name='inttypes.h', define_name='HAVE_INTTYPES_H', mandatory=False)
    conf.check(header_name='sys/inttypes.h', define_name='HAVE_SYS_INT_TYPES_H', mandatory=False)
    conf.check(header_name='sys/types.h', define_name='HAVE_SYS_TYPES_H', mandatory=False)
    conf.check(header_name='sys/stat.h', define_name='HAVE_SYS_STAT_H', mandatory=False)
    conf.check(header_name='dirent.h', define_name='HAVE_DIRENT_H', mandatory=False)

    if Options.options.enable_mpi:
         conf.env.append_value ('DEFINES', 'DCE_MPI=1')
         conf.env['MPI'] = '1'
         
    conf.env.prepend_value('LINKFLAGS', '-Wl,--no-as-needed')
    conf.env.append_value('LINKFLAGS', '-pthread')
    conf.check (lib='dl', mandatory = True)
    conf.check_cc(fragment='int main() {__get_cpu_features();}\n', msg='Checking for glibc get_cpu_features', define_name='HAVE_GETCPUFEATURES', mandatory=False)
    conf.check_cc(fragment='int main() {secure_getenv("test");}\n', msg='Checking for glibc secure_getenv', define_name='HAVE_SECURE_GETENV', mandatory=False)
    conf.check_cc(fragment='int main() {explicit_bzero("test", 1, 1);}\n', msg='Checking for glibc explicit_bzero', define_name='HAVE___EXPLICIT_BZERO', mandatory=False)

    vg_h = conf.check(header_name='valgrind/valgrind.h', mandatory=False)
    vg_memcheck_h = conf.check(header_name='valgrind/memcheck.h', mandatory=False)
    if vg_h and vg_memcheck_h:
        conf.env.append_value('CXXDEFINES', 'HAVE_VALGRIND_H')


    # Enable C++-11 support
    conf.env.append_value('CXXFLAGS', '-std=c++11')

    if Options.options.kernel_stack:
        if not os.path.isdir(Options.options.kernel_stack):
            Logs.error( "\"%s\" is not a directory: please fix your --enable-kernel-stack parameter." % (Options.options.kernel_stack))
            raise SystemExit(1)

        # look for kernel dir from 1) {KERNEL_DIR}/lkl, 2) {KERNEL_DIR}/sim, then 3) {KERNEL_DIR}/lib.
        architectures = ["lkl", "sim", "lib"]
        kernel_stack_dir = None
        for dir in architectures:
            dir = os.path.join(Options.options.kernel_stack, dir)
            if os.path.isdir(dir):
                kernel_stack_dir = dir 
                break

        if not kernel_stack_dir:
            Logs.error("Could not find any of the [%s] architecture. Make sure you use the net-next-sim kernel or fix your --enabled-kernel-stack parameter" % ','.join(architectures))
            raise SystemExit(1)

        # add include dir to KERNEL_DIR if architecture is lkl.
        # This will help DCE to locate dce_init.h and lkl.h
        if (kernel_stack_dir == Options.options.kernel_stack+architectures[0]):
            kernel_stack_dir = os.path.join(kernel_stack_dir, "include")
        else:
            conf.check(header_name='sim.h',
                       includes=os.path.join(kernel_stack_dir, 'include'))
        conf.env['KERNEL_STACK'] = kernel_stack_dir
        conf.env.append_value ('DEFINES', 'KERNEL_STACK=Y')

    conf.env['ENABLE_PYTHON_BINDINGS'] = False
    conf.env['EXAMPLE_DIRECTORIES'] = '.'
    conf.env['NS3_ENABLED_MODULES'] = []
    conf_myscripts(conf)
    
    if Options.options.with_elf_loader is not None and os.path.isdir(Options.options.with_elf_loader):
         if os.path.exists(Options.options.with_elf_loader + '/ldso'):
             conf.env['ELF_LOADER_PATH'] = Options.options.with_elf_loader
         else:
             Logs.warn("elf-loader does not exist")

    ns3waf._report_optional_feature(conf, "elf-loader", "ELF magic loader",
                                    conf.env['ELF_LOADER_PATH'],
                                    "elf-loader not found")

    conf.env['LIB_ASPECT_PATH'] = conf.env['PREFIX']
    if Options.options.with_libaspect is not None and os.path.isdir(Options.options.with_libaspect):
         if os.path.exists(Options.options.with_libaspect):
             conf.env['LIB_ASPECT_PATH'] = os.path.abspath(Options.options.with_libaspect)
         else:
             Logs.warn("libaspect does not exist")

    have_aspect = conf.check(header_name='hook-manager.h', lib=['aspect', 'dl'],
                             includes=os.path.join(conf.env['LIB_ASPECT_PATH'], 'include'),
                             libpath=os.path.join(conf.env['LIB_ASPECT_PATH'], 'lib'),
                             uselib_store='ASPECT', mandatory=False)

    if have_aspect is None:
        conf.env['LIB_ASPECT_PATH'] = None

    ns3waf._report_optional_feature(conf, "libaspect", "Aspect-based tracing",
                                    have_aspect,
                                    "libaspect not found")

    conf.env['VALGRIND_FOUND'] = False
    try:
        conf.find_program('valgrind', var='VALGRIND')
        conf.env['VALGRIND_FOUND'] = True
    except WafError:
        pass

    try:
        conf.find_program('doxygen', var='DOXYGEN')
    except WafError:
        pass

    # sctp-tools check
    have_sctp_tools = conf.check(header_name='netinet/sctp.h', 
                                define_name='HAVE_SCTP_H', mandatory=False)
    conf.env['SCTP_TOOLS_FOUND'] = True
    if have_sctp_tools is None:
        conf.env['SCTP_TOOLS_FOUND'] = False
    ns3waf._report_optional_feature(conf, "sctp", "sctp-tools-dev",
                                    have_sctp_tools,
                                    "sctp-tools (netinet/sctp.h) not found")

    conf.recurse(os.path.join('utils'))
    conf.recurse('bindings/python')
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

def dce_kw(**kw):
    d = dict(**kw)
    if os.uname()[4] == 'x86_64':
        mcmodel = ['-mcmodel=large']
    else:
        mcmodel = []
    nofortify = ['-U_FORTIFY_SOURCE']
    #debug_dl = ['-Wl,--dynamic-linker=/usr/lib/debug/ld-linux-x86-64.so.2']
    debug_dl = []
    d['cxxflags'] = d.get('cxxflags', []) + ['-fpie'] + mcmodel + nofortify + ['-Wno-deprecated-declarations']
    d['cflags'] = d.get('cflags', []) + ['-fpie'] + mcmodel + nofortify
    d['linkflags'] = d.get('linkflags', []) + ['-pie'] + ['-lrt'] + ['-rdynamic'] + debug_dl
    return d

def build_dce_tests(module, bld):
    tests_source = [
        'test/dce-manager-test.cc', 
        ]
    if bld.env['KERNEL_STACK']:
        tests_source += [
            'test/dce-cradle-test.cc',
            'test/dce-mptcp-test.cc',
            ]

    for dir in os.listdir('test/addons'):
        if dir.startswith('.') or dir == 'CVS':
            continue
        elif dir.endswith(".cc"):
            tests_source += ["test/addons/" + dir]

    module.add_runner_test(needed=['core', 'dce', 'internet', 'applications'],
                           source=tests_source)

    module.add_test(features='cxx cxxshlib', source=['test/test-macros.cc'], 
                    target='lib/test', linkflags=['-Wl,-soname=libtest.so'])
    bld.install_files('${PREFIX}/lib', 'lib/libtest.so', chmod=0755 )

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
             ['test-clock-gettime', []],
             ['test-gcc-builtin-apply', []],
             ]
    for name,uselib in tests:
        module.add_test(**dce_kw(target='bin_dce/' + name, source = ['test/' + name + '.cc'],
                                 use = uselib + ['lib/test']))
        bld.install_files('${PREFIX}/bin_dce', 'build/bin_dce/' + name , chmod=0755 )

def build_dce_examples(module, bld):
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
                    ['dccp-server', []],
                    ['dccp-client', []],
                    ['freebsd-iproute', []],
#                    ['little-cout', []],
                    ]

    if bld.env['SCTP_TOOLS_FOUND']:
        dce_examples += [
                    ['sctp-server', ['sctp']],
                    ['sctp-client', ['sctp']],
        ]

    for name,lib in dce_examples:
        module.add_example(**dce_kw(target = 'bin_dce/' + name, 
                                    source = ['example/' + name + '.cc'],
                                    lib = lib))
        bld.install_files('${PREFIX}/bin_dce', 'bin_dce/' + name , chmod=0755 )        

    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-tcp-simple',
                       source=['example/dce-tcp-simple.cc'])

    module.add_example(needed = ['core', 'internet', 'dce'], 
                       target='bin/dce-udp-simple',
                       source=['example/dce-udp-simple.cc'])
    
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
                       
    module.add_example(needed = ['core', 'internet', 'dce', 'tap-bridge', 'csma' ], 
                       target='bin/dce-tap-vlc',
                       source=['example/ccnx/dce-tap-vlc.cc'])       
    
#    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
#                       target='bin/dce-ping',
#                       source=['example/dce-ping.cc', 'example/ccnx/misc-tools.cc'])

    module.add_example(needed = ['core', 'internet', 'dce' ], 
                       target='bin/dce-bash-simple',
                       source=['example/bash/dce-bash-simple.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
                       target='bin/dce-ccn-cache',
                       source=['example/ccnx/dce-ccn-cache.cc', 'example/ccnx/misc-tools.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim', 'csma'], 
                       target='bin/dce-iperf',
                       source=['example/dce-iperf.cc', 'example/ccnx/misc-tools.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim', 'csma', 'fd-net-device'], 
                       target='bin/dce-iperf-emulation',
                       source=['example/dce-iperf-emulation.cc', 'example/ccnx/misc-tools.cc'])
                       
    module.add_example(needed = ['core', 'network', 'internet', 'dce', 'point-to-point', 'csma', 'applications'],
                       target='bin/linear-udp-perf',
                       source=['example/linear-udp-perf.cc'])

    if bld.env['LIB_ASPECT_PATH']:
        module.add_example(needed = ['core', 'network', 'internet', 'dce', 'point-to-point', 'csma', 'applications'],
                           target='bin/dce-debug-aspect',
                           use=['ASPECT'],
                           source=['example/dce-debug-aspect.cc'])

#    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
#                       target='bin/dce-xorp-simple',
#                       source=['example/xorp/dce-xorp-simple.cc', 'example/ccnx/misc-tools.cc'])
                                              
#    module.add_example(needed = ['core', 'internet', 'dce', 'csma' ], 
#                       target='bin/dce-udp-multicast',
#                       source=['example/dce-udp-multicast.cc'])
#    module.add_example(needed = ['core', 'dce', ], 
#                       target='bin/dce-cout-bug',
#                       source=['example/dce-cout-bug.cc'])
                                                                
def build_dce_kernel_examples(module, bld):
    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point'], 
                       target='bin/dce-udp-perf',
                       source=['example/dce-udp-perf.cc'])

    module.add_example(needed = ['core', 'network', 'dce'], 
                       target='bin/dce-linux-simple',
                       source=['example/dce-linux-simple.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'wifi', 'point-to-point', 'csma', 'mobility' ],
                       target='bin/dce-linux',
                       source=['example/dce-linux.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'csma'], 
                       target='bin/dce-dccp',
                       source=['example/dce-dccp.cc'])
    
    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
                       target='bin/dce-ccnd-udp-2-nodes',
                       source=['example/ccnx/dce-ccnd-udp-2-nodes.cc', 'example/ccnx/misc-tools.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim'], 
                       target='bin/dce-ccnd-linear-multiple',
                       source=['example/ccnx/dce-ccnd-linear-multiple.cc', 'example/ccnx/misc-tools.cc'])
                       

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point'], 
                       target='bin/dce-xfrm',
                       source=['example/dce-xfrm.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'wifi', 'point-to-point', 'csma', 'mobility' ],
                       target='bin/dce-ltp',
                       source=['example/dce-ltp.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'mobility'],
                       target='bin/dce-iperf-mptcp',
                       source=['example/dce-iperf-mptcp.cc'])

    module.add_example(needed = ['core', 'internet', 'network', 'dce', 'point-to-point', 'mobility'],
                       target='bin/dce-iperf-heterogeneous-multihop',
                       source=['example/dce-iperf-heterogeneous-multihop.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'mobility', 'applications'],
                       target='bin/dce-cradle-mptcp',
                       source=['example/dce-cradle-mptcp.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'mobility', 'wifi', 'dce-quagga'],
                       target='bin/dce-mptcp-handoff-v6',
                       source=['example/dce-mptcp-handoff-v6.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'mobility', 'wifi', 'dce-quagga'],
                       target='bin/dce-mptcp-handoff-v4v6',
                       source=['example/dce-mptcp-handoff-v4v6.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'mobility', 'wifi', 'lte'],
                       target='bin/dce-mptcp-lte-wifi',
                       source=['example/dce-mptcp-lte-wifi.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'mobility', 'wifi', 'lte', 'dce-quagga'],
                       target='bin/dce-mptcp-lte-wifi-v6',
                       source=['example/dce-mptcp-lte-wifi-v6.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'applications'],
                       target='bin/dce-cradle-simple',
                       source=['example/dce-cradle-simple.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'point-to-point', 'applications', 'internet'],
                       target='bin/dce-tcp-ns3-nsc-comparison',
                       source=['example/dce-tcp-ns3-nsc-comparison.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point'],
                       target='bin/dce-httpd',
                       source=['example/dce-httpd.cc'])

    module.add_example(needed = ['core', 'internet', 'dce', 'point-to-point', 'netanim', 'mobility', 'wifi', 'network'],
                       target='bin/dce-wifi-ccnx',
                       source=['example/ccnx/dce-wifi-ccnx.cc'])

    if bld.env['SCTP_TOOLS_FOUND']:
        module.add_example(needed = ['core', 'network', 'dce', 'point-to-point' ],
                           target='bin/dce-sctp-simple',
                           source=['example/dce-sctp-simple.cc'])

    module.add_example(needed = ['core', 'network', 'dce', 'wifi', 'point-to-point', 'csma', 'mobility' ],
                       target='bin/dce-freebsd',
                       source=['example/dce-freebsd.cc'])


# Add a script to build system 
def build_a_script(bld, name, needed = [], **kw):
    external = [i for i in needed if not i == name]
    if not ns3waf.modules_found(bld, external):
    	return
    kw['use'] = kw.get('use', []) + ns3waf.modules_uselib(bld, needed)
    if 'features' not in kw:
        kw['features'] = 'cxx cxxprogram'
    if bld.env['NS3_ENABLE_STATIC']:
        for module in kw['use']:
            kw['linkflags'] = kw.get('linkflags', [])
            # XXX pkg-config doesn't give the proper order of whole-archive option..
            if 'dce' in module.lower():
                continue
            if 'netlink' in module.lower():
                continue
            if 'header' in module.lower():
                continue
            if 'library' in module.lower():
                continue
            kw['linkflags'] += ['-ldl']
            kw['linkflags'] += ['-Wl,--whole-archive,-Bstatic']
            kw['linkflags'] += bld.env['STLIB_ST_%s' % module.upper()]
    program = bld(**kw)
    program.is_ns3_program = True
    bld.env.append_value('NS3_RUNNABLE_PROGRAMS', name)


# Add directories under myscripts dir
def add_myscripts(bld):
    # submodule parse should be the first
    submodules = []
    scratches = []

    for dir in os.listdir('myscripts'):
        if dir.startswith('.') or dir == 'CVS':
            continue
        if os.path.isdir(os.path.join('myscripts', dir)):
            if dir.startswith('ns-3-dce-'):
                submodules.append(dir)
            else:
                scratches.append(dir)
        else:
            scratches.append(dir)

    for dir in submodules:
        bld.recurse(os.path.join('myscripts', dir))

    for dir in scratches:
        if os.path.isdir(os.path.join('myscripts', dir)):
            bld.recurse(os.path.join('myscripts', dir))
        elif dir.endswith(".cc"):
            bld.build_a_script('dce',
                               needed = bld.env['NS3_MODULES_FOUND'] + ['dce'],
                               target='bin/' + os.path.splitext(dir)[0],
                               source=[os.path.join('myscripts', dir)])

# Configure directories under myscripts dir
def conf_myscripts(conf):
    for dir in os.listdir('myscripts'):
        if dir.startswith('.') or dir == 'CVS':
            continue
        if os.path.isdir(os.path.join('myscripts', dir)):
             conf.recurse(os.path.join('myscripts', dir))

	
def _get_all_task_gen(self):
    for group in self.groups:
        for taskgen in group:
            yield taskgen

def build(bld):    
    bld.env['NS3_MODULES_WITH_TEST_LIBRARIES'] = []
    bld.env['NS3_ENABLED_MODULE_TEST_LIBRARIES'] = []
    bld.env['NS3_SCRIPT_DEPENDENCIES'] = []
    bld.env['NS3_RUNNABLE_PROGRAMS'] = []
    bld.env['NS3_RUNNABLE_SCRIPTS'] = []

    bld.add_group('ns3moduleheader')
    bld.add_group('ns3modulebuild')
    build_netlink(bld)

    if bld.env['KERNEL_STACK']:
        kernel_source = [
            'model/kernel-socket-fd-factory.cc',
            'model/kernel-socket-fd.cc',
            'model/linux-socket-fd-factory.cc',
            'model/freebsd-socket-fd-factory.cc',
            'model/linux/linux-socket-impl.cc',
            ]
        kernel_headers = [
            'model/kernel-socket-fd-factory.h',
            'model/linux-socket-fd-factory.h',
            'model/freebsd-socket-fd-factory.h',
            'model/linux/linux-socket-impl.h',
            ]
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
        'model/dce-dl.cc',
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
        'model/linux/ipv6-linux.cc',
        'model/freebsd/ipv4-freebsd.cc',
        'model/dce-vfs.cc',
        'model/elf-ldd.cc',
        'model/dce-termio.cc',
        'model/process-delay-model.cc',
        'model/linux/linux-ipv4-raw-socket-factory.cc',
        'model/linux/linux-ipv4-raw-socket-factory-impl.cc',
        'model/linux/linux-ipv6-raw-socket-factory.cc',
        'model/linux/linux-ipv6-raw-socket-factory-impl.cc',
        'model/linux/linux-udp-socket-factory.cc',
        'model/linux/linux-udp-socket-factory-impl.cc',
        'model/linux/linux-udp6-socket-factory.cc',
        'model/linux/linux-udp6-socket-factory-impl.cc',
        'model/linux/linux-tcp-socket-factory.cc',
        'model/linux/linux-tcp-socket-factory-impl.cc',
        'model/linux/linux-tcp6-socket-factory.cc',
        'model/linux/linux-tcp6-socket-factory-impl.cc',
        'model/linux/linux-dccp-socket-factory.cc',
        'model/linux/linux-dccp-socket-factory-impl.cc',
        'model/linux/linux-dccp6-socket-factory.cc',
        'model/linux/linux-dccp6-socket-factory-impl.cc',
        'model/linux/linux-sctp-socket-factory.cc',
        'model/linux/linux-sctp-socket-factory-impl.cc',
        'model/linux/linux-sctp6-socket-factory.cc',
        'model/linux/linux-sctp6-socket-factory-impl.cc',
        # helper.
        'helper/ipv4-dce-routing-helper.cc',
        'helper/dce-manager-helper.cc',
        'helper/dce-application-helper.cc',
        'helper/ccn-client-helper.cc',
        'helper/linux-stack-helper.cc',
        'helper/freebsd-stack-helper.cc',
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
        'model/linux/ipv6-linux.h',
        'model/freebsd/ipv4-freebsd.h',
        'model/process-delay-model.h',        
        'model/exec-utils.h',
        'model/utils.h',
        'model/linux/linux-ipv4-raw-socket-factory.h',
        'model/linux/linux-ipv6-raw-socket-factory.h',
        'model/linux/linux-udp-socket-factory.h',
        'model/linux/linux-udp6-socket-factory.h',
        'model/linux/linux-tcp-socket-factory.h',
        'model/linux/linux-tcp6-socket-factory.h',
        'model/linux/linux-dccp-socket-factory.h',
        'model/linux/linux-dccp6-socket-factory.h',
        'model/linux/linux-sctp-socket-factory.h',
        'model/linux/linux-sctp6-socket-factory.h',
        'helper/dce-manager-helper.h',
        'helper/dce-application-helper.h',
        'helper/ccn-client-helper.h',
        'helper/ipv4-dce-routing-helper.h',
        'helper/linux-stack-helper.h',
        'helper/freebsd-stack-helper.h',
        ]

    module_source = module_source + kernel_source
    module_headers = module_headers + kernel_headers
    uselib = ns3waf.modules_uselib(bld, ['core', 'network', 'internet', 'netlink'])
    module = ns3waf.create_module(bld, name='dce',
                                  source=module_source,
                                  headers=module_headers,
                                  use=uselib,
                                  includes=kernel_includes,
                                  cxxflags= ['-Wno-deprecated-declarations'],
                                  lib=['dl'])
#                                  lib=['dl','efence'])

    # make sure submodules built before building examples (that depend submodules)
    bld.build_a_script = types.MethodType(build_a_script, bld)
    add_myscripts(bld)

    build_dce_tests(module, bld)
    build_dce_examples(module, bld)

    # no idea to solve this two-way dependency (dce <-> netlink)
    module.add_runner_test(needed = ['internet', 'point-to-point', 'core', 'dce'], 
                           use=uselib,
                           includes=['netlink'],
                           source=['test/netlink-socket-test.cc'],
                           name='netlink')

    if bld.env['KERNEL_STACK']:
        build_dce_kernel_examples(module, bld)
    
    # build test-runner
    module.add_example(target='bin/test-runner',
                       source = ['utils/test-runner.cc'],
                       use = bld.env['NS3_ENABLED_MODULE_TEST_LIBRARIES'],
                       linkflags = [],
                       needed = bld.env['NS3_MODULES_FOUND'] + ['dce'])
    bld.env.append_value('NS3_RUNNABLE_PROGRAMS', 'bin/test-runner')
    if bld.env['ELF_LOADER_PATH']:
        module.add_example(target='bin/test-runner-vdl',
                           source = ['utils/test-runner.cc'],
                           use = bld.env['NS3_ENABLED_MODULE_TEST_LIBRARIES'],
                           linkflags = ['-Wl,--dynamic-linker=' + os.path.abspath (bld.env['ELF_LOADER_PATH'] + '/ldso')],
                           needed = bld.env['NS3_MODULES_FOUND'] + ['dce'])


    bld.add_group('dce_version_files')
    
    bld.program(source='utils/dcemakeversion.c', 
                name='dcemakeversion',
                target='dcemakeversion', cflags = [ '-g'], linkflags    = ['-lpthread', '-lrt', '-lm', '-ldl'])

    bld(source=['dcemakeversion','model/libc-ns3.version' , 'model/libpthread-ns3.version' ,
                'model/librt-ns3.version', 'model/libm-ns3.version', 'model/libdl-ns3.version'],
        target=['model/libc.version','model/libpthread.version','model/librt.version','model/libm.version','model/libdl.version'],
        rule='${SRC[0].abspath()} ${SRC[1].abspath()}  ${SRC[2].abspath()}  ${SRC[3].abspath()} ${SRC[4].abspath()}  ${SRC[5].abspath()}')

    bld.add_group('dce_use_version_files')

    # The very small libc used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc', 'model/libc-global-variables.cc'],
              target='lib/c-ns3',
              cxxflags=['-g', '-fno-profile-arcs', '-fno-test-coverage', '-Wno-builtin-declaration-mismatch'],
              defines=['LIBSETUP=libc_setup'],
              linkflags=['-nostdlib', '-fno-profile-arcs',
                         '-Wl,--version-script=' + os.path.join('model', 'libc.version'),
                         '-Wl,-soname=libc.so.6'])
    # The very small libpthread used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc'],
              target='lib/pthread-ns3',
              cxxflags=['-g', '-fno-profile-arcs', '-fno-test-coverage', '-Wno-builtin-declaration-mismatch'],
              defines=['LIBSETUP=libpthread_setup'],
              linkflags=['-nostdlib', '-fno-profile-arcs',
                         '-Wl,--version-script=' + os.path.join('model', 'libpthread.version'),
                         '-Wl,-soname=libpthread.so.0'])

    # The very small librt used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc'],
              target='lib/rt-ns3',
              cxxflags=['-g', '-fno-profile-arcs', '-fno-test-coverage', '-Wno-builtin-declaration-mismatch'],
              defines=['LIBSETUP=librt_setup'],
              linkflags=['-nostdlib', '-fno-profile-arcs',
                         '-Wl,--version-script=' + os.path.join('model', 'librt.version'),
                         '-Wl,-soname=librt.so.1'])

    # The very small libm used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc'],
              target='lib/m-ns3',
              cxxflags=['-g', '-fno-profile-arcs', '-fno-test-coverage', '-Wno-builtin-declaration-mismatch'],
              defines=['LIBSETUP=libm_setup'],
              linkflags=['-nostdlib', '-fno-profile-arcs',
                         '-Wl,--version-script=' + os.path.join('model', 'libm.version'),
                         '-Wl,-soname=libm.so.6'])

    # The very small libdl used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.cc', 'model/libc-setup.cc'],
              target='lib/dl-ns3',
              cxxflags=['-g', '-fno-profile-arcs', '-fno-test-coverage', '-Wno-builtin-declaration-mismatch'],
              defines=['LIBSETUP=libdl_setup'],
              linkflags=['-nostdlib', '-fno-profile-arcs',
                         '-Wl,--version-script=' + os.path.join('model', 'libdl.version'),
                         '-Wl,-soname=libdl.so.2'])

    bld.recurse(['utils'])
    bld.recurse('bindings/python')

    # Write the build status file.
    build_status_file = os.path.join(bld.out_dir, 'build-status.py')
    out = open(build_status_file, 'w')
    out.write('#! /usr/bin/env python\n')
    out.write('\n')
    out.write('# Programs that are runnable.\n')
    out.write('ns3_runnable_programs = ' + str(bld.env['NS3_RUNNABLE_PROGRAMS']) + '\n')
    out.write('\n')
    out.write('# Scripts that are runnable.\n')
    out.write('ns3_runnable_scripts = ' + str(bld.env['NS3_RUNNABLE_SCRIPTS']) + '\n')
    out.write('\n')
    out.close()

    wutils.bld = bld
    bld.__class__.all_task_gen = property(_get_all_task_gen)
    Options.cwd_launch = bld.path.abspath()
    if Options.options.run:
        # Check that the requested program name is valid
        program_name, dummy_program_argv = wutils.get_run_program(Options.options.run, wutils.get_command_template(bld.env))

        # When --run'ing a program, tell WAF to only build that program,
        # nothing more; this greatly speeds up compilation when all you
        # want to do is run a test program.
        Options.options.targets += ',' + os.path.basename(program_name)
        if getattr(Options.options, "visualize", False):
            program_obj = wutils.find_program(program_name, bld.env)
            program_obj.use.append('NS3_VISUALIZER')
        for gen in bld.all_task_gen:
            if type(gen).__name__ in ['task_gen', 'ns3header_taskgen', 'ns3moduleheader_taskgen']:
                gen.post()
        bld.env['PRINT_BUILT_MODULES_AT_END'] = False 



def _doxygen(bld):
    env = wutils.bld.env
    proc_env = wutils.get_proc_env()

    if not env['DOXYGEN']:
        Logs.error("waf configure did not detect doxygen in the system -> cannot build api docs.")
        raise SystemExit(1)
        return

    # try:
    #     program_obj = wutils.find_program('print-introspected-doxygen', env)
    # except ValueError: 
    #     Logs.warn("print-introspected-doxygen does not exist")
    #     raise SystemExit(1)
    #     return

    # prog = program_obj.path.find_or_declare(program_obj.target).abspath()

    # if not os.path.exists(prog):
    #     Logs.error("print-introspected-doxygen has not been built yet."
    #                " You need to build ns-3 at least once before "
    #                "generating doxygen docs...")
    #     raise SystemExit(1)

    # # Create a header file with the introspected information.
    # doxygen_out = open(os.path.join('doc', 'introspected-doxygen.h'), 'w')
    # if subprocess.Popen([prog], stdout=doxygen_out, env=proc_env).wait():
    #     raise SystemExit(1)
    # doxygen_out.close()

    # # Create a text file with the introspected information.
    # text_out = open(os.path.join('doc', 'ns3-object.txt'), 'w')
    # if subprocess.Popen([prog, '--output-text'], stdout=text_out, env=proc_env).wait():
    #     raise SystemExit(1)
    # text_out.close()

    #_getVersion()
    doxygen_config = os.path.join('doc', 'doxygen.conf')
    if subprocess.Popen([env['DOXYGEN'], doxygen_config]).wait():
        Logs.error("Doxygen build returned an error.")
        raise SystemExit(1)


from waflib import Context, Build
class Ns3ShellContext(Context.Context):
    """run a shell with an environment suitably modified to run locally built programs"""
    cmd = 'shell'

    def execute(self):

        # first we execute the build
	bld = Context.create_context("build")
	bld.options = Options.options # provided for convenience
	bld.cmd = "build"
	bld.execute()

        # Set this so that the lists won't be printed when the user
        # exits the shell.
        bld.env['PRINT_BUILT_MODULES_AT_END'] = False

        if sys.platform == 'win32':
            shell = os.environ.get("COMSPEC", "cmd.exe")
        else:
            shell = os.environ.get("SHELL", "/bin/sh")

        env = bld.env
        os_env = {
            'NS3_MODULE_PATH': os.pathsep.join(env['NS3_MODULE_PATH']),
            'NS3_EXECUTABLE_PATH': os.pathsep.join(env['NS3_EXECUTABLE_PATH']),
            }
        wutils.run_argv([shell], env, os_env)

class Ns3DoxygenContext(Context.Context):
    """do a full build, generate the introspected doxygen and then the doxygen"""
    cmd = 'doxygen'
    def execute(self):
        # first we execute the build
	bld = Context.create_context("build")
	bld.options = Options.options # provided for convenience
	bld.cmd = "build"
	bld.execute()
        _doxygen(bld)

from waflib import Context, Build
class Ns3SphinxContext(Context.Context):
    """build the Sphinx documentation: manual, tutorial, models"""
    
    cmd = 'sphinx'

    def sphinx_build(self, path):
        print
        print "[waf] Building sphinx docs for " + path
        if path == './doc':
            dir_opt = "BUILDDIR=" + os.getcwd () + "/doc/build/"
        else:
            dir_opt = "BUILDDIR=" + os.getcwd () + "/doc/build-" + (os.path.basename(path.replace("/doc" ,""))).replace("ns-3-dce-","")
        if subprocess.Popen(["make", "SPHINXOPTS=-N", "-k",
                             dir_opt,
                             "html", "singlehtml", "latexpdf" ],
                            cwd=path).wait() :
            Logs.error("Sphinx build of " + path + " returned an error.")
            raise SystemExit(1)

    def execute(self):
        #_getVersion()
        import glob
        for sphinxdir in ["./doc"] + glob.glob('myscripts/*/doc') :
            self.sphinx_build(sphinxdir)
     

from waflib import Context, Build
class Ns3DocContext(Context.Context):
    """build all the documentation: doxygen, manual, tutorial, models"""
    
    cmd = 'docs'

    def execute(self):
        steps = ['doxygen', 'sphinx']
        Options.commands = steps + Options.commands
        

def shutdown(ctx):
    bld = wutils.bld
    if wutils.bld is None:
        return
    env = bld.env

    if Options.options.run:
        wutils.run_program(Options.options.run, env, wutils.get_command_template(env),
                           visualize=Options.options.visualize)
        raise SystemExit(0)

    if Options.options.pyrun:
        wutils.run_python_program(Options.options.pyrun, env,
                                  visualize=Options.options.visualize)
        raise SystemExit(0)

