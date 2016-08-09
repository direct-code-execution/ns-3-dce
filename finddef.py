#!/usr/bin/env python3
# Example taken out of http://pygccxml.readthedocs.io/en/develop/examples/searching1/example.html
from pygccxml import utils
from pygccxml import declarations
from pygccxml import parser
from pygccxml.declarations import declaration_utils
from collections import namedtuple
import os
import argparse
import csv
import subprocess
import logging
import sys


"""
Must be able to generate:
    - 

todo do same for dl/pthread/rt
"""

log = logging.getLogger("dce")
log.setLevel(logging.DEBUG)
log.addHandler(logging.StreamHandler())



# Parse the c++ file
# decls = parser.parse([filename], xml_generator_config)



int_type = declarations.cpptypes.int_t()
double_type = declarations.cpptypes.double_t()


# int dce___cxa_atexit (void (*func)(void *), void *arg, void *d);
# void dce___cxa_finalize (void *d);
# __newlocale

# list of exceptions for functions thatp ygccxml fail to identify correctly
# hack around https://github.com/gccxml/pygccxml/issues/62
# specifier = noexcept
ExplicitFn = namedtuple('ExplicitFn', ["rtype", "fullargs", "arg_names", "location", "specifier"])
exceptions = {
    "sysinfo": ExplicitFn("int", "struct sysinfo *info", "info","/usr/include/x86_64-linux-gnu/sys/sysinfo.h", "noexcept"),
    "sigaction": ExplicitFn("int", "int signum, const struct sigaction *act, struct sigaction *oldact", "signum, act, oldact","/usr/include/signal.h", "noexcept"),
    "wait": ExplicitFn("pid_t", "void *stat_loc", "stat_loc", "/usr/include/x86_64-linux-gnu/sys/wait.h", ""),
    "__fpurge": ExplicitFn("void", "FILE *fd", "fd", "/usr/include/stdio.h", ""),
    "__fpending": ExplicitFn("size_t", "FILE *fd", "fd", "/usr/include/stdio.h", ""),
    "__cxa_atexit": ExplicitFn("int", "void (*func)(void *), void *arg, void *d", "func, arg, d", "/usr/include/stdlib.h", ""),
    "__cxa_finalize": ExplicitFn("void", "void *d", "d", "/usr/include/stdlib.h", ""),
    "fstat64": ExplicitFn("int", "int __fd, struct stat64 *__buf", "__fd, __buf", "/usr/include/x86_64-linux-gnu/sys/stat.h", "noexcept"),
    "pthread_kill": ExplicitFn("int", "pthread_t thread, int sig", "thread, sig", "/usr/include/signal.h", "noexcept"),
    "uname": ExplicitFn("int", "struct utsname *__name", "__name", "/usr/include/x86_64-linux-gnu/sys/utsname.h", "noexcept"),
    "statvfs": ExplicitFn("int", "const char *path, struct statvfs *buf", "path, buf", "/usr/include/x86_64-linux-gnu/sys/statvfs.h", "noexcept"),
    "statfs": ExplicitFn("int", "const char *path, struct statfs *buf", "path, buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
    "statfs64": ExplicitFn("int", "const char *path, struct statfs64 *buf", "path, buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
    # TODO the attributes should not be in definition for GCC (except is part of the type)
    "abort": ExplicitFn("void", "void", "", "/usr/include/stdlib.h", "noexcept __attribute__ ((__noreturn__))"),
    "exit": ExplicitFn("void", "int status", "status", "/usr/include/stdlib.h", "noexcept __attribute__ ((__noreturn__))"),
    "pthread_exit": ExplicitFn("void", "void *retval", "retval", "/usr/include/pthread.h", "__attribute__ ((__noreturn__))"),
    "fstatfs": ExplicitFn("int", "int __fildes, struct statfs * __buf", "__fildes, __buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
    "fstatvfs": ExplicitFn("int", "int __fildes, struct statvfs * __buf", "__fildes, __buf", "/usr/include/x86_64-linux-gnu/sys/statvfs.h", "noexcept"),
    "fstatfs64": ExplicitFn("int", "int __fildes, struct statfs64 * __buf", "__fildes, __buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
# int dce_fstatfs (int __fildes, struct statfs * __buf) noexcept;
    }



class Generator:
    def __init__(self):
        pass
    
    def parse(self, filename):
        """
        cache parsing
        """
        utils.loggers.set_level(logging.DEBUG)

        # Find the location of the xml generator (castxml or gccxml)
        generator_path, generator_name = utils.find_xml_generator()

        # Configure the xml generator
        xml_generator_config = parser.xml_generator_configuration_t(
            xml_generator_path=generator_path,
            xml_generator=generator_name,
            cflags=" -nostdinc -I/usr/include",

            # asked on tracker to generate va_list but not ok
            # flags= ["f1"]
            )
        # config.flags = 
        # The c++ file we want to parse
        # printf is declared in stdio.h
        # filename = "/home/teto/glibc/libio/stdio.h"
        filename = "test.h"

        file_config = parser.file_configuration_t(
            data=filename,
            # content_type=parser.CONTENT_TYPE.CACHED_SOURCE_FILE
            )

        self.project_reader = parser.project_reader_t(xml_generator_config)
        self.decls = self.project_reader.read_files(
            [file_config],
            compilation_mode=parser.COMPILATION_MODE.FILE_BY_FILE)



    def generate_alias(self, aliasname, decl):
                   #define weak_alias(name, aliasname) \
  # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));
        return ""


    def lookup(self, toto):
        
        log.info("Looking for %s" % toto)

        global_namespace = declarations.get_global_namespace(self.decls)
        criteria = declarations.calldef_matcher(name=toto)
        results = declarations.matcher.find(criteria, global_namespace) 
        print("resultats=", results)
        return results[0]

    def generate_wrappers(self, input_filename, libc_filename,
            write_headers : bool,
            write_impl : bool
        ):
        """
        Generate wrappers + headers
        """

        # input_filename = "natives.h.txt"
        locations = {}
        with open(input_filename, "r") as src:
            # aliasnames = last columns ?
            reader = csv.DictReader(src, fieldnames=["type","name"], restkey="extra")
            with open(libc_filename, "w+") as libc_fd:
                # for line in src:
                for row in reader:
                    # function_name = line.rstrip()
                    # print(line)
                    # if row["type"] == "alias":
# or row["type"] == "dce":
                        # continue

                    has_ellipsis = False
                    # look for a match
                    print('row["name"]=', row["name"], "extra=", row["extra"])    
                    # print('row["name"]=', row["name"])    
                    # decl = global_namespace.free_function(name=row["name"])  
                   # Search for the function by name

                   # if row["name"] in ["sigaction", "sysinfo"]
                    # TODO if there is an ellipsis then it should be forwarded as 
                    # va_list
                    template = """
                    {extern} {ret} {name} ({fullargs}) {{
                        {handle_va_list}
                        {retstmt} g_libc.{name}_fn ({arg_names});
                    }}
                    """
                    res = ""
                    # hack around https://github.com/gccxml/pygccxml/issues/62
                    if row["name"] in exceptions.keys():
                        name = row["name"]
                        log.debug("Exception [%s] found " % name)
                        extern=""
                        rtype, libc_fullargs , arg_names, location, specifier = exceptions[name]
                        # partialargs = fullargs
                        # **exceptions[name]
                        print("Values:", rtype, libc_fullargs, arg_names, location)
                    else:

                        decl= self.lookup(row["name"])

                        # print("decl", results)
                        # decl = results[0]
                        # print( "islist ? len",len(func1))
                        name = declaration_utils.full_name(decl)
                        if name[:2] == "::":
                            name = name[2:]
                        log.info("Parsing function %s" % name)
                        # Add the arguments...
                        # default_value', 'ellipsis', 'name', 'type'
                        # print(dir(decl.arguments[0]))
                        # TODO we should record the location
                        # locations.update ({ decl.location })
                        # {ret} {name} ({args}) 
                        # proto = "{extern} {ret} {name} ({args})".format(

                        extern="extern" if decl.has_extern else ""
                        rtype = "%s" % (decl.return_type if decl.return_type is not None else "void")

                        # for a in decl.arguments:
                        #     print(dir(a ))
                        #     print("a", a )
                        #     print("a", a.name )
                        # print("decl=", decl.create_decl_type )
                        print("decl=", decl.calling_convention )
                        print("decl=", decl.does_throw )
                        if decl.has_ellipsis:
                            print("HAS ELLIPSIS")

                        #     print("a", a.attributes )
                        #     # print("a", a.type )
                        

                        # exit(1)
                        # for now keep only types, but hopefully we should have everything
                        # problems appear with function pointer declaration
                        # fullargs = ",".join([str(a.decl_type) for a in decl.arguments])
                        # for arg in decl.arguments:
                        #     print("arg=[%s]"% arg)
                        #     print("arg=[%s]"% dir(arg))

                        # some hacks to workaround pygccxml/castxml flaws:
                        # va_list is not_recognized and function pointer badly displayed

                        temp = []
                        for arg in decl.arguments:
                            s = str(arg.decl_type)
                            if s.startswith("?unknown?"):
                                print("UNKNOWN")
                                s = "va_list"
                            elif "(" in s:
                                print ("TOTO")
                                s= s.rstrip("*")
                            else:
                                s += " " + arg.name
                            temp.append(s)

                        for arg in temp:
                            print("arg=%s"% arg)
                        # temp = ["va_list" else str(a.decl_type) for a in decl.arguments]
                        libc_fullargs = ",".join(temp) # only types
                        location = decl.location.file_name
                        arg_names = ",".join([arg.name for arg in decl.arguments])
                        specifier = "" if decl.does_throw else "noexcept"
                        has_ellipsis = decl.has_ellipsis

                # if "..." in libc_fullargs:
                    if has_ellipsis:
                        handle_va_list = """
                        va_list __dce_va_list;
                        va_start (__dce_va_list, {justbeforelastarg});
                        """.format(justbeforelastarg= decl.arguments[-2].name)
                        # also we need to change arg names
                        arg_names = ",".join([arg.name for arg in decl.arguments] + ["__dce_va_list"]) 

                # + " " + arg.name)
                    # here we generate the code
                    impl = template.format(
                            extern="",
                            ret=rtype,
                            fullargs=libc_fullargs,
                            name=name,
                            retstmt="return" if rtype is not "void" else "",
                            arg_names=arg_names,
                            handle_va_list="",
                        )

                    # then generate aliases for both natives and dce
                   #define weak_alias(name, aliasname) \
                    # if hasattr(row, "extra"):
                    # if len(row['extra']) > 0:
                        # print("EXTRA=", row["extra"])
                    for aliasname in row["extra"]:
                        print("alias=", aliasname)
                        if len(aliasname):
                        # TODO add the alias
                        # this is ok if at the end
                            tpl = "decltype ({name}) {aliasname} __attribute__ ((weak, alias (\"{name}\")));\n"
                            # the pragam requires the alias to be previously declared in clang ?!
                            # tpl = "#pragma weak {aliasname} = {name}"
                            # tpl = "extern __typeof ({name}) {aliasname} __attribute__ ((weak, alias (\"{name}\")));\n"
                            impl += tpl.format(
                                    aliasname=aliasname,
                                    name=name
                                    )

        # # # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));

                    if write_impl:
                        libc_fd.write(impl)

                    # now we generate dce-<FILE>.h content
                    #  
                    # generate only the dce overrides
                    content = ""
                    if row["type"] == "dce":

                        # declaration of dce_{libcfunc}
                        # TODO 
                        content = "{extern} {ret} dce_{name} ({fullargs}) {throw};\n".format(
                                extern="",
                                ret=rtype,
                                fullargs=libc_fullargs,
                                name=name,
                                throw=specifier,
                                )


                    items = locations.setdefault(location, [])
                    items.append(content)

            # Now we generate the header files
            for path, functions in locations.items():
                print("path=", path)
                (head, tail) = os.path.split(path)
                print("head/tail", head, "----" , tail)
                # filename = os.path.basename()
                # filename = "model"
                header = ""
                
                subfolder = ""
                for folder in ["sys", "net", "arpa"]:
                    if head.endswith(folder):
                        subfolder = folder
                        break

                    # tail = os.path.join("sys", tail)
                filename = os.path.join("model", subfolder, "dce-" + tail)
                header=os.path.join(subfolder, tail)
                print(filename)
                # TODO 
# + ".generated.h"
                # header = "model/dce-" + filename
                print("Header name=", filename)
                content = """
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_{guard}
#define DCE_HEADER_{guard}
// TODO add extern "C" ?
#include <{header}>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {{
#endif
                """.format(guard=header.upper().replace(".","_").replace("/","_"), header=header) #os.path.basename(tail))

                for proto in functions:
                    print(proto)
                    content += proto + "\n"

                content += """
#ifdef __cplusplus
}
#endif
#endif
"""
                if write_headers:
                    with open(filename, 'w+') as dst:

                        # print("content=", content)
                        dst.write(content)


def main():


    libc_filename = "model/libc.generated.cc"

    parser  = argparse.ArgumentParser()
    parser.add_argument('-d','--write-headers', action="store_true", default=False, 
            help="Write model/dce-* to files")
    parser.add_argument('-i','--write-impl', action="store_true", default=False,
            help="write %s" % libc_filename)
    parser.add_argument('-a','--write-all', action="store_true", default=False,
            help="Enables -i and -h")
    parser.add_argument('-r','--regen', action="store_true", default=False,
            help="TODO: Disable the cache")
    
    args, unknown = parser.parse_known_args ()
    
    g = Generator()
    g.parse("test.h")
# TODO call that with subprocess.
    # os.system("./gen_natives.sh")
  # redirect output
    output ="model/libc-ns3.h.tmp" 


    print(unknown)
    if len(unknown) > 0:
        for func in unknown:
            g.lookup(func)
        exit(0)

    with open(output, "w") as tmp:
        subprocess.call( [
            "gcc", "model/libc-ns3.h", "-E", "-P",  "-DNATIVE(name,...)=native,name,__VA_ARGS__",
            "-DDCE(name,...)=dce,name,__VA_ARGS__",
            ], stdout=tmp, stderr=sys.stdout)

    # libc-ns3.generated.tmp
    g.generate_wrappers(output, libc_filename, write_headers=args.write_headers or args.write_all, 
            write_impl=args.write_impl or args.write_all)

if __name__ == "__main__":
    main()
