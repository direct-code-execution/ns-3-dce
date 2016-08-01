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


# ns_namespace = global_namespace.namespace("ns")

int_type = declarations.cpptypes.int_t()
double_type = declarations.cpptypes.double_t()

# for decl in global_namespace.declarations:
#     # print(dir(decl))
#     # print(decl)
#     if isinstance(decl, declarations.free_function_t):
#         print(decl)
#         print("DECL_STRING=", decl.decl_string)
#         print("function_type=", decl.function_type)
#         print("PARTIAL_NAME=", decl.partial_name)
#         print("NAME=", decl.name)
        # print("required args=", decl.required_args)
        # print("calling convention=", decl.calling_convention)



# list of exceptions for functions thatp ygccxml fail to identify correctly
# hack around https://github.com/gccxml/pygccxml/issues/62
ExplicitFn = namedtuple('ExplicitFn', ["rtype", "fullargs", "arg_names", "location"])
exceptions = {
    "sysinfo": ExplicitFn("int", "struct sysinfo *info", "info","/usr/include/x86_64-linux-gnu/sys/sysinfo.h"),
    "sigaction": ExplicitFn("int", "int signum, const struct sigaction *act, struct sigaction *oldact", "signum, act, oldact","/usr/include/signal.h"),
    "wait": ExplicitFn("pid_t", "void *stat_loc", "stat_loc", "/usr/include/x86_64-linux-gnu/sys/wait.h"),
    "__fpurge": ExplicitFn("void", "FILE *fd", "fd", "/usr/include/stdio.h"),
    "__fpending": ExplicitFn("size_t", "FILE *fd", "fd", "/usr/include/stdio.h"),
    "fstat64": ExplicitFn("int", "int __fd, struct stat64 *__buf", "__fd, __buf", "/usr/include/x86_64-linux-gnu/sys/stat.h"),
    "pthread_kill": ExplicitFn("int", "pthread_t thread, int sig", "thread, sig", "/usr/include/signal.h"),
    "uname": ExplicitFn("int", "struct utsname *__name", "__name", "/usr/include/x86_64-linux-gnu/sys/utsname.h"),
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
            content_type=parser.CONTENT_TYPE.CACHED_SOURCE_FILE
            )

        self.project_reader = parser.project_reader_t(xml_generator_config)
        self.decls = self.project_reader.read_files(
            [file_config],
            compilation_mode=parser.COMPILATION_MODE.FILE_BY_FILE)



    def generate_alias(self, aliasname, decl):
                   #define weak_alias(name, aliasname) \
  # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));
        return ""

    def generate_wrappers(self, input_filename, libc_filename, write_headers : bool):
        """
        Generate wrappers + headers
        """

        # input_filename = "natives.h.txt"
        global_namespace = declarations.get_global_namespace(self.decls)

        locations = {}
        with open(input_filename, "r") as src:
            # aliasnames = last columns ?
            reader = csv.DictReader(src, fieldnames=["type","name"], restkey="extra")
            with open(libc_filename, "w+") as libc_fd:
                # for line in src:
                for row in reader:
                    # function_name = line.rstrip()
                    # print(line)
                    if row["type"] == "alias":
# or row["type"] == "dce":
                        continue

                    # look for a match
                    print('row["name"]=', row["name"])    
                    # decl = global_namespace.free_function(name=row["name"])  
                   # Search for the function by name

                   # if row["name"] in ["sigaction", "sysinfo"]

                    criteria = declarations.calldef_matcher(name=row["name"])
                    results = declarations.matcher.find(criteria, global_namespace) 

                    # print("decl", results)
                    decl = results[0]
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
                    #     print("a", a.decl_type )
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
                        temp.append(s)

                    for arg in temp:
                        print("arg=%s"% arg)
                    # temp = ["va_list" else str(a.decl_type) for a in decl.arguments]
                    fullargs = ",".join(temp) # only types
                    location = decl.location.file_name
                    arg_names = ",".join([arg.name for arg in decl.arguments])



                # + " " + arg.name)
                    template = """
                    {extern} {ret} {name} ({fullargs}){{
                        {retstmt} g_libc.{name}_fn ({arg_names});
                    }}
                    """
                    res = ""
                    # hack around https://github.com/gccxml/pygccxml/issues/62
                    if row["name"] in exceptions.keys():
                        name = row["name"]
                        log.debug("Exception [%s] found " % name)
                        extern=""
                        rtype, fullargs , arg_names, location = exceptions[name]
                        # **exceptions[name]
                        print("Values:", rtype, fullargs, arg_names, location)

                    res = template.format(
                            extern="",
                            ret=rtype,
                            fullargs=fullargs,
                            name=name,
                            retstmt="return" if rtype is not "void" else "",
                            arg_names=arg_names,
                        )


                    libc_fd.write(res)

                    # now we generate dce-<FILE>.h content
                    #  
                    # generate only the dce overrides
                    content = ""
                    if row["type"] == "dce":

                        # declaration of dce_{libcfunc}
                        content = "{extern} {ret} dce_{name} ({fullargs});\n".format(
                                extern="",
                                ret=rtype,
                                fullargs=fullargs,
                                name=name,
                                )

                    # then generate aliases for both natives and dce
                   #define weak_alias(name, aliasname) \
                    if hasattr(row, "extra"):
                        print("extra=", row["extra"])
                        for aliasname in row["extra"]:
                            print("alias=", aliasname)
                            # TODO add the alias
                            # content += "__typeof ({name}) {aliasname} __attribute__ ((weak, alias (# {name})));\n".format(
                            #         aliasname=aliasname,
                            #         name=name
                            #         )
    # # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));

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
                
                sys = "sys" if head.endswith("sys") else ""
                    # tail = os.path.join("sys", tail)
                filename = os.path.join("model", sys, "dce-" + tail)
                header=os.path.join(sys, tail)
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
                    with open(filename, "w+") as dst:

                        # print("content=", content)
                        dst.write(content)


def main():
    parser  = argparse.ArgumentParser()
    parser.add_argument('-w','--write', action="store_true", default=False, help="write output to files")
    args = parser.parse_args ()
    
    g = Generator()
    g.parse("test.h")
# TODO call that with subprocess.
    # os.system("./gen_natives.sh")
  # redirect output
    output ="model/libc-ns3.h.tmp" 
    with open(output, "w") as tmp:
        subprocess.call( [
            "gcc", "model/libc-ns3.h", "-E", "-P",  "-DNATIVE(name,...)=native,name,__VA_ARGS__",
            "-DDCE(name,...)=dce,name,__VA_ARGS__",
            ], stdout=tmp, stderr=sys.stdout)

    # libc-ns3.generated.tmp
    g.generate_wrappers(output, "model/libc.generated.cc", args.write)

if __name__ == "__main__":
    main()
