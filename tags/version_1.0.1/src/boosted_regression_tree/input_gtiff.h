/*****************************************************************************
FILE: input_gtiff.h
  
PURPOSE: Contains input-related defines and prototypes for the GeoTIFF files

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

#ifndef INPUT_GTIF_H
#define INPUT_GTIF_H

#include <stdlib.h>
#include <stdio.h>
#include "PredictBurnedArea.h"

/* Prototypes */
Input_Gtif_t *OpenGtifInput (char *file_name);
bool CloseGtifInput (Input_Gtif_t *ds_input);

#endif
