## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import waflib

def options(opt):
    opt.tool_options('compiler_cc')
    opt.tool_options('compiler_cxx')
    opt.add_option('--enable-static',
                   help=('Compile module statically: works only on linux, without python'),
                   dest='enable_static', action='store_true',
                   default=False)
    opt.add_option('--disable-log',
                   help=('Do not compile into the code the log output instructions.'),
                   dest='enable_log', action='store_false',
                   default=True)
    opt.add_option('--disable-assert',
                   help='Do not compile into the code the assert checks.',
                   dest='enable_assert', action='store_false',
                   default=True)
    opt.add_option('--enable-gcov',
                   help='Enable code coverage collection.',
                   dest='enable_gcov', action='store_true',
                   default=False)
    opt.add_option('--disable-examples', help='Disable compilation of examples',
                   dest='enable_examples', action='store_false',
                   default=True)
    opt.add_option('--disable-tests', help='Disable compilation of tests',
                   dest='enable_tests', action='store_false',
                   default=True)
    opt.add_option('--disable-debug', help='Disable generation of debug information',
                   dest='enable_debug', action='store_false',
                   default=True)

def _report_optional_feature(conf, name, caption, was_enabled, reason_not_enabled):
    if not 'NS3_OPTIONAL_FEATURES' in conf.env:
        conf.env['NS3_OPTIONAL_FEATURES'] = []
    conf.env['NS3_OPTIONAL_FEATURES'].append((name, caption, was_enabled, reason_not_enabled))


def _check_compilation_flag(conf, flag, mode='cxx'):
    """
    Checks if the C++ compiler accepts a certain compilation flag or flags
    flag: can be a string or a list of strings
    """
    try:
        if mode == 'cxx':
            conf.check_cc(fragment='#include <stdio.h>\nint main() { return 0; }\n',
                          cflags=flag,
                          execute = False, msg = "Checking for %s" % flag)
        else:
            conf.check_cxx(fragment='#include <stdio.h>\nint main() { return 0; }\n',
                           cxxflags=flag,
                           execute = False, msg = "Checking for %s" % flag)

    except conf.errors.ConfigurationError:
        ok = False
    else:
        ok = True
    return ok


def _print_optional_features(conf):
    # Write a summary of optional features status
    print "---- Summary of optional NS-3 features:"
    for (name, caption, was_enabled, reason_not_enabled) in conf.env['NS3_OPTIONAL_FEATURES']:
        if was_enabled:
            status = 'enabled'
        else:
            status = 'not enabled (%s)' % reason_not_enabled
        print "%-30s: %s" % (caption, status)

def _check_static(conf):
    import Options
    import sys
    import re
    import os
    env = conf.env
    env['NS3_ENABLE_STATIC'] = False
    if Options.options.enable_static:
        if sys.platform.startswith('linux') and \
                env['CXX_NAME'] in ['gcc', 'icc']:
            if re.match('i[3-6]86', os.uname()[4]):
                _report_optional_feature(conf, "static", "Static build", True, '')
                env['NS3_ENABLE_STATIC'] = True
            elif os.uname()[4] == 'x86_64':
                if env['NS3_ENABLE_PYTHON_BINDINGS'] and \
                        not _check_compilation_flag(conf, '-mcmodel=large'):
                    _report_optional_feature(conf, "static", "Static build", False,
                                             "Can't enable static builds because " + \
                                                 "no -mcmodel=large compiler " \
                                                 "option. Try --disable-python or upgrade your " \
                                                 "compiler to at least gcc 4.3.x.")
                else:
                    _report_optional_feature(conf, "static", "Static build", True, '')
                    env['NS3_ENABLE_STATIC'] = True                    
        elif env['CXX_NAME'] == 'gcc' and \
                (sys.platform.startswith('darwin') or \
                     sys.platform.startswith('cygwin')):
                _report_optional_feature(conf, "static", "Static build", True, '')
                env['NS3_ENABLE_STATIC'] = True
        else:
            _report_optional_feature(conf, "static", "Static build", False,
                                     "Unsupported platform")
    else:
        _report_optional_feature(conf, "static", "Static build", False,
                                 "option --enable-static not selected")
    # These flags are used for the implicitly dependent modules.
    if env['NS3_ENABLE_STATIC']:
        if sys.platform == 'darwin':
            env['STLIB_MARKER'] = '-Wl,-all_load'
        else:
            env['STLIB_MARKER'] = '-Wl,--whole-archive,-Bstatic'
            env['SHLIB_MARKER'] = '-Wl,-Bdynamic,--no-whole-archive'

def _check_win32(conf):
    import Options
    import sys
    import subprocess
    import os
    env = conf.env
    if conf.env['CXX_NAME'] in ['gcc', 'icc']:
        if sys.platform == 'win32':
            env.append_value("LINKFLAGS", "-Wl,--enable-runtime-pseudo-reloc")
        elif sys.platform == 'cygwin':
            env.append_value("LINKFLAGS", "-Wl,--enable-auto-import")

        cxx, = env['CXX']

        p = subprocess.Popen([cxx, '-print-file-name=libstdc++.so'], stdout=subprocess.PIPE)
        libstdcxx_location = os.path.dirname(p.stdout.read().strip())
        p.wait()
        if libstdcxx_location:
            conf.env.append_value('NS3_MODULE_PATH', libstdcxx_location)

        if Options.platform in ['linux']:
            if _check_compilation_flag(conf, '-Wl,--soname=foo'):
                env['WL_SONAME_SUPPORTED'] = True


def _check_dependencies(conf, required, mandatory):
    found = []
    for module in required:
        retval = conf.check_cfg(package = 'libns3-%s' % module.lower(),
                                args='--cflags --libs', mandatory=mandatory,
                                msg="Checking for ns3-%s" % module.lower(),
                                uselib_store='NS3_%s' % module.upper())
        if not retval is None:
            found.append(module)
    import copy
    if not 'NS3_MODULES_FOUND' in conf.env:
        conf.env['NS3_MODULES_FOUND'] = []
    conf.env['NS3_MODULES_FOUND'] = conf.env['NS3_MODULES_FOUND'] + copy.copy(found)

def modules_uselib(bld, names):
    return ['NS3_%s' % name.upper() for name in names] + \
        ['NS3_LIBRARY_%s' % name.upper() for name in names] + \
        ['NS3_HEADERS_%s' % name.upper() for name in names]

def modules_found(bld, needed):
    for module in needed:
        if not module in bld.env['NS3_MODULES_FOUND']:
            return False
    return True

def _c_libname(bld, name):
    libname = 'ns3-' + name
    if bld.env['NS3_ENABLE_STATIC']:
        return bld.env['cstlib_PATTERN'] % libname
    else:
        return bld.env['cshlib_PATTERN'] % libname

def check_modules(conf, modules, mandatory = True):
    import Options
    import os

    if not 'NS3_CHECK_MODULE_ONCE' in conf.env:
        conf.env['NS3_CHECK_MODULE_ONCE'] = ''
        conf.check_tool('compiler_cc')
        conf.check_tool('compiler_cxx')
        conf.check_cfg(atleast_pkgconfig_version='0.0.0')
        _check_win32(conf)
        _check_static(conf)
        if Options.options.enable_log:
            _report_optional_feature(conf, "log", "Logging", True, '')
            conf.env.append_value('DEFINES', 'NS3_LOG_ENABLE')
        else:
            _report_optional_feature(conf, "log", "Logging", False, 
                                     'option --disable-log selected')
        if Options.options.enable_assert:
            _report_optional_feature(conf, "assert", "Assert checks", True, '')
            conf.env.append_value('DEFINES', 'NS3_ASSERT_ENABLE')
        else:
            _report_optional_feature(conf, "assert", "Assert checks", False, 
                                     'option --disable-assert selected')
        if Options.options.enable_gcov:
            _report_optional_feature(conf, "coverage", "Code coverage", True, '')
            conf.env.append_value('CFLAGS', '-fprofile-arcs')
            conf.env.append_value('CFLAGS', '-ftest-coverage')
            conf.env.append_value('CXXFLAGS', '-fprofile-arcs')
            conf.env.append_value('CXXFLAGS', '-ftest-coverage')
            conf.env.append_value('LINKFLAGS', '-fprofile-arcs')
        else:
            _report_optional_feature(conf, "coverage", "Code coverage", False, 
                                     'option --enable-gcov not selected')
        if Options.options.enable_examples:
            _report_optional_feature(conf, "examples", "Example programs", True, '')
            conf.env['NS3_ENABLE_EXAMPLES'] = True
        else:
            _report_optional_feature(conf, "examples", "Example programs", False, 
                                     'option --disable-examples selected')
            conf.env['NS3_ENABLE_EXAMPLES'] = False

        if Options.options.enable_tests:
            _report_optional_feature(conf, "tests", "Test programs", True, '')
            conf.env['NS3_ENABLE_TESTS'] = True
        else:
            _report_optional_feature(conf, "tests", "Test programs", False, 
                                     'option --disable-tests selected')
            conf.env['NS3_ENABLE_TESTS'] = False            

        if Options.options.enable_debug:
            if 'CXXFLAGS' in conf.env:
                tmp = conf.env['CXXFLAGS']
            else:
                tmp = []
            conf.env['CXXFLAGS'] = tmp + ['-g']
            if 'CFLAGS' in conf.env:
                tmp = conf.env['CFLAGS']
            else:
                tmp = []
            conf.env['CFLAGS'] = tmp + ['-g']
            _report_optional_feature(conf, "debug", "Debug Symbols", True, '')
        else:
            _report_optional_feature(conf, "debug", "Debug Symbols", False, 
                                     'option --disable-debug selected')

    _check_dependencies(conf, modules, mandatory)

def print_feature_summary(conf):
    _print_optional_features(conf)

def _dirs(source):
    import os
    dirs = [os.path.dirname(s) for s in source]
    def uniq(l):
        d = dict()
        for i in l:
            d[i] = True
        return d.keys()
    return uniq(dirs)

def _build_library(bld, name, *k, **kw):
    import os
    source = kw.get('source')
    if source is None:
        return
    cxxflags = []
    cflags = []
    linkflags = []
    ccdefines = ['NS3_MODULE_COMPILATION']
    cxxdefines = ['NS3_MODULE_COMPILATION']
    includes = _dirs(source)
    target = os.path.join('lib', 'ns3-%s' % name)
    if not bld.env['NS3_ENABLE_STATIC']:
        if bld.env['CXX_NAME'] in ['gcc', 'icc'] and bld.env['WL_SONAME_SUPPORTED']:
            linkflags.append('-Wl,--soname=%s' % _c_libname(bld, name))
            pass
        elif bld.env['CXX_NAME'] in ['gcc', 'icc'] and \
                os.uname()[4] == 'x86_64' and \
                bld.env['NS3_ENABLE_PYTHON_BINDINGS']:
            # enable that flag for static builds only on x86-64 platforms
            # when gcc is present and only when we want python bindings
            # (it's more efficient to not use this option if we can avoid it)
            cxxflags.append('-mcmodel=large')
            cflags.append('-mcmodel=large')
    if bld.env['NS3_ENABLE_STATIC']:
        lib_type = 'stlib'
    else:
        lib_type = 'shlib'
    features = waflib.Tools.c_aliases.sniff_features(source=source, _type=lib_type)
    kw['features'] = features
    kw['target'] = target
    kw['cxxflags'] = kw.get('cxxflags', []) + cxxflags
    kw['cflags'] = kw.get('cflags', []) + cflags
    kw['linkflags'] = kw.get('linkflags', []) + linkflags
    kw['ccdefines'] = kw.get('ccdefines', []) + ccdefines
    kw['cxxdefines'] = kw.get('cxxdefines', []) + cxxdefines
    kw['includes'] = kw.get('includes', []) + includes
    bld(*k, **kw)
    bld(name='NS3_LIBRARY_%s' % name.upper(), use=[target])


def _build_headers(bld, name, headers):
    if headers is None:
        return
    import os
    import shutil
    def run(task):
        out_dir = os.path.dirname(task.outputs[0].abspath())
        for header in task.inputs:
            dst = os.path.join(out_dir, os.path.basename(header.abspath()))
            src = header.abspath()
            shutil.copyfile(src, dst)

        outfile = file(task.outputs[0].abspath(), "w")

        print >> outfile, """
#ifdef NS3_MODULE_COMPILATION
# error "Do not include ns3 module aggregator headers from other modules; these are meant only for end user scripts."
#endif

#ifndef NS3_MODULE_%s
    """ % (name.upper().replace('-', '_'),)

        print >> outfile
        print >> outfile, "// Module headers:"
        for header in [src.abspath() for src in task.inputs]:
            print >> outfile, "#include \"%s\"" % (os.path.basename(header),)

        print >> outfile, "#endif"

        outfile.close()
    target = os.path.join('include', 'ns3', '%s-module.h' % name)
    bld(rule=run, source=headers, target=target)
    bld(use=[target], target='NS3_HEADERS_%s' % name.upper(),
        export_includes=['include'])
    bld.install_files(os.path.join('${PREFIX}', 'include', 'ns3'), headers + [target])



def _lib(bld, dep):
    libpath = bld.env['LIBPATH_%s' % dep.upper()]
    linkflags = bld.env['LINKFLAGS_%s' % dep.upper()]
    libs = bld.env['LIB_%s' % dep.upper()]
    retval = []
    for path in libpath:
        retval.append(bld.env['LIBPATH_ST'] % path)
    retval = retval + linkflags
    for lib in libs:
        retval.append(bld.env['LIB_ST'] % lib)
    return retval

def _cflags(bld, dep):
    return bld.env['CFLAGS_%s' % dep]
def _cxxflags(bld, dep):
    return bld.env['CXXFLAGS_%s' % dep]
def _defines(bld, dep):
    return [bld.env['DEFINES_ST'] % define for define in bld.env['DEFINES_%s' % dep]]
def _includes(bld, dep):
    return [bld.env['CPPPATH_ST'] % include for include in bld.env['INCLUDES_%s' % dep]]

def _self_lib(bld, name, libdir):
    if bld.env['NS3_ENABLE_STATIC']:
        path_st = 'STLIBPATH_ST'
        lib_st = 'STLIB_ST'
        lib_marker = 'STLIB_MARKER'
    else:
        path_st = 'LIBPATH_ST'
        lib_st = 'LIB_ST'
        lib_marker = 'SHLIB_MARKER'
    libname = 'ns3-' + name
    return [bld.env[path_st] % libdir,
            bld.env[lib_marker],
            bld.env[lib_st] % libname]

def _generate_pcfile(bld, name, use, prefix, outfilename):
    import os
    outfile = open(outfilename, 'w')
    includedir = os.path.join(prefix, 'include')
    libdir = os.path.join(prefix, 'lib')
    libs = _self_lib(bld, name, '${libdir}')
    for dep in use:
        libs = libs + _lib(bld,dep)
    cflags = [bld.env['CPPPATH_ST'] % '${includedir}']
    for dep in use:
        cflags = cflags + _cflags(bld, dep) + _cxxflags(bld, dep) + \
            _defines(bld, dep) + _includes(bld, dep)
    print >> outfile, """
prefix=%s
libdir=%s
includedir=%s

Name: libns3-%s
Description: ns-3 module %s
Version: devel
Libs: %s
Cflags: %s
""" % (prefix, libdir, includedir,
       name, name, ' '.join(libs), ' '.join(cflags))
    outfile.close()

def _build_pkgconfig(bld, name, use):
    import os
    def run(task):
        _generate_pcfile(bld, name, use, bld.env['PREFIX'], task.outputs[0].abspath())
        return 0
    target = os.path.join('lib', 'pkgconfig', 'libns3-%s.pc' % name)
    bld(rule=run, target=target, always=True)
    bld.install_files(os.path.join('${PREFIX}', 'lib', 'pkgconfig'), [target])

class Module:
    def __init__(self, dirs, bld, name):
        self._source_dirs = dirs
        self._bld = bld
        self._name = name
    def add_tests(self, name = None, **kw):
        import copy
        import os
        import tempfile

        if not name is None:
            target='ns3test-%s-%s' % (self._name, name)
        else:
            target='ns3test-%s' % self._name
        target = os.path.join('bin', target)

        uselib = kw.get('use', [])
        if not modules_uselib(self._bld, [self._name]) in uselib:
            uselib = uselib + modules_uselib(self._bld, [self._name])
        kw['use'] = uselib
        kw['includes'] = kw.get('includes', []) + self._source_dirs

        tmp = self._bld.path.relpath_gen(self._bld.srcnode)
        objects = []
        for src in kw['source']:
            src_target = '%s_object' % src
            objects.append(src_target)
            kw_copy = copy.copy (kw)
            path = os.path.dirname(os.path.join(tmp, src))
            kw_copy['source'] = [src]
            kw_copy['target'] = src_target
            kw_copy['defines'] = kw_copy.get('defines', []) + ['NS_TEST_SOURCEDIR=%s' % path]
            self._bld.objects(**kw_copy)
        handle, filename = tempfile.mkstemp(suffix='.cc')
        os.write (handle, """
#include "ns3/test.h"

int main (int argc, char *argv[])
{
  return ns3::TestRunner::Run(argc, argv);
}
""")
        os.close(handle)
        kw['source'] = [os.path.relpath(filename, self._bld.bldnode.abspath())]
        kw['use'] = uselib + objects
        kw['target'] = target
        kw['install_path'] = None
        self._bld.program(**kw)


def create_module(bld, name, *k, **kw):
    _build_library(bld, name, *k, **kw)
    _build_headers(bld, name, kw.get('headers'))
    _build_pkgconfig(bld, name, kw.get('use'))
    return Module(_dirs(kw.get('source')), bld, name)

def build_program(bld, target=None, source = None, use = None):
    bld.program(source=source, target=target, use=use)

def build_example(bld, target=None, source = None, use = None):
    if bld.env['NS3_ENABLE_EXAMPLES']:
        bld.program(source=source, target=target, use=use)



