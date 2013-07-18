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
#include "myhdf.h"
#include "const.h"
#include "date.h"
#include "error.h"
#include "mystring.h"
#include "PredictBurnedArea.h"

/* Structure for the metadata */
//typedef struct {
  //char provider[MAX_STR_LEN];  /* Data provider type */
 // char sat[MAX_STR_LEN];       /* Satellite */
//  char inst[MAX_STR_LEN];      /* Instrument */
//  Date_t acq_date;             /* Acqsition date/time (scene center) */
//  Date_t prod_date;            /* Production date (must be available for ETM) */
//  float sun_zen;               /* Solar zenith angle (radians; scene center) */
///  float sun_az;                /* Solar azimuth angle (radians; scene center) */
//  char wrs_sys[MAX_STR_LEN];   /* WRS system */
//  int path;                    /* WRS path number */
//  int row;                     /* WRS row number */
//  int fill;                    /* Fill value for image data */
//  int band[NBAND_REFL_MAX];    /* Band numbers */
//} Input_meta_t;

/* Structure for the 'input' data type */
//typedef struct {
//  char *file_name;         /* Input image file name */
//  bool open;               /* Open file flag; open = true */
///  Input_meta_t meta;       /* Input metadata */
  //int nband;               /* Number of input image bands */
 // int nqa_band;            /* Number of input QA bands */
 // Img_coord_int_t size;    /* Input file size */
 // int32 sds_file_id;       /* SDS file id */
 // Myhdf_sds_t sds[NBAND_REFL_MAX];
                           /* SDS data structures for image data */
 // int16 *buf[NBAND_REFL_MAX];
                           /* Input data buffer (one line of image data) */
 // Myhdf_sds_t therm_sds;   /* SDS data structure for thermal image data */
//  int16 *therm_buf;        /* Input data buffer (one line of thermal data) */
//  Myhdf_sds_t qa_sds[NUM_QA_BAND];
                           /* SDS data structure for QA data */
//  uint8 *qa_buf[NUM_QA_BAND];
                           /* Input data buffer (one line of QA data) */
//} Input_t;

/* Prototypes */
Input_t *OpenInput(char *file_name);
bool GetInputLine(Input_t *ds_input, int iband, int iline);
bool GetInputQALine(Input_t *ds_input, int iband, int iline);
bool GetInputThermLine(Input_t *ds_input, int iline);
bool CloseInput(Input_t *ds_input);
bool FreeInput(Input_t *ds_input);
bool GetInputMeta(Input_t *ds_input);

//bool ReadIntoArray(Input_t *ds_input);
#endif

