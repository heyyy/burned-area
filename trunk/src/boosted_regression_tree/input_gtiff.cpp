#include "input_gtiff.h"

/******************************************************************************
MODULE:  OpenGtifInput

PURPOSE:  Open the GeoTiff file and read the metadata.  Leave the TIFF file
pointer open in the returned Input_Gtif_t data structure.

RETURN VALUE:
Type = Input_Gtif_t *
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

NOTES:
******************************************************************************/
Input_Gtif_t *OpenGtifInput
(
    char *file_name       /* I: input geotiff filename */
)
{
    char errmsg[MAX_STR_LEN];/* error message */
    char tmpstr[MAX_STR_LEN];/* temporary pointer string message */
    uint32 nlines;           /* number of lines in tiff image */
    uint32 nsamps;           /* number of samples in tiff image */
    uint16 bitspersample;    /* bits per sample in tiff image */
    uint16 sampleformat;     /* data type of tiff image */
    uint16 coord_sys;        /* coordinate system used (PixelIsArea or
                                PixelIsPoint) */
    double tiePoint[6];      /* array for reading the tiepoints */
    double pixelScale[3];    /* array for reading the pixel size */
    uint16 count;            /* count of attributes to be read from tiff file */
    GTIF *fp_gtif=NULL;      /* geotiff key parser for input file */
    TIFF *fp_tiff=NULL;      /* file pointer for the TIFF file, just for quick
                                access */

    /* Create the Input data structure */
    Input_Gtif_t* ds_input = new Input_Gtif_t();
    if (ds_input == NULL)
        RETURN_ERROR ("allocating Input Gtif data structure", "OpenGtifInput",
            NULL);

    /* Open the input tiff file */
    if ((ds_input->fp_tiff = XTIFFOpen (file_name, "r")) == NULL)
    {
        sprintf (errmsg, "Error opening base TIFF file %s", file_name);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    } 
    fp_tiff = ds_input->fp_tiff;
    ds_input->open = true;

    /* Get metadata from tiff file */
    if (TIFFGetField (fp_tiff, TIFFTAG_IMAGELENGTH, &nlines) == 0)
    {
        sprintf (errmsg, "Error reading number of lines from base TIFF file "
            "%s", file_name);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }
    ds_input->size.l = (int) nlines;
  
    if (TIFFGetField (fp_tiff, TIFFTAG_IMAGEWIDTH, &nsamps) == 0)
    {
        sprintf (errmsg, "Error reading number of samples from base TIFF file "
            "%s", file_name);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }
    ds_input->size.s = (int) nsamps;
  
    if (TIFFGetField (fp_tiff, TIFFTAG_BITSPERSAMPLE, &bitspersample) == 0)
    {
        sprintf (errmsg, "Error reading bitspersample from base TIFF file "
            "%s", file_name);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }
  
    if (TIFFGetField (fp_tiff, TIFFTAG_SAMPLEFORMAT, &sampleformat) == 0)
    {
        sprintf (errmsg, "Error reading sampleformat from base TIFF file "
            "%s", file_name);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }

    /* Check to make sure the product is a 16-bit signed integer */
    if (bitspersample != 16)
    {
        sprintf (errmsg, "Input GeoTIFF band is expected to be a 16-bit "
            "integer but instead it is a %d-bit product", bitspersample);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }

    if (sampleformat != SAMPLEFORMAT_INT)
    {
        if (sampleformat == SAMPLEFORMAT_UINT)
            sprintf (tmpstr, "unsigned integer");
        else if (sampleformat == SAMPLEFORMAT_IEEEFP)
            sprintf (tmpstr, "float");
        else
            sprintf (tmpstr, "unknown");
        sprintf (errmsg, "Error: input GeoTIFF band is expected to be a "
            "signed integer but instead it is a %s product", tmpstr);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }

    count = 6;
    if (TIFFGetField (fp_tiff, TIFFTAG_GEOTIEPOINTS, &count, &tiePoint) == 0)
    {
        sprintf (errmsg, "Error reading tiepoints from base TIFF file %s",
            file_name);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }
    ds_input->ul[0] = tiePoint[3];
    ds_input->ul[1] = tiePoint[4];
  
    count = 3;
    if (TIFFGetField (fp_tiff, TIFFTAG_GEOPIXELSCALE, &count, &pixelScale) == 0)
    {
        sprintf (errmsg, "Error reading pixel size from base TIFF file %s",
            file_name);
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }
    ds_input->pixsize[0] = pixelScale[0];
    ds_input->pixsize[1] = pixelScale[1];

    /* Open the key parser for the geotiff file */
    fp_gtif = GTIFNew (fp_tiff);

    /* GTRasterTypeGeoKey dictates whether the reference coordinate is the UL
       (*RasterPixelIsArea*, code 1) or center (*RasterPixelIsPoint*, code 2)
       of the UL pixel. If this key is missing, the default (as defined by the
       specification) is to be *RasterPixelIsArea*, which is the UL of the UL
       pixel. */
    if (!GTIFKeyGet (fp_gtif, GTRasterTypeGeoKey, &coord_sys, 0, 1))
    {
        /* use a flag to specify that it wasn't in the current file */
        coord_sys = -99;
    }

    /* If the raster type is for the center of the pixel, then readjust to
       the UL corner of the pixel */
    if (coord_sys == RasterPixelIsPoint)
    {
        ds_input->ul[0] -= ds_input->pixsize[0] * 0.5;
        ds_input->ul[1] += ds_input->pixsize[1] * 0.5;
    }

    /* Close the geotiff key parser */
    GTIFFree (fp_gtif);

    /* Allocate the input buffer, assuming each file has only one band. */
    ds_input->buf = (int16 *) calloc (ds_input->size.s, sizeof (int16));
    if (ds_input->buf == NULL)
    {
        sprintf (errmsg, "allocating input buffer");
        RETURN_ERROR (errmsg, "OpenGtifInput", NULL);
    }

    return (ds_input);
}


/******************************************************************************
MODULE:  CloseGtifInput

PURPOSE:  Close the GeoTiff file and free the input buffer.

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

NOTES:
******************************************************************************/
bool CloseGtifInput
(
    Input_Gtif_t *ds_input     /* I: Pointer to the GeoTIFF file data struct */
)
{
    char errmsg[MAX_STR_LEN];   /* error message */

    /* Make sure file is open and available */
    if (!ds_input->open)
    {
        sprintf (errmsg, "file not open: %s", ds_input->file_name);
        RETURN_ERROR (errmsg, "CloseGtifInput", false);
    }

    /* Free the data buffer */
    if (ds_input->buf != NULL)
        free (ds_input->buf);

    /* Close the input tiff file */
    XTIFFClose (ds_input->fp_tiff);

    return true;
}


/******************************************************************************
MODULE:  GetGtifInputCYSummaryData

PURPOSE:  Read one line of seasonal summary data for the specified season and
band/index, and copy it to the associated PBA class array.

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

NOTES:
******************************************************************************/
bool PredictBurnedArea::GetGtifInputCYSummaryData
(
    Input_Gtif_t *ds_input,    /* I: Pointer to the GeoTIFF file data struct */
    int line,                  /* I: input line to be read */
    BandIndex_t band,          /* I: input band/index to be read */
    Season_t season            /* I: input season to be read */
)
{
    char errmsg[MAX_STR_LEN];   /* error message */
    int samp;                   /* current sample to be processed */

    /* Make sure file is open and available */
    if (!ds_input->open)
    {
        sprintf (errmsg, "file not open: %s", ds_input->file_name);
        RETURN_ERROR (errmsg, "GetGtifInputCYSummaryData", false);
    }
  
    /* Read the specified band from the GeoTIFF file */
    if (TIFFReadScanline (ds_input->fp_tiff, ds_input->buf, line, 0) == -1)
    {
        sprintf (errmsg, "Error reading line %d from the input file %s", line,
            ds_input->file_name);
        RETURN_ERROR (errmsg, "GetGtifInputCYSummaryData", false);
    } 

    /* Store the data as the group of bands/indices per season */
    for (samp = 0; samp < ds_input->size.s; samp++)
    {
        cySummaryMat.at<float>(samp,season*PBA_NBANDS+band) =
            ds_input->buf[samp];
    }

    return true;
}


/******************************************************************************
MODULE:  GetGtifInputLYSummaryData

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

NOTES:
******************************************************************************/
bool PredictBurnedArea::GetGtifInputLYSummaryData
(
    Input_Gtif_t *ds_input,    /* I: Pointer to the GeoTIFF file data struct */
    int line,                  /* I: input line to be read */
    BandIndex_t band,          /* I: input band/index to be read */
    Season_t season            /* I: input season to be read */
)
{
    char errmsg[MAX_STR_LEN];   /* error message */
    int samp;                   /* current sample to be processed */

    /* Make sure file is open and available */
    if (!ds_input->open)
    {
        sprintf (errmsg, "file not open: %s", ds_input->file_name);
        RETURN_ERROR (errmsg, "GetGtifInputLYSummaryData", false);
    }
  
    /* Read the specified band from the GeoTIFF file */
    if (TIFFReadScanline (ds_input->fp_tiff, ds_input->buf, line, 0) == -1)
    {
        sprintf (errmsg, "Error reading line %d from the input file %s", line,
            ds_input->file_name);
        RETURN_ERROR (errmsg, "GetGtifInputLYSummaryData", false);
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
MODULE:  GetGtifInputAnnualMaxData

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

NOTES:
******************************************************************************/
bool PredictBurnedArea::GetGtifInputAnnualMaxData
(
    Input_Gtif_t *ds_input,    /* I: Pointer to the GeoTIFF file data struct */
    int line,                  /* I: input line to be read */
    Index_t indx               /* I: input index to be read */
)
{
    char errmsg[MAX_STR_LEN];   /* error message */
    int samp;                   /* current sample to be processed */

    /* Make sure file is open and available */
    if (!ds_input->open)
    {
        sprintf (errmsg, "file not open: %s", ds_input->file_name);
        RETURN_ERROR (errmsg, "GetGtifInputAnnualMaxData", false);
    }
  
    /* Read the specified band from the GeoTIFF file */
    if (TIFFReadScanline (ds_input->fp_tiff, ds_input->buf, line, 0) == -1)
    {
        sprintf (errmsg, "Error reading line %d from the input file %s", line,
            ds_input->file_name);
        RETURN_ERROR (errmsg, "GetGtifInputAnnualMaxData", false);
    } 

    for (samp = 0; samp < ds_input->size.s; samp++)
        maxIndxMat.at<float>(samp,indx) = ds_input->buf[samp];

    return true;
}

