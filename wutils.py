import os
import os.path
import sys
import subprocess
import shlex
import re

# WAF modules
from waflib.Errors import WafError
from waflib import Utils, Scripting, Configure, Build, Options, TaskGen, Context, Task, Logs, Errors

# these are set from the main wscript file
APPNAME=None
VERSION=None
bld=None



def get_command_template(env, arguments=()):
    cmd = Options.options.command_template or '%s'
    for arg in arguments:
        cmd = cmd + " " + arg
    return cmd


if hasattr(os.path, "relpath"):
    relpath = os.path.relpath # since Python 2.6
else:
    def relpath(path, start=os.path.curdir):
        """Return a relative version of a path"""

        if not path:
            raise ValueError("no path specified")

        start_list = os.path.abspath(start).split(os.path.sep)
        path_list = os.path.abspath(path).split(os.path.sep)

        # Work out how much of the filepath is shared by start and path.
        i = len(os.path.commonprefix([start_list, path_list]))

        rel_list = [os.path.pardir] * (len(start_list)-i) + path_list[i:]
        if not rel_list:
            return os.path.curdir
        return os.path.join(*rel_list)

from waflib import Context
def find_program(program_name, env):
    launch_dir = os.path.abspath(Options.cwd_launch)
    #top_dir = os.path.abspath(Options.cwd_launch)
    found_programs = []
    for obj in bld.all_task_gen:
        if not getattr(obj, 'is_ns3_program', False):
            continue

        ## filter out programs not in the subtree starting at the launch dir
        if not (obj.path.abspath().startswith(launch_dir)
                or obj.path.get_bld().abspath().startswith(launch_dir)):
            continue
        
        name1 = os.path.basename(obj.name)
        name2 = os.path.join(relpath(obj.path.abspath(), launch_dir), obj.name)
        names = [name1, name2]
        found_programs.extend(names)
        if program_name in names:
            return obj
    raise ValueError("program '%s' not found; available programs are: %r"
                     % (program_name, found_programs))

def get_proc_env(os_env=None):
    env = bld.env
    if sys.platform == 'linux2':
        pathvar = 'LD_LIBRARY_PATH'
    elif sys.platform == 'darwin':
        pathvar = 'DYLD_LIBRARY_PATH'
    elif sys.platform == 'win32':
        pathvar = 'PATH'
    elif sys.platform == 'cygwin':
        pathvar = 'PATH'
    elif sys.platform.startswith('freebsd'):
        pathvar = 'LD_LIBRARY_PATH'
    else:
        Logs.warn(("Don't know how to configure "
                        "dynamic library path for the platform %r;"
                        " assuming it's LD_LIBRARY_PATH.") % (sys.platform,))
        pathvar = 'LD_LIBRARY_PATH'        

    proc_env = dict(os.environ)
    if os_env is not None:
        proc_env.update(os_env)

    if pathvar is not None:
        if pathvar in proc_env:
            proc_env[pathvar] = os.pathsep.join(list(env['NS3_MODULE_PATH']) + [proc_env[pathvar]])
        else:
            proc_env[pathvar] = os.pathsep.join(list(env['NS3_MODULE_PATH']))

    # DCE specific env
    proc_env[pathvar] = os.pathsep.join([proc_env[pathvar], \
                                             os.path.join(bld.out_dir, 'lib'), \
                                             os.path.join(bld.out_dir, 'lib64'), \
                                             os.path.join(bld.out_dir, 'bin'), \
                                             os.path.join(bld.env.NS3_DIR, 'lib'), \
                                             os.path.join(bld.env.NS3_DIR, 'lib64'), \
                                             os.path.join(bld.env.NS3_DIR, 'bin')])

    dce_paths = [os.getenv('DCE_PATH'), \
            os.path.join(bld.out_dir, 'bin_dce'), \
            os.path.join(bld.env.NS3_DIR, 'sbin'), \
            os.path.join(bld.env.NS3_DIR, 'bin_dce'), \
            proc_env[pathvar]]
    proc_env['DCE_PATH'] = os.pathsep.join([_f for _f in dce_paths if _f])

    dce_roots = [os.getenv('DCE_ROOT'), \
            os.path.join(bld.out_dir), \
            os.path.join(bld.env.PREFIX)]
    proc_env['DCE_ROOT'] = os.pathsep.join([_f for _f in dce_roots if _f])

    pymoddir = bld.path.find_dir('bindings/python')
    if pymoddir is not None:
        pymoddir = pymoddir.get_bld().abspath()
    else:
        pymoddir = ""
    import glob
    pyns3dir = glob.glob(bld.env.NS3_DIR + '/lib*/python*/site-packages')
    # XXX:
    if len(pyns3dir) is not 0:
        pyvizdir = pyns3dir[len(pyns3dir) - 1]
    else:
        pyvizdir = ''
    if 'PYTHONPATH' in proc_env:
        proc_env['PYTHONPATH'] = os.pathsep.join([pymoddir, pyvizdir] + [proc_env['PYTHONPATH']])
    else:
        proc_env['PYTHONPATH'] = os.pathsep.join([pymoddir, pyvizdir])

    if 'PATH' in proc_env:
        proc_env['PATH'] = os.pathsep.join(list(env['NS3_EXECUTABLE_PATH']) + [proc_env['PATH']])
    else:
        proc_env['PATH'] = os.pathsep.join(list(env['NS3_EXECUTABLE_PATH']))

    return proc_env

def run_argv(argv, env, os_env=None, cwd=None, force_no_valgrind=False):
    proc_env = get_proc_env(os_env)
    if Options.options.valgrind and not force_no_valgrind:
        if Options.options.command_template:
            raise WafError("Options --command-template and --valgrind are conflicting")
        if not env['VALGRIND']:
            raise WafError("valgrind is not installed")
        argv = [env['VALGRIND'], "--leak-check=full", "--show-reachable=yes", "--error-exitcode=1"] + argv
        proc = subprocess.Popen(argv, env=proc_env, cwd=cwd, stderr=subprocess.PIPE)
        error = False
        for line in proc.stderr:
            sys.stderr.write(line)
            if "== LEAK SUMMARY" in line:
                error = True
        retval = proc.wait()
        if retval == 0 and error:
            retval = 1
    else:
        try:
            WindowsError
        except NameError:
            retval = subprocess.Popen(argv, env=proc_env, cwd=cwd).wait()
        else:
            try:
                retval = subprocess.Popen(argv, env=proc_env, cwd=cwd).wait()
            except WindowsError as ex:
                raise WafError("Command %s raised exception %s" % (argv, ex))
    if retval:
        signame = None
        if retval < 0: # signal?
            import signal
            for name, val in vars(signal).items():
                if len(name) > 3 and name[:3] == 'SIG' and name[3] != '_':
                    if val == -retval:
                        signame = name
                        break
        if signame:
            raise WafError("Command %s terminated with signal %s."
                                 " Run it under a debugger to get more information "
                                 "(./waf --run <program> --command-template=\"gdb --args %%s <args>\")." % (argv, signame))
        else:
            raise WafError("Command %s exited with code %i" % (argv, retval))
    return retval

def get_run_program(program_string, command_template=None):
    """
    Return the program name and argv of the process that would be executed by
    run_program(program_string, command_template).
    """
    #print "get_run_program_argv(program_string=%r, command_template=%r)" % (program_string, command_template)
    env = bld.env

    if command_template in (None, '%s'):
        argv = shlex.split(program_string)
        #print "%r ==shlex.split==> %r" % (program_string, argv)
        program_name = argv[0]

        try:
            program_obj = find_program(program_name, env)
        except ValueError as ex:
            raise WafError(str(ex))

        program_node = program_obj.path.find_or_declare(program_obj.target)
        #try:
        #    program_node = program_obj.path.find_build(ccroot.get_target_name(program_obj))
        #except AttributeError:
        #    raise Utils.WafError("%s does not appear to be a program" % (program_name,))

        execvec = [program_node.abspath()] + argv[1:]

    else:

        program_name = program_string
        try:
            program_obj = find_program(program_name, env)
        except ValueError as ex:
            raise WafError(str(ex))

        program_node = program_obj.path.find_or_declare(program_obj.target)
        #try:
        #    program_node = program_obj.path.find_build(ccroot.get_target_name(program_obj))
        #except AttributeError:
        #    raise Utils.WafError("%s does not appear to be a program" % (program_name,))

        tmpl = command_template % (program_node.abspath(),)
        execvec = shlex.split(tmpl.replace('\\', '\\\\'))
        #print "%r ==shlex.split==> %r" % (command_template % (program_node.abspath(env),), execvec)
    return program_name, execvec

def run_program(program_string, env, command_template=None, cwd=None, visualize=False):
    """
    if command_template is not None, then program_string == program
    name and argv is given by command_template with %s replaced by the
    full path to the program.  Else, program_string is interpreted as
    a shell command with first name being the program name.
    """
    dummy_program_name, execvec = get_run_program(program_string, command_template)
    if cwd is None:
        if (Options.options.cwd_launch):
            cwd = Options.options.cwd_launch
        else:
            cwd = Options.cwd_launch
    if (Options.options.dlm):
        dummy_program_name, dce_runner = get_run_program('dce-runner', command_template)
        execvec = dce_runner + execvec
    if visualize:
        execvec.append("--SimulatorImplementationType=ns3::VisualSimulatorImpl")
    return run_argv(execvec, env, cwd=cwd)



def run_python_program(program_string, env, visualize=False):
    env = bld.env
    execvec = shlex.split(program_string)
    if (Options.options.cwd_launch):
        cwd = Options.options.cwd_launch
    else:
        cwd = Options.cwd_launch
    if visualize:
        execvec.append("--SimulatorImplementationType=ns3::VisualSimulatorImpl")
    return run_argv([env['PYTHON'][0]] + execvec, env, cwd=cwd)


