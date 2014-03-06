.. include:: replace.txt

Python Bindings
***************

This section describes how generate the DCE Python bindings. The intended audience
are the DCE developers, not the users willing to make simulations. People not
interested in adding new public API can ignore this section, and read the chapter 
:ref:`python_scripts-label`.

Waf configuration scripts generate the Python bindings in a semi-automatic way. 
They use `PyBindGen <http://packages.python.org/PyBindGen>`_
to generate a Python script template. This script needs to be manually updated. 
This intermediate
script will generate a C++ source file that can be then compiled as a shared object,
installed and imported by the Python scripts.

Step by Step 
############

Step 1: Api scan
----------------

In this step, ``Waf`` calls PyBindGen to analyze the DCE public reference headers, 
in order to generate a temporary template Python file, that will generate a C++ file.

.. code-block:: sh

    ./waf --apiscan


Step 2: Pybindgen script update
-------------------------------

The Pyhon script generated in the first step needs to be adjusted and renamed as
``ns3_module_dce.py``. In particular to reduce the coupling with the ns-3 
installation the waf configuration has been simplified. For this reason you
need to paste the correct references for the ns-3 exported symbols, as for example:

.. code-block:: python

    module.add_class('Object', import_from_module='ns.core')
    module.add_class('Node', import_from_module='ns.network')
    module.add_class('NodeContainer', import_from_module='ns.network')


Step 3: C++ source code generation
----------------------------------

``ns3_module_dce.py`` can directly generate a C++ file for the Python module:
``ns3_module_dce.cpp``. It should be added to the code revision system
and included in the distribution. It just requires an installed recent version
of `Pybindgen <http://packages.python.org/PyBindGen>`_.

Step 4: Compilation
-------------------

In the compilation phase, ``wscript`` (the Waf installation module), compiles
``ns3_module_dce.cpp`` and generate a shared object called ``dce.so``. This is
the Python binding module.  

.. code-block:: sh

    ./waf
    ./waf install


Configuration parameters
########################
waf configuration flags:

  --disable-python                  Don't build Python bindings.
  --apiscan                         Rescan the API for Python bindings. Needs working GCCXML / pygccxml environment.
  --with-pybindgen=WITH_PYBINDGEN   Path to an existing `Pybindgen <http://packages.python.org/PyBindGen>`_ source tree to use.
  --with-python=WITH_PYTHON         Path to an existing Pyhton installation


