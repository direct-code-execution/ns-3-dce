#!/usr/bin/env python

import sys
import os, errno

import pybindgen
from pybindgen import FileCodeSink
import pybindgen.settings
from pybindgen.gccxmlparser import ModuleParser, PygenClassifier, PygenSection, WrapperWarning
from pybindgen.typehandlers.codesink import FileCodeSink
from pygccxml.declarations import templates
from pygccxml.declarations.class_declaration import class_t
from pygccxml.declarations.calldef import free_function_t, member_function_t, constructor_t, calldef_t

## we need the smart pointer type transformation to be active even
## during gccxml scanning.
import ns3modulegen_core_customizations

includes_dce = [
  #'dce-application.h',
  #'dce-application-helper.h',
  #'dce-manager-helper.h',
  #'ipv4-dce-routing-helper.h',
  #'linux-stack-helper.h',
  'ccn-client-helper.h'
]


type_annotations = {
    '::ns3::AttributeChecker': {
        'automatic_type_narrowing': 'true',
        'allow_subclassing': 'false',
        },
    '::ns3::AttributeValue': {
        'automatic_type_narrowing': 'true',
        'allow_subclassing': 'false',
        },

    '::ns3::CommandLine': {
        'allow_subclassing': 'true', # needed so that AddValue is able to set attributes on the object
        },

    '::ns3::NscTcpL4Protocol': {
        'ignore': 'true', # this class is implementation detail
        },


    'ns3::RandomVariable::RandomVariable(ns3::RandomVariableBase const & variable) [constructor]': {
        'ignore': None,
        },
    'ns3::RandomVariableBase * ns3::RandomVariable::Peek() const [member function]': {
        'ignore': None,
        },
    'void ns3::RandomVariable::GetSeed(uint32_t * seed) const [member function]': {
        'params': {'seed':{'direction':'out',
                           'array_length':'6'}}
        },
    'bool ns3::TypeId::LookupAttributeByName(std::string name, ns3::TypeId::AttributeInformation * info) const [member function]': {
        'params': {'info':{'transfer_ownership': 'false'}}
        },
    'static bool ns3::TypeId::LookupByNameFailSafe(std::string name, ns3::TypeId * tid) [member function]': {
        'ignore': None, # manually wrapped in 
        },
    'bool ns3::TraceSourceAccessor::ConnectWithoutContext(ns3::ObjectBase * obj, ns3::CallbackBase const & cb) const [member function]': {
        'params': {'obj': {'transfer_ownership':'false'}}
        },
    'bool ns3::TraceSourceAccessor::Connect(ns3::ObjectBase * obj, std::string context, ns3::CallbackBase const & cb) const [member function]': {
        'params': {'obj': {'transfer_ownership':'false'}}
        },
    'bool ns3::TraceSourceAccessor::DisconnectWithoutContext(ns3::ObjectBase * obj, ns3::CallbackBase const & cb) const [member function]': {
        'params': {'obj': {'transfer_ownership':'false'}}
        },
    'bool ns3::TraceSourceAccessor::Disconnect(ns3::ObjectBase * obj, std::string context, ns3::CallbackBase const & cb) const [member function]': {
        'params': {'obj': {'transfer_ownership':'false'}}
        },
    'bool ns3::AttributeAccessor::Set(ns3::ObjectBase * object, ns3::AttributeValue const & value) const [member function]': {
        'params': {'object': {'transfer_ownership':'false'}}
        },
    'ns3::EmpiricalVariable::EmpiricalVariable(ns3::RandomVariableBase const & variable) [constructor]': {
        'ignore': None
        },
    'static ns3::AttributeList * ns3::AttributeList::GetGlobal() [member function]': {
        'caller_owns_return': 'false'
        },
    'void ns3::CommandLine::Parse(int argc, char * * argv) const [member function]': {
        'ignore': None # manually wrapped
        },
    'extern void ns3::PythonCompleteConstruct(ns3::Ptr<ns3::Object> object, ns3::TypeId typeId, ns3::AttributeList const & attributes) [free function]': {
        'ignore': None # used transparently by, should not be wrapped
        },

    'ns3::Ptr<ns3::Ipv4RoutingProtocol> ns3::Ipv4ListRouting::GetRoutingProtocol(uint32_t index, int16_t & priority) const [member function]': {
        'params': {'priority':{'direction':'out'}}
        },
    'ns3::Ipv4RoutingTableEntry * ns3::GlobalRouter::GetInjectedRoute(uint32_t i) [member function]': {
        'params': {'return': { 'caller_owns_return': 'false',}},
        },
    'ns3::Ipv4RoutingTableEntry * ns3::Ipv4GlobalRouting::GetRoute(uint32_t i) const [member function]': {
        'params': {'return': { 'caller_owns_return': 'false',}},
        },

    '::ns3::TestCase': {
        'ignore': 'true', # we don't need to write test cases in Python
        },
    '::ns3::TestRunner': {
        'ignore': 'true', # we don't need to write test cases in Python
        },
    '::ns3::TestSuite': {
        'ignore': 'true', # we don't need to write test cases in Python
        },


    '::ns3::Ptr': {
        'import_from_module': 'ns.core', # Already defined in ns-3 bindings
        },
    '::ns3::ProcStatus::ProcStatus(int n, int e, int p, int64_t ns, int64_t ne, long rs, long re, double nd, long rd, std::string cmd)  [constructor]': {
        'ignore': None,
        },
    '::DceApplication::SetEnvironment (std::vector<std::pair<std::string,std::string>> envs)  [member function]': {
        'params': {'envs':{'direction':'in'}}
        },

    # Classes from ns-3
    '::ns3::Object': {
        'import_from_module': 'ns.core', # Already defined in ns-3 bindings
        },
    '::ns3::AttributeValue': {
        'import_from_module': 'ns.core', # Already defined in ns-3 bindings
        },
                    

    '::ns3::Node': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        },
    '::ns3::NodeContainer': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        },
    '::ns3::Ipv4Address': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        },
    '::ns3::Ipv4Mask': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        },
    '::ns3::Address': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        },
    '::ns3::Application': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        #'ignore': 'true',
        },
    '::ns3::ApplicationContainer': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        },
    '::ns3::EmptyAttributeValue': {
        'ignore': 'true',
        },
                    
    '::ns3::Ipv4AddressHash': {
        'ignore': 'true', # we don't need to write test cases in Python
        },
    '::ns3::Ipv6AddressHash': {
        'ignore': 'true', # we don't need to write test cases in Python
        },
    '::ns3::Ipv4StaticRoutingHelper': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        #'ignore': 'true',
        },
    '::ns3::Ipv4RoutingProtocol': {
        'import_from_module': 'ns.network', # Already defined in ns-3 bindings
        #'ignore': 'true',
        },
                    
                    
    'void ns3::DceApplicationHelper::InstallInNode(Ptr<Node> node) [member function]': {
        'params': {'node':{'direction':'in'}},
        },                    

    }

DCE_INCLUDE_PATH = "."


'''
'''
def pre_scan_hook(dummy_module_parser,
                  pygccxml_definition,
                  global_annotations,
                  parameter_annotations):
    #ns3_header = get_ns3_relative_path(pygccxml_definition.location.file_name)
    ns3_header = DCE_INCLUDE_PATH + "/" +pygccxml_definition.location.file_name
    '''
    print " ********";
    print " ** [pre_scan_hook] pygccxml_definition:"+str(pygccxml_definition);
    attrs = vars(pygccxml_definition)
    print '\n   - '.join("%s: %s" % item for item in attrs.items())

    attrs = vars(pygccxml_definition.location)
    print '  **** pygccxml_definition.location'
    print '\n      + '.join("%s: %s" % item for item in attrs.items())

    #attrs = vars(pygccxml_definition.arguments)
    #print '      pygccxml_definition.arguments'
    #print '\n      | '.join("%s: %s" % item for item in attrs.items())
    
    #print " ** [pre_scan_hook] pygccxml_definition:"+pygccxml_definition;
    #print " ** [pre_scan_hook] pygccxml_definition.demangled_name:"+str(pygccxml_definition.demangled_name);
    print " ** [pre_scan_hook] pygccxml_definition.location:"+str(pygccxml_definition.location);
    
    print " ** [pre_scan_hook] ns3_header:"+ns3_header;
    print " ** [pre_scan_hook] global_annotations:"+str(global_annotations);
    print " ** [pre_scan_hook] parameter_annotations:"+str(parameter_annotations);
    '''

    ## Note: we don't include line numbers in the comments because
    ## those numbers are very likely to change frequently, which would
    ## cause needless changes, since the generated python files are
    ## kept under version control.

    #global_annotations['pygen_comment'] = "%s:%i: %s" % \
    #    (ns3_header, pygccxml_definition.location.line, pygccxml_definition)
    global_annotations['pygen_comment'] = "%s: %s" % \
        (ns3_header, pygccxml_definition)


    ## handle ns3::Object::GetObject (left to its own devices,
    ## pybindgen will generate a mangled name containing the template
    ## argument type name).
    if isinstance(pygccxml_definition, member_function_t) \
            and pygccxml_definition.parent.name == 'Object' \
            and pygccxml_definition.name == 'GetObject':
        template_args = templates.args(pygccxml_definition.demangled_name)
        if template_args == ['ns3::Object']:
            global_annotations['template_instance_names'] = 'ns3::Object=>GetObject'

    ## Don't wrap Simulator::Schedule* (manually wrapped)
    if isinstance(pygccxml_definition, member_function_t) \
            and pygccxml_definition.parent.name == 'Simulator' \
            and pygccxml_definition.name.startswith('Schedule'):
        global_annotations['ignore'] = None

    # manually wrapped
    if isinstance(pygccxml_definition, member_function_t) \
            and pygccxml_definition.parent.name == 'Simulator' \
            and pygccxml_definition.name == 'Run':
        global_annotations['ignore'] = True

    ## http://www.gccxml.org/Bug/view.php?id=9915
    if isinstance(pygccxml_definition, calldef_t):
        for arg in pygccxml_definition.arguments:
            if arg.default_value is None:
                continue
            if "ns3::MilliSeconds( )" == arg.default_value:
                arg.default_value = "ns3::MilliSeconds(0)"
            if "ns3::Seconds( )" == arg.default_value:
                arg.default_value = "ns3::Seconds(0)"

    ## classes
    if isinstance(pygccxml_definition, class_t):
        # no need for helper classes to allow subclassing in Python, I think...
        #if pygccxml_definition.name.endswith('Helper'):
        #    global_annotations['allow_subclassing'] = 'false'

        if pygccxml_definition.decl_string.startswith('::ns3::SimpleRefCount<'):
            global_annotations['incref_method'] = 'Ref'
            global_annotations['decref_method'] = 'Unref'
            global_annotations['peekref_method'] = 'GetReferenceCount'
            global_annotations['automatic_type_narrowing'] = 'true'
            return

        if pygccxml_definition.decl_string.startswith('::ns3::Callback<'):
            # manually handled in ns3modulegen_core_customizations.py
            global_annotations['ignore'] = None
            return

        if pygccxml_definition.decl_string.startswith('::ns3::TracedCallback<'):
            global_annotations['ignore'] = None
            return

        if pygccxml_definition.decl_string.startswith('::ns3::Ptr<'):
            # handled by pybindgen "type transformation"
            global_annotations['ignore'] = None
            return

        # table driven class customization
        try:
            annotations = type_annotations[pygccxml_definition.decl_string]
        except KeyError:
            pass
        else:
            global_annotations.update(annotations)

    ## free functions
    if isinstance(pygccxml_definition, free_function_t):
        if pygccxml_definition.name == 'PeekPointer':
            global_annotations['ignore'] = None
            return

    ## table driven methods/constructors/functions customization
    if isinstance(pygccxml_definition, (free_function_t, member_function_t, constructor_t)):
        try:
            annotations = type_annotations[str(pygccxml_definition)]
        except KeyError:
            pass
        else:
            for key,value in annotations.items():
                if key == 'params':
                    parameter_annotations.update (value)
                    del annotations['params']
            global_annotations.update(annotations)



def dcepy_module_gen( binddir, ns3path, dcepath ):
    DCE_INCLUDE_PATH=dcepath
    '''
    print "************************* dcepy_module_gen"
    print "* binddir = " + binddir
    print "* ns3path = " + ns3path
    print "* dcepath = " + dcepath
    print "******************************************"
    '''
    
    cflags = ''
    
    bldpath = 'bindings/python'
    try:
        os.makedirs(bldpath)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(bldpath):
            pass
        else: raise

    ref_header_dir = binddir+"/refh"
    gccxml_options = dict(
        #include_paths=[ns3path, dcepath+"/model", dcepath + "/helper"],
        include_paths=[ref_header_dir, ns3path ],
        define_symbols={
            #'NS3_ASSERT_ENABLE': None,
            #'NS3_LOG_ENABLE': None,
            },
	    cflags=('--gccxml-cxxflags "%s -DPYTHON_SCAN"' % cflags)
    )
    
    inclfiles = []
    for hf in includes_dce:
      inclfiles.append( ref_header_dir+"/"+hf  )
      #inclfiles.append( dcepath+"/model/"+hf  )
      
      
    #whitelist_paths=[ dcepath+"/model", dcepath + "/helper", ns3path  ]
    whitelist_paths=[ ref_header_dir  ]

    module_parser = ModuleParser('dce', 'ns3')
    module_parser.enable_anonymous_containers = True
    module_parser.add_pre_scan_hook(pre_scan_hook)

    generatepyintermediate = True
    if generatepyintermediate:
        # Test with intermediate file
        fname = bldpath + '/temp_dce_bindings.py'
        print "Generating python pygendbind intermediate file: "+str(fname)
        py_file = open( fname, "wt")
        includes = ['"ns3/dce-module.h"', '"ns3/dce-manager-helper.h"', '"ns3/dce-application.h"', '"ns3/ipv4-dce-routing-helper.h"']
        pysink = FileCodeSink(py_file)
        module_parser.parse_init(inclfiles, whitelist_paths=whitelist_paths, pygen_sink=pysink, gccxml_options=gccxml_options, includes=includes)
        module_parser.scan_types()
        module_parser.scan_methods()
        module_parser.scan_functions()
        module_parser.parse_finalize()          

    else:
        # Test with cpp
        fname = bldpath + '/temp_dce_bindings.cpp'
        #fname = 'dce_bindings.cpp'
        print "Generating python bindings c++ file: "+str(fname)
        pygen_file = open( fname, "wt")
        module_parser.parse_init(inclfiles, whitelist_paths=whitelist_paths, gccxml_options=gccxml_options)

        module_parser.scan_types()
        module_parser.scan_methods()
        module_parser.scan_functions()
        module_parser.parse_finalize()	      
        module_parser.module.add_include('<ns3/dce-module.h>')
        module_parser.module.add_include('<ns3/dce-manager-helper.h>')
        module_parser.module.add_include('<ns3/dce-application.h>')
        module_parser.module.add_include('<ns3/ipv4-dce-routing-helper.h>')
        module_parser.module.add_include('<ns3/linux-stack-helper.h>')
        pybindgen.write_preamble(FileCodeSink(pygen_file))
        module_parser.module.generate(FileCodeSink(pygen_file))
    


                                                                                                                                                               

