Command Line Argument Parsing
-----------------------------

The command line argument package is designed to do parsing of simple command-line arguments
and assign values associated with them either to caller-supplied scalar variables or to populate a
json_object. The work-horse function, :any:`MACSIO_CLARGS_ProcessCmdline`, is used in the following manner...

.. code-block:: shell
    ./macsio --help


.. code-block:: c

    int doMultifile, numCycles, Ni, Nj;
    double Xi, Xj;
    MACSIO_CLARGS_ArgvFlags_t flags;
    json_object *obj = 0;
    int argi = 0;

    flags.error_mode = MACSIO_CLARGS_ERROR;
    flags.route_mode = MACSIO_CLARGS_TOMEM;

    MACSIO_CLARGS_ProccessCmdline(obj, flags, argi, argc, argv,
        "-multifile", "",
            "if specified, use a file-per-timestep",
            &doMultifile,
        "-numCycles %d", "10",
            "specify the number of cycles to run",
            &numCycles,
        "-dims %d %f %d %f", "25 5 35 7",
            "specify size (logical and geometric) of mesh",
            &Ni, &Xi, &Nj, &Xj,
    MACSIO_CLARGS_END_OF_ARGS);

.. doxygengroup:: MACSIO_CLARGS
