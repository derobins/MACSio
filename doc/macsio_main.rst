Main
----

The design of MACSio_ is described in detail in
:download:`an accompanying design document <macsio_design.pdf>`

MACSio_ is divided into two halves. The main program is responsible for managing the generation of data,
as well as other compute and/or communication workloads to be mixed with I/O workloads. A plugin does the
work of marshalling data between memory and disk. A given plugin is intended to represent the *best* approach
for handling the data for a given I/O library and parallel I/O paradigm. Any given performance test
involves the use of just one of the available plugins. Eventually, to model high-level workloads, it is
concievable MACSio_ could be extended to scriptify various of the workloads, including I/O with any given
plugin, it manages.

MACSio_'s command-line arguments are designed to give the user control over the nominal I/O request sizes
emitted from MPI ranks for mesh bulk data and for amorphous metadata. The user specifies a size, in bytes,
for mesh pieces. MACSio_ then computes a mesh part size, in nodes, necessary to hit this target byte count for
double precision data. MACSio_ will determine an N dimensional logical size of a mesh piece that is a close
to equal dimensional as possible. In addition, the user specifies an average number of mesh pieces that will be
assigned to each MPI rank. This does not have to be a whole number. When it is a whole number, each MPI rank
has the same number of mesh pieces. When it is not, some processors have one more mesh piece than others.
This is common of HPC multi-physics applications. Together, the total processor count and average number of
mesh pieces per processor gives a total number of mesh pieces that comprise the entire mesh. MACSio_ then
finds an N dimensional arrangement (N=[1,2,3]) of the pieces that is as close to equal dimension as possible.
If mesh piece size or total count of pieces wind up being prime numbers, MACSio_ will only be able to factor
these into long, narrow shapes where 2 (or 3) of the dimensions are of size 1. That will make examination of
the resulting data using visualization tools like VisIt a little less convenient but is otherwise harmless
from the perspective of driving and assessing I/O performance.

Building MACSio_
^^^^^^^^^^^^^^^^

Once json-cwx has been successfully installed,
CMake is used to build MACSio_ and any of the desired plugins (builds with silo by default)

.. code-block:: shell
   % mkdir build
   % cd build
   % cmake -DCMAKE_INSTALL_PREFIX=[desired-install-location] \
         -DWITH_JSON-CWX_PREFIX=[path to json-cwx] \
         -DWITH_SILO_PREFIX=[path to silo] ..
   % make
   % make install

NOTE: Some options for the cmake line:
  - Build docs:             ``-DBUILD_DOCS=ON``
  - Build HDF5 Plugin:      ``-DENABLE_HDF5_PLUGIN=ON -DWITH_HDF5_PREFIX=[path to hdf5]``
  - Build TyphonIO Plugin:  ``-DENABLE_TYPHONIO_PLUGIN=ON -DWITH_TYPHONIO_PREFIX=[path to typhonio]``
  - Build PDB Plugin:       ``-DENABLE_PBD_PLUGIN=ON``
  - Build Exodus Plugin:    ``-DENABLE_EXODUS_PLUGIN=ON -DWITH_EXODUS_PREFIX=[path to exodus]``

Although MACSio_ is C Language, at a minimum it must be linked using a C++ linker due to
its use of non-constant expressions in static initializers to affect the static plugin
behavior. However, its conceivable that some C++'isms have crept into the code causing
warnings or outright errors with some C compilers.

In addition, MACSio_ sources currently include a large number of ``#warning`` statements
to help remind developers of minor issues to be fixed. When compiling, these
produce a lot of sprurios output in ``stderr`` but are otherwise harmless.

