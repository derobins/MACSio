#ifndef MACSIO_MAIN_H
#define MACSIO_MAIN_H
/*
Copyright (c) 2015, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
Written by Mark C. Miller

LLNL-CODE-676051. All rights reserved.

This file is part of MACSio

Please also read the LICENSE file at the top of the source code directory or
folder hierarchy.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License (as published by the Free Software
Foundation) version 2, dated June 1991.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <json-cwx/json_object.h>

/*!
\defgroup MACSIO_MAIN MACSIO_MAIN
\brief MACSio Main Program

\tableofcontents

MACSio is a Multi-purpose, Application-Centric, Scalable I/O proxy application.

It is designed to support a number of goals with respect to parallel I/O performance benchmarking
including the ability to test and compare various I/O libraries and I/O paradigms, to predict
scalable performance of real applications and to help identify where improvements in I/O performance
can be made.

For an overview of MACSio's design goals and outline of its design, please see
<A HREF="../../macsio_design_intro_final_html/macsio_design_intro_final.htm">this design document.</A>

MACSio is capable of generating a wide variety of mesh and variable data and of amorphous metadata
typical of HPC multi-physics applications. Currently, the only supported mesh type in MACSio is
a rectilinear, multi-block type mesh in 2 or 3 dimensions. However, some of the functions to generate other
mesh types such as curvilinear, block-structured AMR, unstructured, unstructured-AMR and arbitrary
are already available. In addition, regardless of the particular type of mesh MACSio generates for
purposes of I/O performance testing, it stores and marshalls all of the resultant data in an uber
JSON-C object that is passed around witin MACSio and between MACSIO and its I/O plugins.

MACSio employs a very simple algorithm to generate and then decompose a mesh in parallel. However, the
decomposition is also general enough to create multiple mesh pieces on individual MPI ranks and for
the number of mesh pieces vary to between MPI ranks. At present, there is no support to explicitly specify
a particular arrangement of mesh pieces and MPI ranks. However, such enhancement can be easily made at
a later date.

MACSio's command-line arguments are designed to give the user control over the nominal I/O request sizes
emitted from MPI ranks for mesh bulk data and for amorphous metadata. The user specifies a size, in bytes,
for mesh pieces. MACSio then computes a mesh part size, in nodes, necessary to hit this target byte count for
double precision data. MACSio will determine an N dimensional logical size of a mesh piece that is a close
to equal dimensional as possible. In addition, the user specifies an average number of mesh pieces that will be
assigned to each MPI rank. This does not have to be a whole number. When it is a whole number, each MPI rank
has the same number of mesh pieces. When it is not, some processors have one more mesh piece than others.
This is common of HPC multi-physics applications. Together, the total processor count and average number of
mesh pieces per processor gives a total number of mesh pieces that comprise the entire mesh. MACSio then
finds an N dimensional arrangement (N=[1,2,3]) of the pieces that is as close to equal dimension as possible.
If mesh piece size or total count of pieces wind up being prime numbers, MACSio will only be able to factor
these into long, narrow shapes where 2 (or 3) of the dimensions are of size 1. That will make examination of
the resulting data using visualization tools like VisIt a little less convenient but is otherwise harmless
from the perspective of driving and assessing I/O performance.

Once the global whole mesh shape is determined as a count of total pieces and as counts of pieces in each
of the logical dimensions, MACSio uses a very simple algorithm to assign mesh pieces to MPI ranks.
The global list of mesh pieces is numbered starting from 0. First, the number
of pieces to assign to rank 0 is chosen. When the average piece count is non-integral, it is a value
between K and K+1. So, MACSio randomly chooses either K or K+1 pieces but being carful to weight the
randomness so that once all pieces are assigned to all ranks, the average piece count per rank target
is achieved. MACSio then assigns the next K or K+1 numbered pieces to the next MPI rank. It continues
assigning pieces to MPI ranks, in piece number order, until all MPI ranks have been assigned pieces.
The algorithm runs indentically on all ranks. When the algorithm reaches the part assignment for the
rank on which its executing, it then generates the K or K+1 mesh pieces for that rank. Although the
algorithm is essentially a sequential algorithm with asymptotic behavior O(\#total pieces), it is primarily
a simple book-keeping loop which completes in a fraction of a second even for more than one million
pieces.

Each piece of the mesh is a simple rectangular region of space. The spatial bounds of that region are
easily determined. Any variables to be placed on the mesh can be easily handled as long as the variable's
spatial variation can be described in the global goemetric space.

\section sec_building Building MACSio

The first step for building MACSio is to download and install json-cwx (json-c with extensions) from https://github.com/LLNL/json-cwx.

Once json-cwx has been successfully installed, CMake is used to build MACSio and any of the desired plugins (builds with silo by default)
\code
   % mkdir build
   % cd build
   % cmake -DCMAKE_INSTALL_PREFIX=[desired-install-location] \
         -DWITH_JSON-CWX_PREFIX=[path to json-cwx] \
         -DWITH_SILO_PREFIX=[path to silo] ..
   % make
   % make install
\endcode    
 NOTE: Some options for the cmake line:
    - <tt>Build docs</tt>:             -DBUILD_DOCS=ON   
    - <tt>Build HDF5 Plugin</tt>:      -DENABLE_HDF5_PLUGIN=ON -DWITH_HDF5_PREFIX=[path to hdf5]
    - <tt>Build TyphonIO Plugin</tt>:  -DENABLE_TYPHONIO_PLUGIN=ON -DWITH_TYPHONIO_PREFIX=[path to typhonio]
    - <tt>Build PDB Plugin</tt>:       -DENABLE_PBD_PLUGIN=ON
    - <tt>Build Exodus Plugin</tt>:    -DENABLE_EXODUS_PLUGIN=ON -DWITH_EXODUS_PREFIX=[path to exodus]

Although MACSio is C Language, at a minimum it must be linked using a C++ linker due to
its use of non-constant expressions in static initializers to affect the static plugin
behavior. However, its conceivable that some C++'isms have crept into the code causing
warnings or outright errors with some C compilers.

In addition, MACSio sources currently include a large number of \c \#warning statements
to help remind developers (namely me) of minor issues to be fixed. When compiling, these
produce a lot of sprurios output in stderr but are otherwise harmless.

\subsection sec_building_plugins MACSio Plugins

Each plugin is defined by a file such as \c macsio_foo.c
for a plugin named foo. \c macsio_foo.c implements the \c MACSIO_IFACE interface for the
foo plugin.

MACSio does not use \c dlopen() to manage plugins. Instead, MACSio uses a \em static approach
to managing plugins. The set of plugins available in a \c macsio executable is determined at
the time the executable is linked simply by listing all the plugin object files to be linked
into the executable (along with their associated TPL(s)). MACSio exploits a feature in C++
which permits initialization of static variables via non-constant expressions. All symbols in
a plugin are defined with \c static scope. Every plugin defines an <tt>int registration(void)</tt>
function and initializes a static dummy integer to the result of \c registration() like so...

\code
static int register_this_interface(void)
{
  MACSIO_IFACE_Handle_t iface;

  strcpy(iface.name, iface_name);
  strcpy(iface.ext, iface_ext);

  if (!MACSIO_IFACE_Register(&iface))
      MACSIO_LOG_MSG(Die, ("Failed to register interface \"%s\"", iface.name));
}
static int dummy = register_this_interface();
\endcode

At the time the executable loads, the \c register_this_interface() method is called. Note that
this is called long before even \c main() is called. The
call to \c MACSIO_IFACE_Register() from within \c register_this_interface() winds up
adding the plugin to MACSio's global list of plugins. This happens for each plugin. The order
in which they are added to MACSio doesn't matter because plugins are identified by their
(unique) names. If MACSio encounters a case where two different plugins have the same
name, then it will abort and inform the user of the problem. The remedy is to
adjust the name of one of the two plugins. MACSio is able to call \c static methods
defined within the plugin via function callback pointers registered with the interface.

@{
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_MPI
extern MPI_Comm MACSIO_MAIN_Comm;
#else
extern int MACSIO_MAIN_Comm;
#endif
extern int MACSIO_MAIN_Size;
extern int MACSIO_MAIN_Rank;

#ifdef __cplusplus
}
#endif

/*!@}*/

#endif /* MACSIO_MAIN_H */
