#!/usr/bin/env python3
# Example taken out of http://pygccxml.readthedocs.io/en/develop/examples/searching1/example.html
from pygccxml import utils
from pygccxml import declarations
from pygccxml import parser
from pygccxml.declarations import declaration_utils
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
# log.setLevel(logging.DEBUG)
log.addHandler(logging.StreamHandler())

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

            # asked on tracker to generate va_list but not ok
            # flags= ["f1"]
            )
        # config.flags = 
        # The c++ file we want to parse
        # printf is declared in stdio.h
        # filename = "/home/teto/glibc/libio/stdio.h"
        filename = "test.h"
        code = """
#include <signal.h>
        """

        self.project_reader = parser.project_reader_t(xml_generator_config)
        self.decls = self.project_reader.read_string(code)


    def generate_wrappers(self, input_filename, out_filename):
        """
        Generate wrappers + headers
        """

        # input_filename = "natives.h.txt"
        # out_filename = 
        global_namespace = declarations.get_global_namespace(self.decls)

        locations = {}

        # look for a match
        criteria = declarations.calldef_matcher(name="exit")
        results = declarations.matcher.find(criteria, global_namespace) 

        # print("decl", results)
        decl = results[0]
        # print( "islist ? len",len(func1))
        name = declaration_utils.full_name(decl)
        if name[:2] == "::":
            name = name[2:]
        log.info("Parsing function %s" % name)
        extern="extern" if decl.has_extern else ""
        rtype = "%s" % (decl.return_type if decl.return_type is not None else "void")

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
        fullargs = ",".join(temp)
        res = """
            {extern} {ret} {name} ({fullargs});

        """.format(
                extern=extern,
                ret=rtype,
                fullargs=fullargs,
                name=name,
                retstmt="return" if rtype is not "void" else "",
                arg_names=",".join([arg.name for arg in decl.arguments]),
        )
        print(res)



def main():
    parser  = argparse.ArgumentParser()
    g = Generator()
    g.parse("toto")
    g.generate_wrappers("temp", "model/libc.generated.cc")

if __name__ == "__main__":
    main()
