/*****************************************************************************
FILE: output.cpp
  
PURPOSE: Contains functions for creating and writing data to the output file.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
9/15/2012   Jodi Riegle      Original development (based largely on routines
                             from the LEDAPS lndsr application)
9/3/2013    Gail Schmidt     Modified to work in the ESPA environment
4/8/2014    Gail Schmidt     Modified to process data in the ESPA internal
                             raw binary format

NOTES:
*****************************************************************************/

#include <time.h>
#include "output.h"
#include "predict.h"

/* The following libraries are external C libraries from ESPA */
extern "C" {
FILE *open_raw_binary (char *infile, char *access_type);
void close_raw_binary (FILE *fptr);
int read_raw_binary (FILE *rb_fptr, int nlines, int nsamps, int size,
    void *img_array);
int write_raw_binary (FILE *rb_fptr, int nlines, int nsamps, int size,
    void *img_array);   
}


/******************************************************************************
MODULE: CreateOutputHeader

PURPOSE: Creates an output header file for the output image, using band 1 of
the input surface reflectance file
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error creating the output file
true           Successful creation

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool CreateOutputHeader
(
  char *base_name,     /* I: input base filename of SR file to be processed */
  char *output_file    /* I: name of output image file to create */
)
{
  char errmsg[MAX_STR_LEN];         /* error string */
  char band1_hdr[MAX_STR_LEN];      /* filename for band 1 header */
  char output_hdr[MAX_STR_LEN];     /* output header file */
  char *cptr = NULL;                /* character pointer */

  /* Create the band 1 header filename */
  sprintf (band1_hdr, "%s_sr_band1.hdr", base_name);

  /* Create the output header filename */
  strcpy (output_hdr, output_file);
  cptr = strrchr (output_hdr, '.');
  if (cptr == NULL) {
    sprintf (errmsg, "output filename doesn't match the expected .img "
      "file extension (%s)", output_file);
    RETURN_ERROR(errmsg, "CreateOutputHeader", false); 
  }
  strcpy (cptr, ".hdr");

  /* Copy input header file to the output header file */
  std::ifstream src (band1_hdr);
  if (!src) {
    sprintf (errmsg, "input header file doesn't exist (%s)", band1_hdr);
    RETURN_ERROR(errmsg, "CreateOutputHeader", false); 
  }

  std::ofstream dst(output_hdr);
  dst << src.rdbuf();

  return true;
}


/******************************************************************************
MODULE: OpenOutput

PURPOSE: Sets up the 'output' data structure and opens the output file for
write access.
 
RETURN VALUE:
Type = Output_t*
Value          Description
-----          -----------
NULL           Error opening the output file or creating it
non-NULL       Successful creation

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
4/8/2014      Gail Schmidt     Modified to use the ESPA raw binary file format

NOTES:
*****************************************************************************/
Output_t *OpenOutput
(
  char *file_name,        /* I: output filename to be created */
  Img_coord_int_t *size   /* I: image size of the file to be created */
)
{
  Output_t *ds_output = NULL;   /* output structure to be populated */

  /* Create the Output data structure */
  ds_output = (Output_t *) malloc (sizeof (Output_t));
  if (ds_output == NULL)
    RETURN_ERROR("allocating Output data structure", "OpenOutput", NULL);

  /* Populate the data structure */
  ds_output->file_name = DupString (file_name);
  if (ds_output->file_name == (char *)NULL)
    RETURN_ERROR("duplicating file name", "OpenOutput", NULL);

  ds_output->open = false;
  ds_output->size.l = size->l;
  ds_output->size.s = size->s;

  /* Open file for write access */
  ds_output->fp_img = open_raw_binary (file_name, (char *) "wb");
  if (ds_output->fp_img == NULL)
    RETURN_ERROR("unable to open output image file", "OpenOutput", NULL);
  ds_output->open = true;

  /* Allocate the output buffer */
  ds_output->buf = (int16 *) calloc (ds_output->size.s, sizeof (int16));
  if (ds_output->buf == NULL)
    RETURN_ERROR ("allocating output buffer", "OpenOutput", NULL);

  return ds_output;
}


/******************************************************************************
MODULE: CloseOutput

PURPOSE: Closes the output file.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error closing the output file
true           Successful close

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
4/8/2014      Gail Schmidt     Modified to use the ESPA raw binary file format

NOTES:
*****************************************************************************/
bool CloseOutput
(
  Output_t *ds_output   /* I: output data structure to be closed */
)
{
  if (!ds_output->open)
    RETURN_ERROR("file not open", "CloseOutput", false);

  /* Close the file pointer */
  close_raw_binary (ds_output->fp_img);
  ds_output->open = false;

  return true;
}


/******************************************************************************
MODULE: FreeOutput

PURPOSE: Frees the 'output' data structure memory
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error freeing the data structure memory
true           Successfully freed the memory

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
4/8/2014      Gail Schmidt     Modified to use the ESPA raw binary file format

NOTES:
*****************************************************************************/
bool FreeOutput
(
  Output_t *ds_output   /* I: output data structure to be freed */
)
{
  if (ds_output != NULL) {
    if (ds_output->open)
      RETURN_ERROR("file still open", "FreeOutput", false);

    /* Free the data buffer */
    free (ds_output->buf);

    /* Free the filename */
    free (ds_output->file_name);

    /* Free the structure */
    free(ds_output);
  }

  return true;
}


/******************************************************************************
MODULE: PutOutputLine (class PredictBurnedArea)

PURPOSE: Writes a line of data to the output file.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error writing the line to the output file
true           Successfully wrote the line to the output file

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
4/8/2014      Gail Schmidt     Modified to use the ESPA raw binary file format

NOTES:
*****************************************************************************/
bool PredictBurnedArea::PutOutputLine
(
  Output_t *ds_output,   /* I: 'output' data structure where buf contains
                               the line to be written */
  int iline              /* I: current line to be written (0-based) */
)
{
  /* Check the parameters */
  if (ds_output == (Output_t *)NULL)
    RETURN_ERROR("invalid output structure", "PutOutputLine", false);
  if (!ds_output->open)
    RETURN_ERROR("file not open", "PutOutputLine", false);
  if (iline < 0 || iline >= ds_output->size.l)
    RETURN_ERROR("invalid line number", "PutOutputLine", false);

  /* Write the data */
  if (write_raw_binary (ds_output->fp_img, 1, ds_output->size.s, sizeof (int16),
    ds_output->buf) != SUCCESS)
    RETURN_ERROR("writing output", "PutOutputLine", false);

  return true;
}

