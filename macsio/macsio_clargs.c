#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <json-c/json.h>

#include <macsio_clargs.h>
#include <macsio_log.h>

typedef struct _knownArgInfo {
   char *helpStr;		/* the help string for this command line argument */
   char *fmtStr;		/* format string for this command line argument */
   int argNameLength;		/* number of characters in this command line argument name */
   int paramCount;		/* number of parameters associated with this argument */
   char *paramTypes;		/* the string of parameter conversion specification characters */
   void **paramPtrs;		/* an array of pointers to caller-supplied scalar variables to be assigned */
   struct _knownArgInfo *next;	/* pointer to the next comand line argument */
} MACSIO_KnownArgInfo_t;

static int GetSizeFromModifierChar(char c)
{
    int n=1;
    switch (c)
    {
        case 'b': case 'B': n=(1<< 0); break;
        case 'k': case 'K': n=(1<<10); break;
        case 'm': case 'M': n=(1<<20); break;
        case 'g': case 'G': n=(1<<30); break;
        default:  n=1; break;
    }
    return n;
}

/* Handles adding one or more params for a single key. If the key doesn't
   exist, its a normal object add. If it does exist and is not already an
   array object, delete it and make it into an array object. Otherwise
   add the new param to the existing array object. */
static void
add_param_to_json_retobj(json_object *retobj, char const *key, json_object *addobj)
{
    json_object *existing_member = 0;

    if (json_object_object_get_ex(retobj, key, &existing_member))
    {
        if (json_object_is_type(existing_member, json_type_array))
        {
            json_object_array_add(existing_member, addobj);
        }
        else
        {
            json_object *addarray = json_object_new_array();
            json_object_array_add(addarray, existing_member);
            json_object_array_add(addarray, addobj);
            json_object_get(existing_member);
            json_object_object_add(retobj, key, addarray); /* replaces */
        }
    }
    else
    {
        json_object_object_add(retobj, key, addobj);
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------
 * Audience:	Private
 * Chapter:	Example and Test Utilities	
 * Purpose:	Parse and assign command-line arguments	
 *
 * Description:	This routine is designed to do parsing of simple command-line arguments and assign values associated with
 *		them to caller-supplied scalar variables. It is used in the following manner.
 *
 *		   MACSIO_ProccessCommandLine(argc, argv,
 *		      "-multifile",
 *		         "if specified, use a file-per-timestep",
 *		         &doMultifile,
 *		      "-numCycles %d",
 *		         "specify the number of cycles to run",
 *		         &numCycles,
 *		      "-dims %d %f %d %f",
 *		         "specify size (logical and geometric) of mesh",
 *		         &Ni, &Xi, &Nj, &Xj
 *		      MACSIO_CLARGS_END_OF_ARGS);
 *
 *		After the argc,argv arguments, the remaining arguments come in groups of 3. The first of the three is a
 *		argument format specifier much like a printf statement. It indicates the type of each parameter to the
 *		argument and the number of parameters. Presently, it understands only %d, %f and %s types. the second
 *		of the three is a help line for the argument. Note, you can make this string as long as the C-compiler
 *		will permit. You *need*not* embed any '\n' charcters as the print routine will do that for you.
 *
 *		Command line arguments for which only existence of the argument is tested assume a caller-supplied return
 *		value of int and will be assigned `1' if the argument exists and `0' otherwise.
 *
 *		Do not name any argument with a substring `help' as that is reserved for obtaining help. Also, do not
 *		name any argument with the string `end_of_args' as that is used to indicate the end of the list of
 *		arguments passed to the function.
 *
 *		If any argument on the command line has the substring `help', help will be printed by processor 0 and then
 *		this function calls MPI_Finalize() (in parallel) and exit().
 *
 * Parallel:    This function must be called collectively in MPI_COMM_WORLD. Parallel and serial behavior is identical except in
 *		the
 *
 * Return:	MACSIO_CLARGS_OK, MACSIO_CLARGS_ERROR or MACSIO_CLARGS_HELP
 *
 * Programmer:	Mark Miller, LLNL, Thu Dec 19, 2001 
 *---------------------------------------------------------------------------------------------------------------------------------
 */
int
MACSIO_CLARGS_ProcessCmdline(
   void **retobj,       /* returned object (for cases that need it) */
   MACSIO_CLARGS_ArgvFlags_t flags, /* flag to indicate what to do if encounter an unknown argument (FATAL|WARN) */
   int argi,            /* first arg index to start processing at */
   int argc,		/* argc from main */
   char **argv,		/* argv from main */
   ...			/* a comma-separated list of 1) argument name and format specifier,  2) help-line for argument,
			   3) caller-supplied scalar variables to set */
)
{
   char *argvStr = NULL;
   int i, rank = 0;
   int helpWasRequested = 0;
   int invalidArgTypeFound = 0;
   int firstArg;
   int terminalWidth = 80 - 10;
   MACSIO_KnownArgInfo_t *knownArgs;
   va_list ap;
   json_object *ret_json_obj = 0;

#ifdef HAVE_MPI
   {  int result;
      if ((MPI_Initialized(&result) != MPI_SUCCESS) || !result)
      { 
         MACSIO_LOG_MSGV(flags.error_mode?MACSIO_LOG_MsgErr:MACSIO_LOG_MsgWarn, ("MPI is not initialized"));
         return MACSIO_CLARGS_ERROR;
      }
   }
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

   /* quick check for a help request */
   if (rank == 0)
   {
      for (i = 0; i < argc; i++)
      {
	 if (strstr(argv[i], "help") != NULL)
	 {
	    char *s;
	    helpWasRequested = 1;
            if ((s=getenv("COLUMNS")) && isdigit(*s))
	       terminalWidth = (int)strtol(s, NULL, 0) - 10;
	    break;
	 }
      }
   }

#ifdef HAVE_MPI
#warning DO JUST ONE BCAST HERE
   MPI_Bcast(&helpWasRequested, 1, MPI_INT, 0, MPI_COMM_WORLD);
#endif

   /* everyone builds the command line parameter list */
   va_start(ap, argv);

   knownArgs = NULL;
   firstArg = 1;
   while (1)
   {
      int n, paramCount, argNameLength;
      char *fmtStr, *helpStr, *p, *paramTypes;
      void **paramPtrs;
      MACSIO_KnownArgInfo_t *newArg, *oldArg;

      /* get this arg's format specifier string */
      fmtStr = va_arg(ap, char *);

      /* check to see if we're done */
      if (!strcmp(fmtStr, MACSIO_CLARGS_END_OF_ARGS))
	 break;

      /* get this arg's help string */
      helpStr = va_arg(ap, char *);

      /* print this arg's help string from proc 0 if help was requested */
      if (helpWasRequested && rank == 0)
      {
	 static int first = 1;
	 char helpFmtStr[32];
	 FILE *outFILE = (isatty(2) ? stderr : stdout);

	 if (first)
	 {
	    first = 0;
	    fprintf(outFILE, "usage and help for %s\n", argv[0]);
	 }

	 /* this arguments format string */
	 fprintf(outFILE, "   %-s\n", fmtStr);

	 /* this arguments help-line format string */
	 sprintf(helpFmtStr, "      %%-%d.%ds", terminalWidth, terminalWidth);

	 /* this arguments help string */
	 p = helpStr;
	 n = (int)strlen(helpStr);
	 i = 0;
	 while (i < n)
	 {
	    fprintf(outFILE, helpFmtStr, p);
	    p += terminalWidth;
	    i += terminalWidth;
	    if ((i < n) && (*p != ' '))
	       fprintf(outFILE, "-\n");
	    else
	       fprintf(outFILE, "\n"); 
	 }
      }

      /* count number of parameters for this argument */
      paramCount = 0;
      n = (int)strlen(fmtStr);
      argNameLength = 0;
      for (i = 0; i < n; i++)
      {
	 if (fmtStr[i] == '%' && fmtStr[i+1] != '%')
	 {
	    paramCount++;
	    if (argNameLength == 0)
	       argNameLength = i-1;
	 }
      }

      if (paramCount)
      {
	 int k;

         /* allocate a string for the conversion characters */
         paramTypes = (char *) malloc((unsigned int)paramCount+1);

         /* allocate a string for the pointers to caller's arguments to set */ 
         paramPtrs = (void **) calloc(paramCount, sizeof(void*));

         /* ok, make a second pass through the string and setup argument pointers and conversion characters */
	 k = 0;
         for (i = 0; i < n; i++)
	 {
	    if (fmtStr[i] == '%' && fmtStr[i+1] != '%')
	    {
	       paramTypes[k] = fmtStr[i+1];
               if (flags.route_mode == MACSIO_CLARGS_TOMEM)
               {
	           switch (paramTypes[k])
	           {
	              case 'd': paramPtrs[k] = va_arg(ap, int *); break;
	              case 's': paramPtrs[k] = va_arg(ap, char **); break;
	              case 'f': paramPtrs[k] = va_arg(ap, double *); break;
	              case 'n': paramPtrs[k] = va_arg(ap, int *); break;
	              default:  invalidArgTypeFound = k+1; break;
	           }
               }
	       k++;
	    }
	 }
      }	 
      else
      {
	 /* in this case, we assume we just have a boolean (int) value */
	 argNameLength = n;
	 paramTypes = NULL;
	 paramPtrs = (void **) malloc (sizeof(void*));
         if (flags.route_mode == MACSIO_CLARGS_TOMEM)
	     paramPtrs[0] = va_arg(ap, int *);
      }

      /* ok, we're done with this parameter, so plug it into the paramInfo array */
      newArg = (MACSIO_KnownArgInfo_t *) malloc(sizeof(MACSIO_KnownArgInfo_t));
      newArg->helpStr = helpStr;
      newArg->fmtStr = fmtStr;
      newArg->argNameLength = argNameLength;
      newArg->paramCount = paramCount;
      newArg->paramTypes = paramTypes;
      newArg->paramPtrs = paramPtrs;
      newArg->next = NULL; 
      if (firstArg)
      {
	 firstArg = 0;
	 knownArgs = newArg;
	 oldArg = newArg;
      }
      else
         oldArg->next = newArg;
      oldArg = newArg;
   }
   va_end(ap);

#ifdef HAVE_MPI
   MPI_Bcast(&invalidArgTypeFound, 1, MPI_INT, 0, MPI_COMM_WORLD);
#endif
   if (invalidArgTypeFound)
   {
      if (rank == 0)
          MACSIO_LOG_MSGV(flags.error_mode?MACSIO_LOG_MsgErr:MACSIO_LOG_MsgWarn,
              ("invalid argument type encountered at position %d",invalidArgTypeFound));
#warning FIX WARN FAILURE BEHAVIOR HERE
      return MACSIO_CLARGS_ERROR;
   }

   /* exit if help was requested */
   if (helpWasRequested)
      return MACSIO_CLARGS_HELP;

   /* ok, now broadcast the whole argc, argv data */ 
#ifdef HAVE_MPI
   {
      int argvLen;
      char *p;

      /* processor zero computes size of linearized argv and builds it */
      if (rank == 0)
      {
#warning MAKE THIS A CLARG
	 if (getenv("MACSIO_CLARGS_IGNORE_UNKNOWN_ARGS"))
	    flags.error_mode = MACSIO_CLARGS_WARN;

         /* compute size of argv */
         argvLen = 0;
         for (i = 0; i < argc; i++)
	    argvLen += (strlen(argv[i])) + 1;

         /* allocate an array of chars to broadcast */
         argvStr = (char *) malloc((unsigned int)argvLen);

         /* now, fill argvStr */
         p = argvStr;
         for (i = 0; i < argc; i++)
         {
	    strcpy(p, argv[i]);
	    p += (strlen(argv[i]) + 1);
	    *(p-1) = '\0';
         }
      }

      /* now broadcast the linearized argv */
      MPI_Bcast(&flags, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast(&argc, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast(&argvLen, 1, MPI_INT, 0, MPI_COMM_WORLD);
      if (rank != 0)
         argvStr = (char *) malloc((unsigned int)argvLen);
      MPI_Bcast(argvStr, argvLen, MPI_CHAR, 0, MPI_COMM_WORLD);

      /* Issue: We're relying upon embedded nulls in argvStr */
      /* now, put it back into the argv form */
      if (rank != 0)
      {
	 argv = (char **) malloc(argc * sizeof(char*));
	 p = argvStr;
	 for (i = 0; i < argc; i++)
	 {
	    argv[i] = p;
	    p += (strlen(p) + 1);
	 }
      }
   }
#endif

   /* And now, finally, we can process the arguments and assign them */
   if (flags.route_mode == MACSIO_CLARGS_TOJSON)
       ret_json_obj = json_object_new_object();
   i = argi;
   while (i < argc)
   {
      int foundArg;
      MACSIO_KnownArgInfo_t *p;
      char argName[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

      /* search known arguments for this command line argument */
      p = knownArgs;
      foundArg = 0;
      while (p && !foundArg)
      {
         if (!strncmp(argv[i], p->fmtStr, (unsigned int)(p->argNameLength) ))
         {
            strncpy(argName, argv[i], (unsigned int)(p->argNameLength));
	    foundArg = 1;
         }
	 else
	    p = p->next;
      }
      if (foundArg)
      {
	 int j;

	 /* assign command-line arguments to caller supplied parameters */
	 if (p->paramCount)
	 {
	    for (j = 0; j < p->paramCount; j++)
	    {
               if (i == argc - 1)
                   MACSIO_LOG_MSG(Die, ("too few arguments for command-line options"));
	       switch (p->paramTypes[j])
	       {
	          case 'd':
	          {
                     int n = strlen(argv[++i])-1;
		     int *pInt = (int *) (p->paramPtrs[j]);
                     int tmpInt;
                     double tmpDbl;
                     n = GetSizeFromModifierChar(argv[i][n]);
		     tmpInt = strtol(argv[i], (char **)NULL, 10);
                     tmpDbl = tmpInt * n;
                     if ((int)tmpDbl != tmpDbl)
                     {
                         MACSIO_LOG_MSGV(flags.error_mode?MACSIO_LOG_MsgErr:MACSIO_LOG_MsgWarn,
                             ("integer overflow (%.0f) for arg \"%s\"",tmpDbl,argv[i-1]));
                     }
                     else
                     {
                         if (flags.route_mode == MACSIO_CLARGS_TOMEM)
                             *pInt = (int) tmpDbl;
                         else if (flags.route_mode == MACSIO_CLARGS_TOJSON)
                             add_param_to_json_retobj(ret_json_obj, argName, json_object_new_int((int)tmpDbl));
                     }
		     break;
	          }
	          case 's':
	          {
		     char **pChar = (char **) (p->paramPtrs[j]);
		     if (pChar && *pChar == NULL)
		     {
                         if (flags.route_mode == MACSIO_CLARGS_TOMEM)
                         {
		             *pChar = (char*) malloc(strlen(argv[++i]+1));
		             strcpy(*pChar, argv[i]);
                         }
                         else if (flags.route_mode == MACSIO_CLARGS_TOJSON)
                         {
                             add_param_to_json_retobj(ret_json_obj, argName, json_object_new_string(argv[i+1]));
                             i++;
                         }
		     }
		     else
                     {
                         if (flags.route_mode == MACSIO_CLARGS_TOMEM)
		             strcpy(*pChar, argv[++i]);
                         else if (flags.route_mode == MACSIO_CLARGS_TOJSON)
                             add_param_to_json_retobj(ret_json_obj, argName, json_object_new_string(argv[++i]));
                     }
		     break;
	          }
	          case 'f':
	          {
		     double *pDouble = (double *) (p->paramPtrs[j]);
                     if (flags.route_mode == MACSIO_CLARGS_TOMEM)
		         *pDouble = atof(argv[++i]);
                     else if (flags.route_mode == MACSIO_CLARGS_TOJSON)
                         add_param_to_json_retobj(ret_json_obj, argName, json_object_new_double(atof(argv[++i])));
		     break;
	          }
	          case 'n': /* special case to return arg index */
	          {
		     int *pInt = (int *) (p->paramPtrs[j]);
                     if (flags.route_mode == MACSIO_CLARGS_TOMEM)
		         *pInt = i++;
                     else if (flags.route_mode == MACSIO_CLARGS_TOJSON)
                         add_param_to_json_retobj(ret_json_obj, "argi", json_object_new_int(i++));
		     break;
	          }
               }
   	    }
	 }
	 else
	 {
            int *pInt = (int *) (p->paramPtrs[0]);
            if (flags.route_mode == MACSIO_CLARGS_TOMEM)
                *pInt = 1; 
            else if (flags.route_mode == MACSIO_CLARGS_TOJSON)
                add_param_to_json_retobj(ret_json_obj, argName, json_object_new_boolean((json_bool)1));
	 }
      }
      else
      {
	 char *p = strrchr(argv[0], '/');
	 FILE *outFILE = (isatty(2) ? stderr : stdout);
	 p = p ? p+1 : argv[0];
	 if (rank == 0)
             MACSIO_LOG_MSGV(flags.error_mode?MACSIO_LOG_MsgErr:MACSIO_LOG_MsgWarn,
                 ("%s: unknown argument %s. Type %s --help for help",p,argv[i],p));
         return MACSIO_CLARGS_ERROR; 
      }

      /* move to next argument */
      i++;
   }

   /* free argvStr */
   if (argvStr)
      free(argvStr);

   /* free the argv pointers we created, locally */
   if (rank != 0 && argv)
      free(argv);

   /* free the known args stuff */
   while (knownArgs)
   {
      MACSIO_KnownArgInfo_t *next = knownArgs->next;

      if (knownArgs->paramTypes)
         free(knownArgs->paramTypes);
      if (knownArgs->paramPtrs)
         free(knownArgs->paramPtrs);
      free(knownArgs);
      knownArgs = next;
   }

   if (flags.route_mode == MACSIO_CLARGS_TOJSON)
       *retobj = ret_json_obj;

   return MACSIO_CLARGS_OK;
}