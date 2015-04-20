/*****************************************************************************
FILE: input_rb.h
  
PURPOSE: Contains input-related defines and prototypes for the raw binary files
of seasonal summaries and annual maximums.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
9/15/2012   Jodi Riegle      Original development (based largely on routines
                             from the LEDAPS lndsr application)
9/3/2013    Gail Schmidt     Modified to work in the ESPA environment
4/9/2014    Gail Schmidt     Updated to work with raw binary

NOTES:
*****************************************************************************/

#ifndef INPUT_RB_H
#define INPUT_RB_H

#include <stdlib.h>
#include <stdio.h>
#include "espa_common.h"
#include "input.h"
#include "PredictBurnedArea.h"

/* Prototypes */
Input_Rb_t *OpenRbInput (char *file_name);
bool CloseRbInput (Input_Rb_t *ds_input);
bool FreeRbInput (Input_Rb_t *ds_input);

#endif
