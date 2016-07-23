#!/usr/bin/env python3
# Example taken out of http://pygccxml.readthedocs.io/en/develop/examples/searching1/example.html
from pygccxml import utils
from pygccxml import declarations
from pygccxml import parser
from pygccxml.declarations import declaration_utils
import os
import argparse
import csv


"""
Must be able to generate:
    - 

todo do same for dl/pthread/rt
"""




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






class Generator:
    def __init__(self):
        pass
    
    def parse(self, filename):
        """
        cache parsing
        """

        # Find the location of the xml generator (castxml or gccxml)
        generator_path, generator_name = utils.find_xml_generator()

        # Configure the xml generator
        xml_generator_config = parser.xml_generator_configuration_t(
            xml_generator_path=generator_path,
            xml_generator=generator_name)

        # The c++ file we want to parse
        # printf is declared in stdio.h
        # filename = "/home/teto/glibc/libio/stdio.h"
        filename = "test.h"

        file_config = parser.file_configuration_t(
            data=filename,
            content_type=parser.CONTENT_TYPE.CACHED_SOURCE_FILE)

        self.project_reader = parser.project_reader_t(xml_generator_config)
        self.decls = self.project_reader.read_files(
            [file_config],
            compilation_mode=parser.COMPILATION_MODE.FILE_BY_FILE)



    def generate_alias(self, aliasname, decl):
                   #define weak_alias(name, aliasname) \
  # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));
        return ""

    def generate_wrappers(self, input_filename, out_filename):
        """
        Generate wrappers + headers
        """

        # input_filename = "natives.h.txt"
        # out_filename = 
        global_namespace = declarations.get_global_namespace(self.decls)

        locations = {}
        with open(input_filename, "r") as src:
            # aliasnames = last columns ?
            reader = csv.DictReader(src, fieldnames=["type","name"], restkey="extra")
            with open(out_filename, "w+") as dst:
                # for line in src:
                for row in reader:
                    # function_name = line.rstrip()
                    # print(line)
                    if row["type"] == "alias":
# or row["type"] == "dce":
                        continue

                    # look for a match
                    print('row["name"]', row["name"])    
                    # decl = global_namespace.free_function(name=row["name"])  
                   # Search for the function by name
                    criteria = declarations.calldef_matcher(name=row["name"])
                    results = declarations.matcher.find(criteria, global_namespace) 

                    # print("decl", results)
                    decl = results[0]
                    # print( "islist ? len",len(func1))
                    name = declaration_utils.full_name(decl)
                    if name[:2] == "::":
                        name = name[2:]
                    print(name)
                    # Add the arguments...
                    # default_value', 'ellipsis', 'name', 'type'
                    # print(dir(decl.arguments[0]))
                    # TODO we should record the location
                    # locations.update ({ decl.location })
                    # {ret} {name} ({args}) 
                    # proto = "{extern} {ret} {name} ({args})".format(
                    extern="extern" if decl.has_extern else ""
                    rtype = "%s" % (decl.return_type if decl.return_type is not None else "void")
                    fullargs = " ".join([str(a) for a in decl.arguments])
                    res = """
                     {extern} {ret} {name} ({fullargs}){{
                        {retstmt} g_libc.{name}_fn ({arg_names});
                    }}
                    """.format(
                            extern=extern,
                            ret=rtype,
                            fullargs=fullargs,
                            name=name,
                            retstmt="return" if rtype is not "void" else "",
                            arg_names=",".join([arg.name for arg in decl.arguments]),
                    )

                    dst.write(res)

                    # now we generate dce-<FILE>.h content
                    # 
                    if row["type"] != "dce":
                        continue

                    # declaration of dce_{libcfunc}
                    content = "{extern} {ret} dce_{name} ({fullargs});\n".format(
                            extern=extern,
                            ret=rtype,
                            fullargs=fullargs,
                            name=name,
                            )

                    # then append aliases
                   #define weak_alias(name, aliasname) \
                    if hasattr(row, "extra"):
                        print("extra=", row["extra"])
                        for aliasname in row["extra"]:
                            print("alias=", aliasname)
                            # content += "__typeof ({name}) {aliasname} __attribute__ ((weak, alias (# {name})));\n".format(
                            #         aliasname=aliasname,
                            #         name=name
                            #         )
    # # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));

                    items = locations.setdefault(decl.location.file_name, [])
                    items.append(content)

            # Now we generate the header files
            for filename, functions in locations.items():
                filename = os.path.basename(filename)
                header = "tmp/dce-" + filename
                print("Header name=", header)
                with open(header, "w+") as dst:
                    content = """
                    /* GENERATED BY MATT */
                    """

                    for proto in functions:
                        print(proto)
                        content += proto + "\n"

                    # print("content=", content)
                    dst.write(content)


def main():
    parser  = argparse.ArgumentParser()
    g = Generator()
    g.parse("test.h")
# TODO call that with subprocess.
    subprocess.call( gcc "model/libc-ns3.h" -E -P -D'NATIVE(name,...)=native,name,__VA_ARGS__' -D'DCE(name,...)=dce,name,__VA_ARGS__' \
    g.generate_wrappers("natives.h.txt2", "model/libc.generated.cc")

if __name__ == "__main__":
    main()
