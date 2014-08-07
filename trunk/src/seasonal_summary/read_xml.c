#include "generate_stack.h"

/******************************************************************************
MODULE:  read_xml

PURPOSE:  Open the input reflectance XML file and reads the desired metadata
for that file.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           An error occurred during processing of the XML file
SUCCESS         Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date        Programmer       Reason
----------  ---------------  -------------------------------------
3/12/2014   Gail Schmidt     Original Development

NOTES:
******************************************************************************/
int read_xml
(
    char *xml_infile,            /* I: input XML file to open and read */
    Ba_scene_meta_t *scene_meta  /* O: scene metadata */
)
{
    char FUNC_NAME[] = "read_xml";   /* function name */
    char errmsg[STR_SIZE];    /* error message */
    int ib;                   /* loop counter for bands */
    int count;                /* count of chars written via snprintf */
    int refl_indx = -1;       /* band index in XML file for reflectance band */
    int nday[12] = {31, 29, 31, 30,  31,  30,  31,  31,  30,  31,  30,  31};
    int idoy[12] = { 1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336};
    bool leap;                /* is this a leap year? */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure */
    Espa_global_meta_t *gmeta = NULL;   /* pointer to global meta */

    /* Validate the input metadata file */
    if (validate_xml_file (xml_infile) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Initialize the metadata structure */
    init_metadata_struct (&xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata (xml_infile, &xml_metadata) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }
    gmeta = &xml_metadata.global;

    /* Use the surface reflectance band as the input band to pull the
       corner points */
    for (ib = 0; ib < xml_metadata.nbands; ib++)
    {
        if (!strcmp (xml_metadata.band[ib].name, "sr_band1") &&
            !strcmp (xml_metadata.band[ib].product, "sr_refl"))
        {
            /* this is the index we'll use for reflectance band info */
            refl_indx = ib;
            break;
        }
    }

    /* Make sure we found the band */
    if (refl_indx == -1)
    {
        sprintf (errmsg, "Unable to find the surface reflectance band1 in "
            "the XML file.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Use the XML filename for the scene filename */
    count = snprintf (scene_meta->filename, sizeof (scene_meta->filename), "%s",
        xml_infile);
    if (count < 0 || count >= sizeof (scene_meta->filename))
    {
        sprintf (errmsg, "Overflow of scene_meta->filename string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Assign the XML metadata to the scene metadata */
    scene_meta->wrs_path = gmeta->wrs_path;
    scene_meta->wrs_row = gmeta->wrs_row;
    scene_meta->bounding_coords[ESPA_WEST] = gmeta->bounding_coords[ESPA_WEST];
    scene_meta->bounding_coords[ESPA_EAST] = gmeta->bounding_coords[ESPA_EAST];
    scene_meta->bounding_coords[ESPA_NORTH] =
        gmeta->bounding_coords[ESPA_NORTH];
    scene_meta->bounding_coords[ESPA_SOUTH] =
        gmeta->bounding_coords[ESPA_SOUTH];
    scene_meta->nlines = xml_metadata.band[refl_indx].nlines;
    scene_meta->nsamps = xml_metadata.band[refl_indx].nsamps;
    scene_meta->pixel_size[0] = xml_metadata.band[refl_indx].pixel_size[0];
    scene_meta->pixel_size[1] = xml_metadata.band[refl_indx].pixel_size[1];
    scene_meta->utm_zone = gmeta->proj_info.utm_zone;

    count = snprintf (scene_meta->satellite, sizeof (scene_meta->satellite),
        "%s", gmeta->satellite);
    if (count < 0 || count >= sizeof (scene_meta->satellite))
    {
        sprintf (errmsg, "Overflow of scene_meta->satellite string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Handle the acquisition date, which needs to be split from a string
       yyyy-mm-dd to month, day, year, julian DOY */
    if (sscanf (gmeta->acquisition_date, "%4d-%2d-%2d",
        &scene_meta->acq_date.year, &scene_meta->acq_date.month,
        &scene_meta->acq_date.day) != 3) 
    {
        sprintf (errmsg, "Invalid acquisition date format: %s",
            gmeta->acquisition_date);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (scene_meta->acq_date.year < 1900 || scene_meta->acq_date.year > 2400) 
    {
        sprintf (errmsg, "Invalid acquisition date format: %s.  Year out "
            "of range.", gmeta->acquisition_date);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    if (scene_meta->acq_date.month < 1 || scene_meta->acq_date.month > 12) 
    {
        sprintf (errmsg, "Invalid acquisition date format: %s.  Month out "
            "of range.", gmeta->acquisition_date);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    if (scene_meta->acq_date.day < 1 ||
        scene_meta->acq_date.day > nday[scene_meta->acq_date.month-1])
    {
        sprintf (errmsg, "Invalid acquisition date format: %s.  Day out "
            "of range.", gmeta->acquisition_date);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    scene_meta->acq_date.doy = scene_meta->acq_date.day +
        idoy[scene_meta->acq_date.month - 1] - 1;

    /* Handle leap year */
    leap = (scene_meta->acq_date.year % 4 == 0 &&
        (scene_meta->acq_date.year % 100 != 0 ||
         scene_meta->acq_date.year % 400 == 0));
    if (!leap)
    {
        if (scene_meta->acq_date.month == 2 && scene_meta->acq_date.day > 28)
        {
            sprintf (errmsg, "Invalid acquisition date: %s.  Month out "
                "of range for leap year.", gmeta->acquisition_date);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (scene_meta->acq_date.month > 2)
            scene_meta->acq_date.doy--;
    } 

    /* Determine the season */
    if (scene_meta->acq_date.month == 12 ||
        scene_meta->acq_date.month == 1 || scene_meta->acq_date.month == 2)
        strcpy (scene_meta->season, "winter");
    else if (scene_meta->acq_date.month >= 3 &&
        scene_meta->acq_date.month <= 5)
        strcpy (scene_meta->season, "spring");
    else if (scene_meta->acq_date.month >= 6 &&
        scene_meta->acq_date.month <= 8)
        strcpy (scene_meta->season, "summer");
    else
        strcpy (scene_meta->season, "fall");

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Successful completion */
    return (SUCCESS);
}
