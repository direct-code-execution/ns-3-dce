from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers


import pybindgen.settings
import warnings

class ErrorHandler(pybindgen.settings.ErrorHandler):
    def handle_error(self, wrapper, exception, traceback_):
        warnings.warn("exception %r in wrapper %s" % (exception, wrapper))
        return True
pybindgen.settings.error_handler = ErrorHandler()


import sys

def module_init():
    root_module = Module('dce', cpp_namespace='::ns3')
    root_module.add_include('"ns3/dce-module.h"')
    root_module.add_include('"ns3/dce-manager-helper.h"')
    root_module.add_include('"ns3/dce-application.h"')
    return root_module

def register_types(module):
    root_module = module.get_root()
    
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: ns3::Application [class]
    module.add_class('Application', import_from_module='ns.network')
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::ApplicationContainer [class]
    module.add_class('ApplicationContainer', import_from_module='ns.network')
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::AttributeValue [class]
    module.add_class('AttributeValue', import_from_module='ns.core')
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: ns3::DceApplication [class]
    module.add_class('DceApplication', parent=root_module['ns3::Application'])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::DceApplicationHelper [class]
    module.add_class('DceApplicationHelper', allow_subclassing=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::Object [class]
    module.add_class('Object', import_from_module='ns.core')
    module.add_class('Node', import_from_module='ns.network')
    module.add_class('NodeContainer', import_from_module='ns.network')
    
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::ProcStatus [class]
    module.add_class('ProcStatus')
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::DceManagerHelper [class]
    module.add_class('DceManagerHelper', parent=root_module['ns3::Object'])
    module.add_container('std::vector< std::string >', 'std::string', container_type=u'vector')
    module.add_container('std::vector< ns3::ProcStatus >', 'ns3::ProcStatus', container_type=u'vector')
    #module.add_container('std::vector< std::pair< std::string, std::string > >', 'std::pair< std::string, std::string >', container_type=u'vector')
    
    
    typehandlers.add_type_alias(u'unsigned int', u'ns3::uid_t')
    typehandlers.add_type_alias(u'unsigned int*', u'ns3::uid_t*')
    typehandlers.add_type_alias(u'unsigned int&', u'ns3::uid_t&')
    
    

def register_methods(root_module):
    register_Ns3Application_methods(root_module, root_module['ns3::Application'])
    register_Ns3ApplicationContainer_methods(root_module, root_module['ns3::ApplicationContainer'])
    register_Ns3AttributeValue_methods(root_module, root_module['ns3::AttributeValue'])
    register_Ns3DceApplication_methods(root_module, root_module['ns3::DceApplication'])
    register_Ns3DceApplicationHelper_methods(root_module, root_module['ns3::DceApplicationHelper'])
    register_Ns3Object_methods(root_module, root_module['ns3::Object'])
    register_Ns3ProcStatus_methods(root_module, root_module['ns3::ProcStatus'])
    register_Ns3DceManagerHelper_methods(root_module, root_module['ns3::DceManagerHelper'])
    return

def register_Ns3Application_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: ns3::Application::Application() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: ns3::Application::Application(ns3::Application const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::Application const &', 'arg0')])
    return

def register_Ns3ApplicationContainer_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::ApplicationContainer::ApplicationContainer() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::ApplicationContainer::ApplicationContainer(ns3::ApplicationContainer const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::ApplicationContainer const &', 'arg0')])
    return

def register_Ns3AttributeValue_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::AttributeValue::AttributeValue() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::AttributeValue::AttributeValue(ns3::AttributeValue const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::AttributeValue const &', 'arg0')])
    return

def register_Ns3DceApplication_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: ns3::DceApplication::DceApplication(ns3::DceApplication const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::DceApplication const &', 'arg0')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: ns3::DceApplication::DceApplication() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetArguments(std::vector<std::basic_string<char, std::char_traits<char>, std::allocator<char> >,std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > > args) [member function]
    cls.add_method('SetArguments', 
                   'void', 
                   [param('std::vector< std::string >', 'args')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetBinary(std::string filename) [member function]
    cls.add_method('SetBinary', 
                   'void', 
                   [param('std::string', 'filename')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetEgid(unsigned int i) [member function]
    cls.add_method('SetEgid', 
                   'void', 
                   [param('unsigned int', 'i')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetEnvironment(std::vector<std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::basic_string<char, std::char_traits<char>, std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::basic_string<char, std::char_traits<char>, std::allocator<char> > > > > envs) [member function]
    cls.add_method('SetEnvironment', 
                   'void', 
                   [param('std::vector< std::pair< std::string, std::string > >', 'envs')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetEuid(unsigned int i) [member function]
    cls.add_method('SetEuid', 
                   'void', 
                   [param('unsigned int', 'i')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetGid(unsigned int i) [member function]
    cls.add_method('SetGid', 
                   'void', 
                   [param('unsigned int', 'i')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetStackSize(uint32_t stackSize) [member function]
    cls.add_method('SetStackSize', 
                   'void', 
                   [param('uint32_t', 'stackSize')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetStdinFile(std::string filename) [member function]
    cls.add_method('SetStdinFile', 
                   'void', 
                   [param('std::string', 'filename')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application.h: void ns3::DceApplication::SetUid(unsigned int i) [member function]
    cls.add_method('SetUid', 
                   'void', 
                   [param('unsigned int', 'i')])
    return

def register_Ns3DceApplicationHelper_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::DceApplicationHelper::DceApplicationHelper(ns3::DceApplicationHelper const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::DceApplicationHelper const &', 'arg0')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::DceApplicationHelper::DceApplicationHelper() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArgument(std::string arg) [member function]
    cls.add_method('AddArgument', 
                   'void', 
                   [param('std::string', 'arg')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArguments(std::string a0, std::string a1) [member function]
    cls.add_method('AddArguments', 
                   'void', 
                   [param('std::string', 'a0'), param('std::string', 'a1')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArguments(std::string a0, std::string a1, std::string a2) [member function]
    cls.add_method('AddArguments', 
                   'void', 
                   [param('std::string', 'a0'), param('std::string', 'a1'), param('std::string', 'a2')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArguments(std::string a0, std::string a1, std::string a2, std::string a3) [member function]
    cls.add_method('AddArguments', 
                   'void', 
                   [param('std::string', 'a0'), param('std::string', 'a1'), param('std::string', 'a2'), param('std::string', 'a3')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArguments(std::string a0, std::string a1, std::string a2, std::string a3, std::string a4) [member function]
    cls.add_method('AddArguments', 
                   'void', 
                   [param('std::string', 'a0'), param('std::string', 'a1'), param('std::string', 'a2'), param('std::string', 'a3'), param('std::string', 'a4')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArguments(std::string a0, std::string a1, std::string a2, std::string a3, std::string a4, std::string a5) [member function]
    cls.add_method('AddArguments', 
                   'void', 
                   [param('std::string', 'a0'), param('std::string', 'a1'), param('std::string', 'a2'), param('std::string', 'a3'), param('std::string', 'a4'), param('std::string', 'a5')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArguments(std::string a0, std::string a1, std::string a2, std::string a3, std::string a4, std::string a5, std::string a6) [member function]
    cls.add_method('AddArguments', 
                   'void', 
                   [param('std::string', 'a0'), param('std::string', 'a1'), param('std::string', 'a2'), param('std::string', 'a3'), param('std::string', 'a4'), param('std::string', 'a5'), param('std::string', 'a6')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddArguments(std::string a0, std::string a1, std::string a2, std::string a3, std::string a4, std::string a5, std::string a6, std::string a7) [member function]
    cls.add_method('AddArguments', 
                   'void', 
                   [param('std::string', 'a0'), param('std::string', 'a1'), param('std::string', 'a2'), param('std::string', 'a3'), param('std::string', 'a4'), param('std::string', 'a5'), param('std::string', 'a6'), param('std::string', 'a7')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::AddEnvironment(std::string name, std::string value) [member function]
    cls.add_method('AddEnvironment', 
                   'void', 
                   [param('std::string', 'name'), param('std::string', 'value')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::ApplicationContainer ns3::DceApplicationHelper::Install(ns3::NodeContainer c) [member function]
    cls.add_method('Install', 
                   'ns3::ApplicationContainer', 
                   [param('ns3::NodeContainer', 'c')], 
                   is_virtual=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: ns3::ApplicationContainer ns3::DceApplicationHelper::InstallInNode(ns3::Ptr<ns3::Node> node) [member function]
    cls.add_method('InstallInNode', 
                   'ns3::ApplicationContainer', 
                   [param('ns3::Node *', 'node', transfer_ownership=False)], 
                   is_virtual=False)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::ParseArguments(std::string args) [member function]
    cls.add_method('ParseArguments', 
                   'void', 
                   [param('std::string', 'args')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::ResetArguments() [member function]
    cls.add_method('ResetArguments', 
                   'void', 
                   [])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::ResetEnvironment() [member function]
    cls.add_method('ResetEnvironment', 
                   'void', 
                   [])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::SetBinary(std::string filename) [member function]
    cls.add_method('SetBinary', 
                   'void', 
                   [param('std::string', 'filename')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::SetEgid(ns3::uid_t i) [member function]
    cls.add_method('SetEgid', 
                   'void', 
                   [param('ns3::uid_t', 'i')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::SetEuid(ns3::uid_t i) [member function]
    cls.add_method('SetEuid', 
                   'void', 
                   [param('ns3::uid_t', 'i')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::SetGid(ns3::uid_t i) [member function]
    cls.add_method('SetGid', 
                   'void', 
                   [param('ns3::uid_t', 'i')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::SetStackSize(uint32_t stackSize) [member function]
    cls.add_method('SetStackSize', 
                   'void', 
                   [param('uint32_t', 'stackSize')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::SetStdinFile(std::string filename) [member function]
    cls.add_method('SetStdinFile', 
                   'void', 
                   [param('std::string', 'filename')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-application-helper.h: void ns3::DceApplicationHelper::SetUid(ns3::uid_t i) [member function]
    cls.add_method('SetUid', 
                   'void', 
                   [param('ns3::uid_t', 'i')])
    return

def register_Ns3Object_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::Object::Object() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::Object::Object(ns3::Object const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::Object const &', 'arg0')])
    return

def register_Ns3ProcStatus_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::ProcStatus::ProcStatus(ns3::ProcStatus const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::ProcStatus const &', 'arg0')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::ProcStatus::ProcStatus() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::ProcStatus::ProcStatus(int n, int e, int p, int64_t ns, int64_t ne, long int rs, long int re, double nd, long int rd, std::string cmd) [constructor]
    cls.add_constructor([param('int', 'n'), param('int', 'e'), param('int', 'p'), param('int64_t', 'ns'), param('int64_t', 'ne'), param('long int', 'rs'), param('long int', 're'), param('double', 'nd'), param('long int', 'rd'), param('std::string', 'cmd')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: std::string ns3::ProcStatus::GetCmdLine() const [member function]
    cls.add_method('GetCmdLine', 
                   'std::string', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: int ns3::ProcStatus::GetExitCode() const [member function]
    cls.add_method('GetExitCode', 
                   'int', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: int ns3::ProcStatus::GetNode() const [member function]
    cls.add_method('GetNode', 
                   'int', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: int ns3::ProcStatus::GetPid() const [member function]
    cls.add_method('GetPid', 
                   'int', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: long int ns3::ProcStatus::GetRealDuration() const [member function]
    cls.add_method('GetRealDuration', 
                   'long int', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: long int ns3::ProcStatus::GetRealEndTime() const [member function]
    cls.add_method('GetRealEndTime', 
                   'long int', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: long int ns3::ProcStatus::GetRealStartTime() const [member function]
    cls.add_method('GetRealStartTime', 
                   'long int', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: double ns3::ProcStatus::GetSimulatedDuration() const [member function]
    cls.add_method('GetSimulatedDuration', 
                   'double', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: int64_t ns3::ProcStatus::GetSimulatedEndTime() const [member function]
    cls.add_method('GetSimulatedEndTime', 
                   'int64_t', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: int64_t ns3::ProcStatus::GetSimulatedStartTime() const [member function]
    cls.add_method('GetSimulatedStartTime', 
                   'int64_t', 
                   [], 
                   is_const=True)
    return

def register_Ns3DceManagerHelper_methods(root_module, cls):
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::DceManagerHelper::DceManagerHelper(ns3::DceManagerHelper const & arg0) [copy constructor]
    cls.add_constructor([param('ns3::DceManagerHelper const &', 'arg0')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: ns3::DceManagerHelper::DceManagerHelper() [constructor]
    cls.add_constructor([])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: static std::vector<ns3::ProcStatus,std::allocator<ns3::ProcStatus> > ns3::DceManagerHelper::GetProcStatus() [member function]
    cls.add_method('GetProcStatus', 
                   'std::vector< ns3::ProcStatus >', 
                   [], 
                   is_static=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: std::string ns3::DceManagerHelper::GetVirtualPath() const [member function]
    cls.add_method('GetVirtualPath', 
                   'std::string', 
                   [], 
                   is_const=True)
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::Install(ns3::NodeContainer nodes) [member function]
    cls.add_method('Install', 
                   'void', 
                   [param('ns3::NodeContainer', 'nodes')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::SetAttribute(std::string n1, ns3::AttributeValue const & v1) [member function]
    cls.add_method('SetAttribute', 
                   'void', 
                   [param('std::string', 'n1'), param('ns3::AttributeValue const &', 'v1')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::SetDelayModel(std::string type, std::string n0="", ns3::AttributeValue const & v0=ns3::EmptyAttributeValue(), std::string n1="", ns3::AttributeValue const & v1=ns3::EmptyAttributeValue()) [member function]
    '''
    cls.add_method('SetDelayModel', 
                   'void', 
                   [param('std::string', 'type'), param('std::string', 'n0', default_value='""'), param('ns3::AttributeValue const &', 'v0', default_value='ns3::EmptyAttributeValue()'), param('std::string', 'n1', default_value='""'), param('ns3::AttributeValue const &', 'v1', default_value='ns3::EmptyAttributeValue()')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::SetLoader(std::string type) [member function]
    cls.add_method('SetLoader', 
                   'void', 
                   [param('std::string', 'type')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::SetNetworkStack(std::string type, std::string n0="", ns3::AttributeValue const & v0=ns3::EmptyAttributeValue()) [member function]
    cls.add_method('SetNetworkStack', 
                   'void', 
                   [param('std::string', 'type'), param('std::string', 'n0', default_value='""'), param('ns3::AttributeValue const &', 'v0', default_value='ns3::EmptyAttributeValue()')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::SetScheduler(std::string type, std::string n0="", ns3::AttributeValue const & v0=ns3::EmptyAttributeValue(), std::string n1="", ns3::AttributeValue const & v1=ns3::EmptyAttributeValue()) [member function]
    cls.add_method('SetScheduler', 
                   'void', 
                   [param('std::string', 'type'), param('std::string', 'n0', default_value='""'), param('ns3::AttributeValue const &', 'v0', default_value='ns3::EmptyAttributeValue()'), param('std::string', 'n1', default_value='""'), param('ns3::AttributeValue const &', 'v1', default_value='ns3::EmptyAttributeValue()')])
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::SetTaskManagerAttribute(std::string n0, ns3::AttributeValue const & v0) [member function]
    cls.add_method('SetTaskManagerAttribute', 
                   'void', 
                   [param('std::string', 'n0'), param('ns3::AttributeValue const &', 'v0')])
                   '''
    ## .//local/home/epmancini/dce_py/source/ns-3-dce-py/bindings/python/refh/dce-manager-helper.h: void ns3::DceManagerHelper::SetVirtualPath(std::string p) [member function]
    cls.add_method('SetVirtualPath', 
                   'void', 
                   [param('std::string', 'p')])
    return

def register_functions(root_module):
    module = root_module
    return

def main():
    out = FileCodeSink(sys.stdout)
    root_module = module_init()
    register_types(root_module)
    register_methods(root_module)
    register_functions(root_module)
    root_module.generate(out)

if __name__ == '__main__':
    main()

