.. include:: replace.txt

Creating your protocol implementation as a DCE sub-module
=========================================================

If your application has a configuration file to modify the behavior of applications, introducing a particular Helper class will be helpful to handle your application. In this section, we will give you an advanced way of using your application with DCE.

Some of existing submodule are following this way. You can find `ns-3-dce-quagga <../../manual-quagga/html/index.html>`_ and `ns-3-dce-umip <../../manual-umip/html/index.html>`_ as examples to add sub-module.

Obtaining DCE sub-module template
---------------------------------
First of all, you could start with referring sub module template available as follows.

::

   hg clone http://code.nsnam.org/thehajime/ns-3-dce-submodule (your module name)


The template consists of, wscript, helper, test and documentation. You could rename all/some of them for your module.
Then, put ``ns-3-dce-submodule`` directory under ns-3-dce/myscripts/. This will be required to build under ns-3-dce module as an extension (sub-module) of dce.

Writing wscript
---------------

Writing bakeconf.xml (optional)
-------------------------------

Implementing helper class (optional)
------------------------------------

Writing examples (optional)
---------------------------



