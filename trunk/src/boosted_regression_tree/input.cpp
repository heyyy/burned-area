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
4/7/2014      Gail Schmidt     Commented out the use of the thermal buffer
                               setup since it currently doesn't get used
4/9/2014      Gail Schmidt     Use a local image buffer for reading the data
                               vs. allocating space and freeing for each line
                               read.
4/14/2014     Gail Schmidt     The single QA mask created by seasonal summ and
                               annual mask is a 16-bit signed int vs. the
                               previous unsigned char individual masks.

NOTES:
*****************************************************************************/

#include "input.h"
using namespace std;

/* The following libraries are external C libraries from ESPA */
extern "C" {
FILE *open_raw_binary (char *infile, char *access_type);
void close_raw_binary (FILE *fptr);
int read_raw_binary (FILE *rb_fptr, int nlines, int nsamps, int size,
    void *img_array);
int write_raw_binary (FILE *rb_fptr, int nlines, int nsamps, int size,
    void *img_array);
}

/* Band names for the reflectance bands that will be read from the surface
   reflectance product */
const char *refl_band_names[NUM_REFL_BAND] = {"sr_band1.img", "sr_band2.img",
  "sr_band3.img", "sr_band4.img", "sr_band5.img", "sr_band7.img"};


/******************************************************************************
MODULE: OpenInput

PURPOSE: Sets up the 'input' data structure, opens the input files for read
access and stores some of the metadata from the input file.
 
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
4/4/2014      Gail Schmidt     Modified to utilize the input ESPA file format
                               vs. HDF files
4/9/2014      Gail Schmidt     Use a local image buffer for reading the data
                               vs. allocating space and freeing for each line
                               read.

NOTES:
*****************************************************************************/
Input_t *OpenInput
(
  char *base_name,     /* I: input base filename of SR file to be opened */
  char *mask_name,     /* I: input mask filename of SR file to be opened */
  int fill_val         /* I: fill value for input data */
)
{
  char errstr[MAX_STR_LEN];           /* error string */
  char tmpstr[MAX_STR_LEN];           /* temporary string */
  char tmp_base_name[MAX_STR_LEN];    /* temporary copy of the base_name */
  char *cptr = NULL;                  /* character pointer */
  int ib;                             /* looping variable for bands */
  int nlines;                         /* number of lines in the image */
  int nsamps;                         /* number of samples in the image */

  /* Create the Input data structure */
  Input_t* ds_input = new Input_t();
  if (ds_input == NULL)
    RETURN_ERROR ("allocating Input data structure", "OpenInput", NULL);

  /* Populate the data structure */
  ds_input->base_name = DupString(base_name);
  if (ds_input->base_name == NULL) {
    RETURN_ERROR ("duplicating base file name", "OpenInput", NULL);
  }

  ds_input->mask_name = DupString(mask_name);
  if (ds_input->mask_name == NULL) {
    RETURN_ERROR ("duplicating mask name", "OpenInput", NULL);
  }

  /* Set up the band information for the input files */
  ds_input->nband = NUM_REFL_BAND;

  /* Open files for access */
  for (ib = 0; ib < ds_input->nband; ib++) {
    sprintf (tmpstr, "%s_%s", ds_input->base_name, refl_band_names[ib]);
    ds_input->fp_img[ib] = open_raw_binary (tmpstr, (char *)"rb");
    if (ds_input->fp_img[ib] == NULL) {
      sprintf (errstr, "opening input reflectance file: %s", tmpstr);
      RETURN_ERROR (errstr, "OpenInput", NULL);
    }
  }

  ds_input->fp_qa = open_raw_binary (ds_input->mask_name, (char *)"rb");
  if (ds_input->fp_qa == NULL) {
    sprintf (errstr, "opening input mask file: %s", ds_input->mask_name);
    RETURN_ERROR (errstr, "OpenInput", NULL);
  }

  /* Read the header file for band 1 to obtain the nlines and nsamps */
  sprintf (tmpstr, "%s_sr_band1.hdr", ds_input->base_name);
  if (!ReadHdr (tmpstr, &nlines, &nsamps)) {
    sprintf (errstr, "reading input header file: %s", tmpstr);
    RETURN_ERROR (errstr, "OpenInput", NULL);
  }
  ds_input->size.l = nlines;
  ds_input->size.s = nsamps;
  ds_input->meta.fill = fill_val;
  ds_input->open = true;

  /* Allocate the input reflectance image buffer */
  ds_input->img_buf = (int16 *) calloc (ds_input->size.s, sizeof (int16));
  if (ds_input->img_buf == NULL) {
      sprintf (errstr, "allocating input reflectance image buffer");
      RETURN_ERROR (errstr, "OpenInput", NULL);
  }

  /* Allocate the input QA/mask image buffer */
  ds_input->qa_buf = (int16 *) calloc (ds_input->size.s, sizeof (int16));
  if (ds_input->qa_buf == NULL) {
      sprintf (errstr, "allocating input QA/mask image buffer");
      RETURN_ERROR (errstr, "OpenInput", NULL);
  }

  /* Get the acquisition date (year) from the base filename
     (ex. LT50350321989265XXX03) */
  strcpy (tmp_base_name, base_name);
  cptr = strrchr (tmp_base_name, '/');
  if (cptr == NULL)
    cptr = &tmp_base_name[0];
  else
    cptr++;
  cptr += 9;   /* year is 9 characters in */
  cptr[4] = '\0';
  ds_input->meta.acq_year = atoi (cptr);

  return ds_input;
}


/******************************************************************************
MODULE: CloseInput

PURPOSE: Close the input file.
 
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
4/4/2014      Gail Schmidt     Modified to utilize the input ESPA file format
                               vs. HDF files

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

  /* Close image file pointers */
  for (ib = 0; ib < ds_input->nband; ib++) {
    close_raw_binary (ds_input->fp_img[ib]);
  }

  /* Close QA file pointer */
  close_raw_binary (ds_input->fp_qa);

  /* Mark file as closed */
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
4/4/2014      Gail Schmidt     Modified to utilize the input ESPA file format
                               vs. HDF files
4/9/2014      Gail Schmidt     Use a local image buffer for reading the data
                               vs. allocating space and freeing for each line
                               read.

NOTES:
*****************************************************************************/
bool FreeInput
(
  Input_t *ds_input   /* I: input data structure */
)
{
  if (ds_input != NULL) {
    if (ds_input->open)
      RETURN_ERROR("file still open", "FreeInput", false);

    /* Free the image buffers */
    free (ds_input->qa_buf);
    free (ds_input->img_buf);

    /* Free the filenames */
    free (ds_input->base_name);
    free (ds_input->mask_name);

    /* Free the structure */
    free (ds_input);
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
4/4/2014      Gail Schmidt     Modified to utilize the input ESPA file format
                               vs. HDF files
4/9/2014      Gail Schmidt     Use a local image buffer for reading the data
                               vs. allocating space and freeing for each line
                               read.

NOTES:
  1. Band data read is stored in class variable predMat (cv::Mat) as floating
     point values
*****************************************************************************/
bool PredictBurnedArea::GetInputData
(
  Input_t *ds_input,    /* I: input data structure */
  int iband             /* I: input band (0-based) */
)
{
  int samp;            /* looping variable */

  /* Check the parameters */
  if (ds_input == NULL)
    RETURN_ERROR("invalid input structure", "GetInputData", false);
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputData", false);
  if (iband < 0 || iband >= ds_input->nband)
    RETURN_ERROR("invalid band number", "GetInputData", false);

  /* Read the data */
  if (read_raw_binary (ds_input->fp_img[iband], 1, ds_input->size.s,
    sizeof (int16), ds_input->img_buf) != SUCCESS)
    RETURN_ERROR("reading input", "GetInputData", false)

  /* Grabbing bands 1-5 & 7 and putting value into predMat */
  for (samp = 0; samp < ds_input->size.s; samp++)
    predMat.at<float>(samp,iband) = ds_input->img_buf[samp];

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
12/8/2013     Gail Schmidt     Added support for the adjacent cloud QA band
4/4/2014      Gail Schmidt     Modified to utilize the input ESPA file format
                               vs. HDF files.  Also only using a single QA/mask
                               band at this time.
4/9/2014      Gail Schmidt     Use a local image buffer for reading the data
                               vs. allocating space and freeing for each line
                               read.

NOTES:
*****************************************************************************/
bool PredictBurnedArea::GetInputQALine
(
  Input_t *ds_input    /* I: input data structure */
)
{
  int samp;            /* looping variable */

  /* Check the parameters */
  if (ds_input == (Input_t *)NULL)
    RETURN_ERROR("invalid input structure", "GetIntputQALine", false);
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputQALine", false);

  /* Read the data */
  if (read_raw_binary (ds_input->fp_qa, 1, ds_input->size.s, sizeof (int16),
      ds_input->qa_buf) != SUCCESS)
    RETURN_ERROR("reading QA input", "GetInputQALine", false)

  /* Grabbing QA band and putting value into qaMat */
  for (samp = 0; samp < ds_input->size.s; samp++)
      qaMat.at<short>(samp) = ds_input->qa_buf[samp];

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
        if ((qaMat.at<short>(i) == INPUT_FILL_VALUE) ||
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
        if ((qaMat.at<short>(i) == INPUT_FILL_VALUE) ||
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
        if ((qaMat.at<short>(i) == INPUT_FILL_VALUE) ||
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
        if ((qaMat.at<short>(i) == INPUT_FILL_VALUE) ||
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

