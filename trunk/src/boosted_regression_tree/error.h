/*****************************************************************************
FILE: error.h
  
PURPOSE: Contains error handling related defines and prototypes

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
*****************************************************************************/

#ifndef ERROR_H
#define ERROR_H

#define EXIT_ERROR(message, module) \
          Error((message), (module), (__FILE__), (long)(__LINE__), true)

#define RETURN_ERROR(message, module, status) \
          {Error((message), (module), (__FILE__), (long)(__LINE__), false); \
       return (status);}

void Error(const char *message, const char *module, 
           const char *source, long line, bool done);

#endif
