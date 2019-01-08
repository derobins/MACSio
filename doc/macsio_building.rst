Building MACSio_
----------------

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

