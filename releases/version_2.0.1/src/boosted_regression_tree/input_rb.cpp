/*****************************************************************************
FILE: input_rb.cpp
  
PURPOSE: Contains functions for opening, reading, closing, and processing
input raw binary products for seasonal summaries and annual maximums.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
4/9/2014      Gail Schmidt     Original Development

NOTES:
*****************************************************************************/

#include "input_rb.h"

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
MODULE:  OpenRbInput

PURPOSE:  Open the image file (for seasonal summaries and annual maximums) and
read the metadata.  Leave the file pointer open in the returned
Input_Rb_t data structure.

RETURN VALUE:
Type = Input_Rb_t *
Value           Description
-----           -----------
NULL            An error occurred during processing
non-NULL        Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
8/16/2013     Gail Schmidt     Original Development
4/8/2014      Gail Schmidt     Updated for raw binary

NOTES:
******************************************************************************/
Input_Rb_t *OpenRbInput
(
    char *file_name       /* I: input image filename */
)
{
    char errmsg[MAX_STR_LEN];     /* error message */
    char tmpstr[MAX_STR_LEN];     /* temporary pointer string message */
    char input_hdr[MAX_STR_LEN];  /* output header file */
    char *cptr = NULL;            /* character pointer */
    int nlines;                   /* number of lines in image */
    int nsamps;                   /* number of samples in image */

    /* Create the Input data structure */
    Input_Rb_t* ds_input = new Input_Rb_t();
    if (ds_input == NULL)
        RETURN_ERROR ("Error allocating input raw binary data structure",
            "OpenRbInput", NULL);

    /* Populate the data structure */
    ds_input->file_name = DupString(file_name);
    if (ds_input->file_name == NULL)
        RETURN_ERROR ("Error duplicating input raw binary file name",
            "OpenRbInput", NULL);

    /* Open the input raw binary file */
    ds_input->fp_img = open_raw_binary (file_name, (char *) "rb");
    if (ds_input->fp_img == NULL)
    {
        sprintf (errmsg, "Error opening input raw binary file: %s", file_name);
        RETURN_ERROR (errmsg, "OpenRbInput", NULL);
    }

    /* Read the header file to obtain the nlines and nsamps */
    strcpy (input_hdr, file_name);
    cptr = strrchr (input_hdr, '.');
    if (cptr == NULL)
    {
        sprintf (errmsg, "Error input filename doesn't match the expected .img "
            "file extension (%s)", file_name);
        RETURN_ERROR(errmsg, "OpenRbInput", false); 
    }
    strcpy (cptr, ".hdr");

    if (!ReadHdr (input_hdr, &nlines, &nsamps)) {
        sprintf (errmsg, "reading input header file: %s", tmpstr);
        RETURN_ERROR (errmsg, "OpenRbInput", NULL);
    }
    ds_input->size.l = nlines;
    ds_input->size.s = nsamps;
    ds_input->open = true;
  
    /* Allocate the input buffer */
    ds_input->buf = (int16 *) calloc (ds_input->size.s, sizeof (int16));
    if (ds_input->buf == NULL)
    {
        sprintf (errmsg, "allocating input raw binary buffer");
        RETURN_ERROR (errmsg, "OpenRbInput", NULL);
    }

    return (ds_input);
}


/******************************************************************************
MODULE:  CloseRbInput

PURPOSE:  Close the raw binary file.

RETURN VALUE:
Type = bool
Value           Description
-----           -----------
false           An error occurred during processing
true            Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
8/16/2013     Gail Schmidt     Original Development
4/8/2014      Gail Schmidt     Updated for raw binary

NOTES:
******************************************************************************/
bool CloseRbInput
(
    Input_Rb_t *ds_input   /* I: Pointer to the raw binary file data struct */
)
{
    char errmsg[MAX_STR_LEN];   /* error message */

    /* Make sure file is open and available */
    if (!ds_input->open)
    {
        sprintf (errmsg, "file not open: %s", ds_input->file_name);
        RETURN_ERROR (errmsg, "CloseRbInput", false);
    }

    /* Close the file pointer */
    close_raw_binary (ds_input->fp_img);

    /* Mark file as closed */
    ds_input->open = false;

    return true;
}


/******************************************************************************
MODULE:  FreeRbInput

PURPOSE:  Free the input data structure and memory.

RETURN VALUE:
Type = bool
Value           Description
-----           -----------
false           An error occurred during processing
true            Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
4/8/2014      Gail Schmidt     Original Development

NOTES:
******************************************************************************/
bool FreeRbInput
(
    Input_Rb_t *ds_input   /* I: Pointer to the raw binary file data struct */
)
{
    if (ds_input != NULL)
    {
        if (ds_input->open)
            RETURN_ERROR ("file still open", "FreeRbInput", false);

        /* Free the data buffer */
        free (ds_input->buf);

        /* Free the filename */
        free (ds_input->file_name);

        /* Free the structure */
        free (ds_input);
    }

    return true;
}


/******************************************************************************
MODULE:  GetRbInputLYSummaryData (class PredictBurnedArea)

PURPOSE:  Read one line of the previous years' seasonal summary data for the
specified season and band/index, and copy it to the associated PBA class array.

RETURN VALUE:
Type = bool
Value           Description
-----           -----------
false           An error occurred during processing
true            Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
8/16/2013     Gail Schmidt     Original Development
4/8/2014      Gail Schmidt     Updated for raw binary

NOTES:
******************************************************************************/
bool PredictBurnedArea::GetRbInputLYSummaryData
(
    Input_Rb_t *ds_input,  /* I: pointer to the raw binary file data struct */
    int line,              /* I: input line to be read */
    BandIndex_t band,      /* I: input band/index to be read */
    Season_t season        /* I: input season to be read */
)
{
    char errmsg[MAX_STR_LEN];   /* error message */
    int samp;                   /* current sample to be processed */

    /* Validate the line to be read */
    if (line < 0 || line >= ds_input->size.l)
        RETURN_ERROR("invalid line number", "GetRbInputLYSummaryData", false);

    /* Make sure file is open and available */
    if (!ds_input->open)
    {
        sprintf (errmsg, "file not open: %s", ds_input->file_name);
        RETURN_ERROR (errmsg, "GetRbInputLYSummaryData", false);
    }
  
    /* Read the specified line from the input file */
    if (read_raw_binary (ds_input->fp_img, 1, ds_input->size.s,
        sizeof (int16), ds_input->buf) != SUCCESS)
    {
        sprintf (errmsg, "Error reading line %d from the input file %s", line,
            ds_input->file_name);
        RETURN_ERROR (errmsg, "GetRbInputLYSummaryData", false);
    }

    /* Store the data as the group of bands/indices per season */
    for (samp = 0; samp < ds_input->size.s; samp++)
    {
        lySummaryMat.at<float>(samp,season*PBA_NBANDS+band) =
            ds_input->buf[samp];
    }

    return true;
}


/******************************************************************************
MODULE:  GetRbInputAnnualMaxData (class PredictBurnedArea)

PURPOSE:  Read one line of the annual maximum data for the specified index,
and copy it to the associated PBA class array.

RETURN VALUE:
Type = bool
Value           Description
-----           -----------
false           An error occurred during processing
true            Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
8/16/2013     Gail Schmidt     Original Development
4/8/2014      Gail Schmidt     Updated for raw binary

NOTES:
******************************************************************************/
bool PredictBurnedArea::GetRbInputAnnualMaxData
(
    Input_Rb_t *ds_input,  /* I: pointer to the raw binary file data struct */
    int line,              /* I: input line to be read */
    Index_t indx           /* I: input index to be read */
)
{
    char errmsg[MAX_STR_LEN];   /* error message */
    int samp;                   /* current sample to be processed */

    /* Validate the line to be read */
    if (line < 0 || line >= ds_input->size.l)
        RETURN_ERROR("invalid line number", "GetRbInputAnnualMaxData", false);

    /* Make sure file is open and available */
    if (!ds_input->open)
    {
        sprintf (errmsg, "file not open: %s", ds_input->file_name);
        RETURN_ERROR (errmsg, "GetRbInputAnnualMaxData", false);
    }
  
    /* Read the specified line from the input file */
    if (read_raw_binary (ds_input->fp_img, 1, ds_input->size.s,
        sizeof (int16), ds_input->buf) != SUCCESS)
    {
        sprintf (errmsg, "Error reading line %d from the input file %s", line,
            ds_input->file_name);
        RETURN_ERROR (errmsg, "GetRbInputLYSummaryData", false);
    }

    /* Store the data as the group of bands/indices per year */
    for (samp = 0; samp < ds_input->size.s; samp++)
        maxIndxMat.at<float>(samp,indx) = ds_input->buf[samp];

    return true;
}
