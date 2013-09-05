/*****************************************************************************
FILE: error.cpp
  
PURPOSE: Contains functions for handling errors.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
9/15/2012   Jodi Riegle      Original development (based largely on routines
                             from the LEDAPS lndsr application)
9/3/2013    Gail Schmidt     Modified to work in the ESPA environment

NOTES:
  1. See 'error.h' for information on the 'ERROR' and 'ERROR_RETURN' macros 
     that automatically populate the source code file name, line number and 
     exit flag.  
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "error.h"

/******************************************************************************
MODULE: Error

PURPOSE: Writes an error message to 'stderr' and optionally exit's the program
with a 'EXIT_FAILURE' status.
 
RETURN VALUE:
Type = None
Value          Description
-----          -----------

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
  1. If the 'errno' flag is set, the 'perror' function is first called to 
     print any i/o related errors.
  2. The error message is written to 'stdout'.
  3. The module name, source code name and line number are included in the 
     error message.
*****************************************************************************/
void Error
(
    const char *message,  /* I: error message to be written */
    const char *module,   /* I: calling module name */
    const char *source,   /* I: source code file name containing the calling
                                module */
    long line,            /* I: line number in the source code file */
    bool done             /* I: flag indicating if program is to exit with a
                                failure status;
                                'true' = exit, 'false' = return */
)
{
  if (errno) perror(" i/o error ");
  fprintf(stderr, " error [%s, %s:%ld] : %s\n", module, source, line, message);
  if (done) exit(EXIT_FAILURE);
  else return;
}
