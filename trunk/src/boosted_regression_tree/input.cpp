/*****************************************************************************
FILE: input.cpp
  
PURPOSE: Contains functions for opening, reading, closing, and processing
input HDF products.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
11/15/2012    Jodi Riegle      Original development
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
9/10/2011     Gail Schmidt     Modified to use the cfmask QA values which are
                               more accurate than the SR QA values

NOTES:
*****************************************************************************/

#include "input.h"

using namespace std;

const char *SDS_PREFIX = "band";
const char *INPUT_PROVIDER = "DataProvider";
const char *INPUT_SAT = "Satellite";
const char *INPUT_INST = "Instrument";
const char *INPUT_ACQ_DATE = "AcquisitionDate";
const char *INPUT_PROD_DATE = "Level1ProductionDate";
const char *INPUT_SUN_ZEN = "SolarZenith";
const char *INPUT_SUN_AZ = "SolarAzimuth";
const char *INPUT_WRS_SYS = "WRS_System";
const char *INPUT_WRS_PATH = "WRS_Path";
const char *INPUT_WRS_ROW = "WRS_Row";
const char *INPUT_WBC = "WestBoundingCoordinate";
const char *INPUT_EBC = "EastBoundingCoordinate";
const char *INPUT_NBC = "NorthBoundingCoordinate";
const char *INPUT_SBC = "SouthBoundingCoordinate";
const char *INPUT_UL_LAT_LONG = "UpperLeftCornerLatLong";
const char *INPUT_LR_LAT_LONG = "LowerRightCornerLatLong";
const char *INPUT_FILL_VALUE = "_FillValue";

#define N_LSAT_WRS1_ROWS  (251)
#define N_LSAT_WRS1_PATHS (233)
#define N_LSAT_WRS2_ROWS  (248)
#define N_LSAT_WRS2_PATHS (233)

/* Band names for the QA bands that will be read from the surface reflectance
   product.  Needs to match the QA_Band_t enumerated type in
   PredictBurnedArea.h */
/* CFMASK: const char *qa_band_names[NUM_QA_BAND] = {"fmask_band"}; */
const char *qa_band_names[NUM_QA_BAND] = {"fill_QA", "DDV_QA", "cloud_QA",
  "cloud_shadow_QA", "snow_QA", "land_water_QA", "adjacent_cloud_QA"};


/******************************************************************************
MODULE: OpenInput

PURPOSE: Sets up the 'input' data structure, opens the input file for read
access, allocates memory, and stores some of the metadata from the input file.
 
RETURN VALUE:
Type = Input_t*
Value          Description
-----          -----------
NULL           Error opening the HDF file and populating the data structure
non-NULL       Successful processing of the input file

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
9/10/2013     Gail Schmidt     Modified to use the cfmask QA values which are
                               more accurate than the SR QA values
12/8/2013     Gail Schmidt     Restored the LEDAPS SR QA values

NOTES:
*****************************************************************************/
Input_t *OpenInput
(
  char *file_name      /* I: input filename of file to be opened */
)
{
  Myhdf_attr_t attr;                  /* local attributes */
  const char *error_string = NULL;    /* error message */
  char sds_name[40];                  /* name of SDS to be read */
  int ir;                             /* looping variable */
  Myhdf_dim_t *dim[2];                /* dimension structure */
  int ib;                             /* looping variable for bands */
  double dval[NBAND_REFL_MAX];        /* double value array */
  int16 *buf = NULL;                  /* buffer for the reflectance values */
  uint8 *qa_buf = NULL;               /* buffer for the QA values */

  /* Create the Input data structure */
  Input_t* ds_input = new Input_t();
  if (ds_input == NULL)
    RETURN_ERROR("allocating Input data structure", "OpenInput", NULL);

  /* Populate the data structure */
  ds_input->file_name = DupString(file_name);
  if (ds_input->file_name == NULL) {
      delete ds_input;
    RETURN_ERROR("duplicating file name", "OpenInput", NULL);
  }

  /* Open file for SD access */
  ds_input->sds_file_id = SDstart((char *)file_name, DFACC_RDONLY);
  if (ds_input->sds_file_id == HDF_ERROR) {
      delete ds_input;
    RETURN_ERROR("opening input file", "OpenInput", NULL);
  }
  ds_input->open = true;

  /* Get the input metadata */
  if (!GetInputMeta(ds_input)) {
      delete ds_input->file_name;
      delete ds_input;
    RETURN_ERROR("getting input metadata", "OpenInput", NULL);
  }

  /* Get SDS information and start SDS access */
  for (ib = 0; ib < ds_input->nband; ib++) {  /* image data */
    ds_input->sds[ib].name = NULL;
    ds_input->sds[ib].dim[0].name = NULL;
    ds_input->sds[ib].dim[1].name = NULL;
    ds_input->buf[ib] = NULL;
  }

  for (ib = 0; ib < ds_input->nqa_band; ib++) {  /* QA data */
    ds_input->qa_sds[ib].name = NULL;
    ds_input->qa_sds[ib].dim[0].name = NULL;
    ds_input->qa_sds[ib].dim[1].name = NULL;
    ds_input->qa_buf[ib] = NULL;
  }

  /* thermal data */
  ds_input->therm_sds.name = NULL;
  ds_input->therm_sds.dim[0].name = NULL;
  ds_input->therm_sds.dim[1].name = NULL;
  ds_input->therm_buf = NULL;

  /* Loop through the image bands and obtain the SDS information */
  for (ib = 0; ib < ds_input->nband; ib++) {
    if (sprintf(sds_name, "%s%d", SDS_PREFIX, ds_input->meta.band[ib]) < 0) {
      error_string = "creating SDS name";
      break;
    }

    ds_input->sds[ib].name = DupString(sds_name);
    if (ds_input->sds[ib].name == NULL) {
      error_string = "setting SDS name";
      break;
    }

    if (!GetSDSInfo(ds_input->sds_file_id, &ds_input->sds[ib])) {
      error_string = "getting sds info";
      break;
    }

    /* Check rank */
    if (ds_input->sds[ib].rank != 2) {
      error_string = "invalid rank";
      break;
    }

    /* Check SDS type */
    if (ds_input->sds[ib].type != DFNT_INT16) {
      error_string = "invalid number type";
      break;
    }

    /* Get dimensions */
    for (ir = 0; ir < ds_input->sds[ib].rank; ir++) {
      dim[ir] = &ds_input->sds[ib].dim[ir];
      if (!GetSDSDimInfo(ds_input->sds[ib].id, dim[ir], ir)) {
        error_string = "getting dimensions";
        break;
      }
    }

    if (error_string != NULL) break;

    /* Save and check line and sample dimensions */
    if (ib == 0) {
      ds_input->size.l = dim[0]->nval;
      ds_input->size.s = dim[1]->nval;
    } else {
      if (ds_input->size.l != dim[0]->nval) {
        error_string = "all line dimensions do not match";
        break;
      }
      if (ds_input->size.s != dim[1]->nval) {
        error_string = "all sample dimensions do not match";
        break;
      }
    }

    /* If ds_input is the first image band, read the fill value */
    attr.type = DFNT_FLOAT32;
    attr.nval = 1;
    attr.name = (char *) INPUT_FILL_VALUE;
    if (!GetAttrDouble(ds_input->sds[ib].id, &attr, dval))
      RETURN_ERROR("reading band SDS attribute (fill value)", "OpenInput",
        NULL);
    if (attr.nval != 1)
      RETURN_ERROR("invalid number of values (fill value)", "OpenInput", NULL);
    ds_input->meta.fill = dval[0];
  }  /* for ib */

  /* Loop through the QA bands and obtain the SDS information */
  for (ib = 0; ib < ds_input->nqa_band; ib++) {
    strcpy (sds_name, qa_band_names[ib]);
    ds_input->qa_sds[ib].name = DupString(sds_name);

    if (ds_input->qa_sds[ib].name == NULL) {
      error_string = "setting QA SDS name";
      break;
    }

    if (!GetSDSInfo(ds_input->sds_file_id, &ds_input->qa_sds[ib])) {
      error_string = "getting QA sds info";
      break;
    }

    /* Check rank */
    if (ds_input->qa_sds[ib].rank != 2) {
      error_string = "invalid QA rank";
      break;
    }

    /* Check SDS type */
    if (ds_input->qa_sds[ib].type != DFNT_UINT8) {
      error_string = "invalid QA number type";
      break;
    }

    /* Get dimensions */
    for (ir = 0; ir < ds_input->qa_sds[ib].rank; ir++) {
      dim[ir] = &ds_input->qa_sds[ib].dim[ir];
      if (!GetSDSDimInfo(ds_input->qa_sds[ib].id, dim[ir], ir)) {
        error_string = "getting QA dimensions";
        break;
      }
    }

    if (error_string != NULL) break;

    /* Save and check line and sample dimensions */
    if (ib == 0) {
      ds_input->size.l = dim[0]->nval;
      ds_input->size.s = dim[1]->nval;
    } else {
      if (ds_input->size.l != dim[0]->nval) {
        error_string = "all line dimensions do not match";
        break;
      }
      if (ds_input->size.s != dim[1]->nval) {
        error_string = "all sample dimensions do not match";
        break;
      }
    }
  }  /* for ib */

  /* Process any errors */
  if (error_string != NULL)
    RETURN_ERROR(error_string, "OpenInput", NULL);

  /* For the single thermal band, obtain the SDS information */
  strcpy (sds_name, "band6");
  ds_input->therm_sds.name = DupString(sds_name);

  if (ds_input->therm_sds.name == NULL)
    error_string = "setting thermal SDS name";

  if (!GetSDSInfo(ds_input->sds_file_id, &ds_input->therm_sds))
    error_string = "getting thermal sds info";

  /* Check rank */
  if (ds_input->therm_sds.rank != 2)
    error_string = "invalid thermal rank";

  /* Check SDS type */
  if (ds_input->therm_sds.type != DFNT_INT16)
    error_string = "invalid thermal number type";

  /* Get dimensions */
  for (ir = 0; ir < ds_input->therm_sds.rank; ir++) {
    dim[ir] = &ds_input->therm_sds.dim[ir];
    if (!GetSDSDimInfo(ds_input->therm_sds.id, dim[ir], ir))
      error_string = "getting thermal dimensions";
  }

  /* Process any errors */
  if (error_string != NULL)
    RETURN_ERROR(error_string, "OpenInput", NULL);

  /* Allocate input buffers.  Thermal band only has one band.  Image and QA
     buffers have multiple bands. */
  buf = (int16 *)calloc((size_t)(ds_input->size.s * ds_input->nband),
    sizeof(int16));
  if (buf == NULL) {
    RETURN_ERROR("allocating input buffer", "OpenInput", NULL);
  }
  else {
    ds_input->buf[0] = buf;
    for (ib = 1; ib < ds_input->nband; ib++)
      ds_input->buf[ib] = ds_input->buf[ib - 1] + ds_input->size.s;
  }

  qa_buf = (uint8 *)calloc((size_t)(ds_input->size.s * ds_input->nqa_band),
    sizeof(uint8));
  if (qa_buf == NULL) {
    RETURN_ERROR("allocating input QA buffer", "OpenInput", NULL);
  }
  else {
    ds_input->qa_buf[0] = qa_buf;
    for (ib = 1; ib < ds_input->nqa_band; ib++)
      ds_input->qa_buf[ib] = ds_input->qa_buf[ib - 1] + ds_input->size.s;
  }

  ds_input->therm_buf = (int16 *)calloc((size_t)(ds_input->size.s),
    sizeof(int16));
  if (ds_input->therm_buf == NULL)
    RETURN_ERROR("allocating input thermal buffer", "OpenInput", NULL);

  return ds_input;
}


/******************************************************************************
MODULE: CloseInput

PURPOSE: Ends SDS access and closed the input file.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error closing the input file
true           Successful closing of the input file

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool CloseInput
(
  Input_t *ds_input   /* I: input data structure for file to be closed */
)
{
  int ib;

  if (!ds_input->open)
    RETURN_ERROR("file not open", "CloseInput", false);

  /* Close image SDSs */
  for (ib = 0; ib < ds_input->nband; ib++) {
    if (SDendaccess(ds_input->sds[ib].id) == HDF_ERROR)
      RETURN_ERROR("ending sds access", "CloseInput", false);
  }

  /* Close QA SDSs */
  for (ib = 0; ib < ds_input->nqa_band; ib++) {
    if (SDendaccess(ds_input->qa_sds[ib].id) == HDF_ERROR)
      RETURN_ERROR("ending qa sds access", "CloseInput", false);
  }

  /* Close thermal SDS */
  if (SDendaccess(ds_input->therm_sds.id) == HDF_ERROR)
    RETURN_ERROR("ending thermal sds access", "CloseInput", false);

  /* Close the HDF file itself */
  SDend(ds_input->sds_file_id);
  ds_input->open = false;

  return true;
}


/******************************************************************************
MODULE: FreeInput

PURPOSE: Frees the 'input' data structure and memory.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error freeing the input structure and memory
true           Successful freeing of the input structure and memory

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool FreeInput
(
  Input_t *ds_input   /* I: input data structure */
)
{
  int ib, ir;

  if (ds_input->open)
    RETURN_ERROR("file still open", "FreeInput", false);

  if (ds_input != NULL) {
    /* Free image band SDSs */
    for (ib = 0; ib < ds_input->nband; ib++) {
      for (ir = 0; ir < ds_input->sds[ib].rank; ir++) {
        if (ds_input->sds[ib].dim[ir].name != NULL)
          delete ds_input->sds[ib].dim[ir].name;
      }
      if (ds_input->sds[ib].name != NULL)
        delete ds_input->sds[ib].name;
    }

    /* Free QA band SDSs */
    for (ib = 0; ib < ds_input->nqa_band; ib++) {
      for (ir = 0; ir < ds_input->qa_sds[ib].rank; ir++) {
        if (ds_input->qa_sds[ib].dim[ir].name != NULL)
          delete ds_input->qa_sds[ib].dim[ir].name;
      }
      if (ds_input->qa_sds[ib].name != NULL)
        delete ds_input->qa_sds[ib].name;
    }

    /* Free thermal band SDS */
    for (ir = 0; ir < ds_input->therm_sds.rank; ir++) {
      if (ds_input->therm_sds.dim[ir].name != NULL)
        delete ds_input->therm_sds.dim[ir].name;
    }
    if (ds_input->therm_sds.name != NULL)
      delete ds_input->therm_sds.name;

    /* Free the data buffers */
    if (ds_input->buf[0] != NULL)
      free(ds_input->buf[0]);
    if (ds_input->qa_buf[0] != NULL)
      free(ds_input->qa_buf[0]);
    if (ds_input->therm_buf != NULL)
      free(ds_input->therm_buf);

    if (ds_input->file_name != NULL)
      free(ds_input->file_name);
    free(ds_input);
  } /* end if */

  return true;
}


/******************************************************************************
MODULE: GetInputData (class PredictBurnedArea)

PURPOSE: Reads the surface reflectance data for the current band and line
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading the data
true           Successful reading of the data

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
  1. Band data read is stored in class variable predMat (cv::Mat) as floating
     point values
*****************************************************************************/
bool PredictBurnedArea::GetInputData
(
  Input_t *ds_input,    /* I: input data structure */
  int iband,            /* I: input band (0-based) */
  int iline             /* I: input line (0-based) */
)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];
  int16 *data = new int16[ds_input->size.s];

  /* Check the parameters */
  if (ds_input == (Input_t *)NULL)
    RETURN_ERROR("invalid input structure", "GetIntputLine", false);
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputLine", false);
  if (iband < 0 || iband >= ds_input->nband)
    RETURN_ERROR("invalid band number", "GetInputLine", false);

  /* Read the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_input->size.s;

  if (SDreaddata (ds_input->sds[iband].id, start, NULL, nval, (VOIDP)data) ==
      HDF_ERROR)
    RETURN_ERROR("reading input", "GetInputLine", false)

  /* Grabbing bands 1-5 & 7 and putting value into predMat */
  for (int i=0; i<ds_input->size.s; i++) {
      predMat.at<float>(i,iband) = data[i];
  }

  return true;
}


/******************************************************************************
MODULE: calcBands (class PredictBurnedArea)

PURPOSE: Computes the NDVI, NDMI, NBR, and NBR2 for the input data and places
it in predMat
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error processing the data
true           Successful processing of the data

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
  1. The spectral index is multiplied by 1000.0 to match what is used in the
     training dataset.  The training data has the spectral indices represented
     as an integer, where the actual index has been multiplied by 1000.
  2. It is assumed the data for the current line has already been loaded into
     predMat via GetInputData.
*****************************************************************************/
bool PredictBurnedArea::calcBands
(
    Input_t *ds_input   /* I: input data structure for this data */
)
{
    for (int i = 0; i < ds_input->size.s; i++) {
        /* NDVI - using bands 4 and 3 */
        if ((fillMat.at<unsigned char>(i) != 0) ||
            (predMat.at<float>(i,PREDMAT_B4) + predMat.at<float>(i,PREDMAT_B3)
            == 0)) { //avoid division by 0 and fill data
            predMat.at<float>(i,PREDMAT_NDVI) = 0;
        } else {
            predMat.at<float>(i,PREDMAT_NDVI) =
                ((predMat.at<float>(i,PREDMAT_B4) -
                  predMat.at<float>(i,PREDMAT_B3)) /
                 (predMat.at<float>(i,PREDMAT_B4) +
                  predMat.at<float>(i,PREDMAT_B3))) * 1000;
        }

        /* NDMI - using bands 4 and 5 */
        if ((fillMat.at<unsigned char>(i) != 0) ||
            (predMat.at<float>(i,PREDMAT_B4) + predMat.at<float>(i,PREDMAT_B5)
            == 0)) { //avoid division by 0 and fill data
            predMat.at<float>(i,PREDMAT_NDMI) = 0; //avoid division by 0
        } else {
            predMat.at<float>(i,PREDMAT_NDMI) =
                ((predMat.at<float>(i,PREDMAT_B4) -
                  predMat.at<float>(i,PREDMAT_B5)) /
                 (predMat.at<float>(i,PREDMAT_B4) +
                  predMat.at<float>(i,PREDMAT_B5))) * 1000;
        }

        /* NBR - using bands 4 and 7 */
        if ((fillMat.at<unsigned char>(i) != 0) ||
            (predMat.at<float>(i,PREDMAT_B4) + predMat.at<float>(i,PREDMAT_B7)
            == 0)) { //avoid division by 0 and fill data
            predMat.at<float>(i,PREDMAT_NBR) = 0; //avoid division by 0
        } else {
            predMat.at<float>(i,PREDMAT_NBR) =
                ((predMat.at<float>(i,PREDMAT_B4) -
                  predMat.at<float>(i,PREDMAT_B7)) /
                 (predMat.at<float>(i,PREDMAT_B4) +
                  predMat.at<float>(i,PREDMAT_B7))) * 1000;
        }

        /* NBR2 - using bands 5 and 7 */
        if ((fillMat.at<unsigned char>(i) != 0) ||
            (predMat.at<float>(i,PREDMAT_B5) + predMat.at<float>(i,PREDMAT_B7)
            == 0)) { //avoid division by 0 and fill data
            predMat.at<float>(i,PREDMAT_NBR2) = 0; //avoid division by 0
        } else {
            predMat.at<float>(i,PREDMAT_NBR2) =
                ((predMat.at<float>(i,PREDMAT_B5) -
                  predMat.at<float>(i,PREDMAT_B7)) /
                 (predMat.at<float>(i,PREDMAT_B5) +
                  predMat.at<float>(i,PREDMAT_B7))) * 1000;
        }
    }

    return true;
}


/******************************************************************************
MODULE: GetInputQALine (class PredictBurnedArea)

PURPOSE: Reads the QA data for the current QA band and line
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading the data
true           Successful reading of the data

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
9/10/2013     Gail Schmidt     Modified to use the cfmask QA mask for cloud,
                               snow, etc.
12/8/2013     Gail Schmidt     Backed out use of cfmask and returned to using
                               the LEDAPS SR mask for QA

NOTES:
  1. QA data read is stored in class variables fillMat, cloudMat, cloudShadMat,
     and landWaterMat (all of type cv::Mat).
*****************************************************************************/
bool PredictBurnedArea::GetInputQALine
(
  Input_t *ds_input,    /* I: input data structure */
  int iband,            /* I: input band (0-based) */
  int iline             /* I: input line (0-based) */
)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];

  uint8 *data;
  data = new uint8[ds_input->size.s];

  /* Check the parameters */
  if (ds_input == (Input_t *)NULL)
    RETURN_ERROR("invalid input structure", "GetIntputQALine", false);
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputQALine", false);
  if (iband < 0 || iband >= NUM_QA_BAND)
    RETURN_ERROR("invalid band number", "GetInputQALine", false);
  if (iline < 0 || iline >= ds_input->size.l)
    RETURN_ERROR("invalid line number", "GetInputQALine", false);

  /* Read the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_input->size.s;

  if (SDreaddata (ds_input->qa_sds[iband].id, start, NULL, nval, (VOIDP)data)
    == HDF_ERROR)
    RETURN_ERROR("reading input", "GetInputQALine", false);

  if (strcmp(ds_input->qa_sds[iband].name, "fill_QA") ==0) {
      for (int i=0; i<ds_input->size.s; i++) {
           fillMat.at<unsigned char>(i,0) = data[i];
      }
  }

  else if (strcmp(ds_input->qa_sds[iband].name, "cloud_QA") ==0) {
      for (int i=0; i<ds_input->size.s; i++) {
           cloudMat.at<unsigned char>(i,0) = data[i];
      }
  }

  else if (strcmp(ds_input->qa_sds[iband].name, "cloud_shadow_QA") ==0) {
      for (int i=0; i<ds_input->size.s; i++) {
           cloudShadMat.at<unsigned char>(i,0) = data[i];
      }
  }

  else if (strcmp(ds_input->qa_sds[iband].name, "land_water_QA") ==0) {
      for (int i=0; i<ds_input->size.s; i++) {
           landWaterMat.at<unsigned char>(i,0) = data[i];
      }
  }

  else {
      RETURN_ERROR("invalid QA band", "GetInputQALine", false);
  }

  return true;
}


/******************************************************************************
MODULE: GetInputMeta

PURPOSE: Reads the metadata for the input HDF file
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading the metadata
true           Successful reading of the metadata

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
  1. Band data read is stored in class variable predMat (cv::Mat) as floating
     point values
*****************************************************************************/
bool GetInputMeta
(
  Input_t *ds_input     /* I: input data structure */
)
{
  Myhdf_attr_t attr;
  double dval[NBAND_REFL_MAX];
  char date[MAX_DATE_LEN + 1];
  int ib;
  Input_meta_t *meta = NULL;
  char *error_string = NULL;
  int refl_bands[NUM_REFL_BAND] = {1,2,3,4,5,7};  /* identify the reflectance
                                  bands expected to be in the product */

  /* Check the parameters */
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputMeta", false);
  meta = &ds_input->meta;

  /* Read the metadata */
  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_PROVIDER;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.provider))
    RETURN_ERROR("reading attribute (data provider)", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_SAT;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.sat))
    RETURN_ERROR("reading attribute (instrument)", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_INST;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.inst))
    RETURN_ERROR("reading attribute (instrument)", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_DATE_LEN;
  attr.name = (char *) INPUT_ACQ_DATE;
  if (!GetAttrString(ds_input->sds_file_id, &attr, date))
    RETURN_ERROR("reading attribute (acquisition date)", "GetInputMeta", false);
  if (!DateInit(&meta->acq_date, date, DATE_FORMAT_DATEA_TIME))
    RETURN_ERROR("converting acquisition date", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_DATE_LEN;
  attr.name = (char *) INPUT_PROD_DATE;
  if (!GetAttrString(ds_input->sds_file_id, &attr, date))
    RETURN_ERROR("reading attribute (production date)", "GetInputMeta", false);
  if (!DateInit(&meta->prod_date, date, DATE_FORMAT_DATEA_TIME))
    RETURN_ERROR("converting production date", "GetInputMeta", false);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_SUN_ZEN;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (solar zenith)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (solar zenith)",
                  "GetInputMeta", false);
  if (dval[0] < -90.0  ||  dval[0] > 90.0)
    RETURN_ERROR("solar zenith angle out of range", "GetInputMeta", false);
  meta->sun_zen = (float)(dval[0] * RAD);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_SUN_AZ;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (solar azimuth)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (solar azimuth)",
                 "GetInputMeta", false);
  if (dval[0] < -360.0  ||  dval[0] > 360.0)
    RETURN_ERROR("solar azimuth angle out of range", "GetInputMeta", false);
  meta->sun_az = (float)(dval[0] * RAD);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_WRS_SYS;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.wrs_sys))
    RETURN_ERROR("reading attribute (WRS system)", "GetInputMeta", false);

  attr.type = DFNT_INT16;
  attr.nval = 1;
  attr.name = (char *) INPUT_WRS_PATH;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (WRS path)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (WRS path)", "GetInputMeta", false);
  meta->path = (int)floor(dval[0] + 0.5);
  if (meta->path < 1)
    RETURN_ERROR("WRS path out of range", "GetInputMeta", false);

  attr.type = DFNT_INT16;
  attr.nval = 1;
  attr.name = (char *) INPUT_WRS_ROW;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (WRS row)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (WRS row)", "GetInputMeta", false);
  meta->row = (int)floor(dval[0] + 0.5);
  if (meta->row < 1)
    RETURN_ERROR("WRS path out of range", "GetInputMeta", false);

  /* identify the reflectance bands */
  ds_input->nband = NUM_REFL_BAND;
  for (ib = 0; ib < ds_input->nband; ib++)
    meta->band[ib] = refl_bands[ib];

  /* Get the upper left and lower right corners */
  meta->ul_corner.is_fill = false;
  attr.type = DFNT_FLOAT32;
  attr.nval = 2;
  attr.name = (char *) INPUT_UL_LAT_LONG;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval)) {
      printf ("WARNING: Unable to read the UL lat/long coordinates.  "
          "Processing will continue but the scene will be assumed to be "
          "a normal, north-up scene and not an ascending polar scene.  Thus "
          "the solar azimuth will be used as-is and not adjusted if the "
          "scene is flipped.\n");
      meta->ul_corner.is_fill = true;
  }
  if (attr.nval != 2) {
    RETURN_ERROR("Invalid number of values for the UL lat/long coordinate",
      "GetInputMeta", false);
  }
  meta->ul_corner.lat = dval[0];
  meta->ul_corner.lon = dval[1];

  meta->lr_corner.is_fill = false;
  attr.type = DFNT_FLOAT32;
  attr.nval = 2;
  attr.name = (char *) INPUT_LR_LAT_LONG;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval)) {
      printf ("WARNING: Unable to read the LR lat/long coordinates.  "
          "Processing will continue but the scene will be assumed to be "
          "a normal, north-up scene and not an ascending polar scene.  Thus "
          "the solar azimuth will be used as-is and not adjusted if the "
          "scene is flipped.");
      meta->lr_corner.is_fill = true;
  }
  if (attr.nval != 2) {
    RETURN_ERROR("Invalid number of values for the LR lat/long coordinate",
      "GetInputMeta", false);
  }
  meta->lr_corner.lat = dval[0];
  meta->lr_corner.lon = dval[1];

  /* Get the bounding coordinates if they are available */
  meta->bounds.is_fill = false;
  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_WBC;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval)) {
    printf ("WARNING: Unable to read the west bounding coordinate.  "
        "Processing will continue but the bounding coordinates will not "
        "be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (attr.nval != 1) {
    printf ("WARNING: Invalid number of values for west bounding "
        "coordinate.  Processing will continue but the bounding "
        "coordinates will not be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (dval[0] < -180.0 || dval[0] > 180.0)
    RETURN_ERROR("west bound coord out of range", "GetInputMeta", false);
  meta->bounds.min_lon = dval[0];

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_EBC;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval)) {
    printf ("WARNING: Unable to read the east bounding coordinate.  "
        "Processing will continue but the bounding coordinates will not "
        "be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (attr.nval != 1) {
    printf ("WARNING: Invalid number of values for east bounding "
        "coordinate.  Processing will continue but the bounding "
        "coordinates will not be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (dval[0] < -180.0 || dval[0] > 180.0)
    RETURN_ERROR("east bound coord out of range", "GetInputMeta", false);
  meta->bounds.max_lon = dval[0];

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_NBC;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval)) {
    printf ("WARNING: Unable to read the north bounding coordinate.  "
        "Processing will continue but the bounding coordinates will not "
        "be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (attr.nval != 1) {
    printf ("WARNING: Invalid number of values for north bounding "
        "coordinate.  Processing will continue but the bounding "
        "coordinates will not be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (dval[0] < -90.0 || dval[0] > 90.0)
    RETURN_ERROR("north bound coord out of range", "GetInputMeta", false);
  meta->bounds.max_lat = dval[0];

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_SBC;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval)) {
    printf ("WARNING: Unable to read the south bounding coordinate.  "
        "Processing will continue but the bounding coordinates will not "
        "be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (attr.nval != 1) {
    printf ("WARNING: Invalid number of values for south bounding "
        "coordinate.  Processing will continue but the bounding "
        "coordinates will not be written to the output product.\n");
    meta->bounds.is_fill = true;
  }
  if (dval[0] < -90.0 || dval[0] > 90.0)
    RETURN_ERROR("south bound coord out of range", "GetInputMeta", false);
  meta->bounds.min_lat = dval[0];

  /* Set the number of QA bands */
  ds_input->nqa_band = NUM_QA_BAND;

  /* Check WRS path/rows */
  error_string = (char *)NULL;

  if (!strcmp (meta->wrs_sys, "1")) {
    if (meta->path > N_LSAT_WRS1_PATHS)
      error_string = (char *) "WRS path number out of range";
    else if (meta->row > N_LSAT_WRS1_ROWS)
      error_string = (char *) "WRS row number out of range";
  } else if (!strcmp (meta->wrs_sys, "2")) {
    if (meta->path > N_LSAT_WRS2_PATHS)
      error_string = (char *) "WRS path number out of range";
    else if (meta->row > N_LSAT_WRS2_ROWS)
      error_string = (char *) "WRS row number out of range";
  } else
    error_string = (char *) "invalid WRS system";

  if (error_string != (char *)NULL)
    RETURN_ERROR(error_string, "GetInputMeta", false);

  return true;
}
