#!/usr/bin/env python3
# Example taken out of http://pygccxml.readthedocs.io/en/develop/examples/searching1/example.html
from pygccxml import utils
from pygccxml import declarations
from pygccxml import parser
from pygccxml.declarations import declaration_utils

# todo
def pprint(decl):
    # Get the full name of the calldef...
    name = declaration_utils.full_name(decl)
    if name[:2] == "::":
        name = name[2:]
    # Add the arguments...
    # default_value', 'ellipsis', 'name', 'type'
    # print(dir(decl.arguments[0]))
    args = [str(a) for a in decl.arguments]
    res = "%s(%s)" % (name, ", ".join(args))
    # Add the return type...
    if decl.return_type is not None:
        res = "%s %s" % (decl.return_type, res)
    # extern?
    if decl.has_extern:
        res = "extern " + res
    # Append the declaration class
    # cls = self.__class__.__name__
    # if cls[-2:] == "_t":
    #     cls = cls[:-2]
    # cls = cls.replace('_', ' ')
    # return "%s [%s]" % (res, cls)
    res += "{\n"
    res += "g_libc." + name + "_fn("
    res += ",".join([arg.name for arg in decl.arguments])
    res += ")\n}"

    print("%s " % (res, ))
def gen_wrapper_from_libc_name(func):
    # Get the full name of the calldef...
    name = declaration_utils.full_name(decl)
    if name[:2] == "::":
        name = name[2:]
    # Add the arguments...
    args = [str(a) for a in decl.arguments]
    res = "%s(%s)" % (name, ", ".join(args))
    # Add the return type...
    if decl.return_type is not None:
        res = "%s %s" % (decl.return_type, res)
    # extern?
    if decl.has_extern:
        res = "extern " + res
    # Append the declaration class
    # cls = self.__class__.__name__
    # if cls[-2:] == "_t":
    #     cls = cls[:-2]
    # cls = cls.replace('_', ' ')
    # return "%s [%s]" % (res, cls)
    print("%s " % (res, ))
    return "%s " % (res, )



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

# Parse the c++ file
decls = parser.parse([filename], xml_generator_config)

global_namespace = declarations.get_global_namespace(decls)

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

# criteria = declarations.calldef_matcher(name="printf")
# func1a = declarations.matcher.get_single(criteria, global_namespace)

with open("natives.h.txt", "r") as f:
    for line in f:
        line = line.rstrip()
        print(line)



        func1a = global_namespace.free_function(name=line)
        # print(dir(func1a))
        pprint(func1a)
# print(func1a.create_decl_string())

# This prints all the declarations in the namespace declaration tree:
# ns::a [variable]
# ns::b [variable]
# ns::c [variable]
# double ns::func2(double a) [free function]

# Let's search for specific declarations
# for decl in ns_namespace.declarations:
    # if decl.name == "b":
    # print(decl)
    # if isinstance(decl, declarations.free_function_t):
    #     print(decl)

# This prints:
# ns::b [variable]
# double ns::func2(double a) [free function]

