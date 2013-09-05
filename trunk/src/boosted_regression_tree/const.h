/*****************************************************************************
FILE: const.h
  
PURPOSE: Contains PI and degree/radians-related constants

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

#ifndef CONST_H
#define CONST_H

#include <math.h>

#ifndef PI
#ifndef M_PI
#define PI (3.141592653589793238)
#else
#define PI (M_PI)
#endif
#endif

#define TWO_PI (2.0 * PI)
#define HALF_PI (PI / 2.0)

#define DEG (180.0 / PI)
#define RAD (PI / 180.0)

#endif
