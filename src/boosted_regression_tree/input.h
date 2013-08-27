/*
 * input.h
 *
 *  Created on: Nov 15, 2012
 *      Author: jlriegle
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdlib.h>
#include <stdio.h>
#include "PredictBurnedArea.h"

/* Prototypes */
Input_t *OpenInput(char *file_name);
bool GetInputLine(Input_t *ds_input, int iband, int iline);
bool GetInputQALine(Input_t *ds_input, int iband, int iline);
bool CloseInput(Input_t *ds_input);
bool FreeInput(Input_t *ds_input);
bool GetInputMeta(Input_t *ds_input);

#endif

