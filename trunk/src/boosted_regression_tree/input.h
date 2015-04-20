/*****************************************************************************
FILE: input.h
  
PURPOSE: Contains input related defines and prototypes

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

#ifndef INPUT_H
#define INPUT_H

#include <stdlib.h>
#include <stdio.h>
#include "espa_common.h"
#include "PredictBurnedArea.h"

/* Prototypes */
Input_t *OpenInput (char *base_name, char *mask_name, int fill_val);
bool CloseInput (Input_t *ds_input);
bool FreeInput (Input_t *ds_input);
bool ReadHdr (string filename, int* lines, int* samples);

#endif
