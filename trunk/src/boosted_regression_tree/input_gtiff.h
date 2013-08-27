/*
 * input_gtiff.h
 *
 *  Created on: Aug. 16, 2013
 *      Author: gschmidt
 */

#ifndef INPUT_GTIF_H
#define INPUT_GTIF_H

#include <stdlib.h>
#include <stdio.h>
#include "PredictBurnedArea.h"

/* Prototypes */
Input_Gtif_t *OpenGtifInput (char *file_name);
bool CloseGtifInput (Input_Gtif_t *ds_input);

#endif
