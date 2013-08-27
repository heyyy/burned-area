#ifndef OUTPUT_H
#define OUTPUT_H

#include "PredictBurnedArea.h"

/* Prototypes */

bool CreateOutput(char *file_name, char *input_header, char *output_header);
Output_t *OpenOutput(char *file_name, int nband, int nband_qa,
   char sds_names[NBAND_REFL_MAX_OUT][MAX_STR_LEN],
   char qa_sds_names[NUM_QA_BAND][MAX_STR_LEN], Img_coord_int_t *size);

bool PutOutputLine(Output_t *ds_output, int iband, int iline);
bool PutOutputQALine(Output_t *ds_output, int iband, int iline);
bool CloseOutput(Output_t *ds_output);
bool FreeOutput(Output_t *ds_output);

#endif
