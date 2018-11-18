HDF5 Plugin
-----------

The `HDF5`_ plugin is designed to support both MIF_ and SSF parallel I/O paradigms.
In SSF mode, writes can be collective or independent. In MIF_ mode, only independent
writes are supported because each task creates its own datasets.

In addition, it supports gzip, szip and zfp compression but only in MIF_ mode.

Finally, there are a number of command-line arguments to the plugin that allow
control of a number of low-level `HDF5`_ features. More can be easily added.

There have been some recent additions to help improve scalable performance of the
HDF5_ library and support for these features are yet to have been added to the plugin.

There is a minor difference in the representation of the mesh and its field between
the MIF_ and SSF parallel I/O modes. In MIF_ mode, each task outputs a chunk of mesh
such that nodes on the exterior of the chunk are duplicates of the equivalent nodes
on neighboring chunks. However, in SSF mode, the whole mesh is assembled into a single,
monolithic whole mesh. The nodes that are duplicated in neighboring mesh chunks are
factored out of the actual write requests to HDF5 to avoid lower levels in the I/O 
stack from having to handle coincident writes.

Command-Line Arguments
^^^^^^^^^^^^^^^^^^^^^^

.. note:: Need to auto-gen the command-line args doc from the source code or a 
   run of the executable with --help


Dependencies
^^^^^^^^^^^^
To use either gzip or szip compression with the `HDF5`_ plugin, the HDF5 library
to which MACSio_ is linked must have been compiled with support for zlib and szip.

However, to use zfp compression with the `HDF5`_ plugin, all that is necessary is
that the the H5Z-ZFP filter be available to the `HDF5`_ library to load at run time
as any other filter plugin. The H5Z-ZFP filter must have been compatably compiled.
In addition, the environment variable, ``HDF5_PLUGIN_PATH`` set to include a 
directory containining the H5Z-ZFP filter plugin library. For example,

.. code-block:: shell

    env HDF5_PLUGIN_PATH=<path-to-plugin-dir> mpirun -np 4 ./macsio/macsio ...

.. doxygenfile:: macsio_hdf5.c

.. _HDF5 : https://www.hdfgroup.org/downloads/hdf5/
