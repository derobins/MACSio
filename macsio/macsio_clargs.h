#ifndef _MACSIO_CLAGS_H
#define _MACSIO_CLAGS_H
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!
\defgroup MACSIO_CLARGS MACSIO_CLARGS
\brief Command-line argument parsing utilities 

@{
*/

/* error modes */
#define MACSIO_CLARGS_WARN 0
#define MACSIO_CLARGS_ERROR 1

/* route modes */
#define MACSIO_CLARGS_TOMEM     0 
#define MACSIO_CLARGS_TOJSON    1

/* default modes */
#define MACSIO_CLARGS_ASSIGN_OFF 0
#define MACSIO_CLARGS_ASSIGN_ON 1

#define MACSIO_CLARGS_HELP  -1
#define MACSIO_CLARGS_OK 0

#define MACSIO_CLARGS_GRP_SEP_STR "macsio_args_group_"
#define MACSIO_CLARGS_GRP_BEG MACSIO_CLARGS_GRP_SEP_STR "beg_"
#define MACSIO_CLARGS_GRP_END MACSIO_CLARGS_GRP_SEP_STR "end_"
#define MACSIO_CLARGS_ARG_GROUP_BEG(GRPNAME,GRPHELP) MACSIO_CLARGS_GRP_BEG #GRPNAME, MACSIO_CLARGS_NODEFAULT, #GRPHELP
#define MACSIO_CLARGS_ARG_GROUP_END(GRPNAME) MACSIO_CLARGS_GRP_END #GRPNAME, MACSIO_CLARGS_NODEFAULT, ""
#define MACSIO_CLARGS_ARG_GROUP_BEG2(GRPNAME,GRPHELP) MACSIO_CLARGS_GRP_BEG #GRPNAME, MACSIO_CLARGS_NODEFAULT, #GRPHELP, 0
#define MACSIO_CLARGS_ARG_GROUP_END2(GRPNAME)         MACSIO_CLARGS_GRP_END #GRPNAME, MACSIO_CLARGS_NODEFAULT, "", 0

#define MACSIO_CLARGS_END_OF_ARGS "macsio_end_of_args"

#define MACSIO_CLARGS_NODEFAULT (void*)0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MACSIO_CLARGS_ArgvFlags_t
{
    unsigned int error_mode    : 1; /**< 0=warn, 1=abort */
    unsigned int route_mode    : 2; /**< 0=scalar variables, 1=json_object, 2,3 unused */
    unsigned int defaults_mode : 1; /**< 0=assign, 1=do not assign */
} MACSIO_CLARGS_ArgvFlags_t;

/*!
\brief Command-line argument parsing, default values and help output

This function is used in MACSio's main program and in plugins to simplify the
addition of command-line argument groups, individual command-line arguments,
english language help strings regarding their purpose and use, default values.

After the \c argi, \c argc, \c argv trio of arguments, the remaining arguments
come in groups of either 3 (when mapping to a json_object) or 4 (when
mapping to scalar program variables).

  The first of these is an argument format specifier much like a printf
  statement. It indicates the type of each parameter to the argument as well as
  the number of parameters. Presently, it understands only %d, %f and %s types.

  The second is a string argument indicating the default value, as a string.

  The third is a help string for the argument. Note, you can make this string as
  long as the C-compiler will permit. You *need*not* embed any newline
  characters as the routine to print the help string will do that for you.
  However, you may embed newline characters if you wish to control specific line
  breaking when output.

  The fourth argument is present only when mapping to scalar program variables
  and is a pointer to the variable locations where values will be stored.

Command line arguments for which only existence on the command-line is tested
assume a caller-supplied return value of int and will be assigned `1' if the
argument exists on the command line and `0' otherwise.

Do not name any argument with a substring \c `help' as that is reserved for
obtaining help. Also, do not name any argument with the string `end_of_args'
as that is used to indicate the end of the list of arguments passed to the function.

If any argument on the command line has the substring `help', help will be
printed by processor 0 and then this function calls MPI_Finalize()
(in parallel) and exit().

This function must be called collectively in MPI_COMM_WORLD. All tasks are
guaranteed to complete with identical results.

\return An integer value of either

  - \c MACSIO_CLARGS_OK
  - \c MACSIO_CLARGS_HELP
  - \c MACSIO_CLARGS_ERROR

*/
extern int
MACSIO_CLARGS_ProcessCmdline(
   void **retobj,       /**< void pointer to returned object (for cases that need it) */
   MACSIO_CLARGS_ArgvFlags_t flags, /* flags to control behavior (see \c MACSIO_CLARGS_ArgvFlags_t) */
   int argi,            /**< First arg index at which to to start processing \c argv */
   int argc,            /**< \c argc from main */
   char **argv,         /**< \c argv from main */
   ...                  /**< Comma-separated list of 3 or 4 arguments per command-line argument;
                             1) argument name and scanf format specifier(s) for command-line argument;
                             2) default value for command-line argument;
                             3) help-string for command-line argument;
                             4) pointer to memory location to store parsed value from command-line argument. */
);


#ifdef __cplusplus
}
#endif

/*!@}*/

#endif /* _UTIL_H */
