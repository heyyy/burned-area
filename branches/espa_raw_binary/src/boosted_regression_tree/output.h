/*****************************************************************************
FILE: output.h
  
PURPOSE: Contains output-related constants, prototypes, and defines.

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

#ifndef OUTPUT_H
#define OUTPUT_H

#include "PredictBurnedArea.h"

/* Prototypes */
bool CreateOutputHeader (char *base_name, char *output_file);
Output_t *OpenOutput(char *file_name, Img_coord_int_t *size);
bool CloseOutput(Output_t *ds_output);
bool FreeOutput(Output_t *ds_output);

#endif
