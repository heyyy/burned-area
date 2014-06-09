#include "determine_max_extent.h"

/******************************************************************************
MODULE:  read_extent

PURPOSE:  Open the input reflectance XML file and reads the bounding extents
for that file in projection coords.

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
04/24/2013  Gail Schmidt     Original Development
03/10/2014  Gail Schmidt     Update to work with the ESPA internal file format

NOTES:
******************************************************************************/
int read_extent
(
    char *xml_infile,  /* I: input XML file to open and read */
    double *east,      /* O: eastern projection coordinate of the file */
    double *west,      /* O: western projection coordinate of the file */
    double *north,     /* O: northern projection coordinate of the file */
    double *south      /* O: southern projection coordinate of the file */
)
{
    char FUNC_NAME[] = "read_extent";   /* function name */
    char errmsg[STR_SIZE];    /* error message */
    int ib;                   /* loop counter for bands */
    int refl_indx = -1;       /* band index in XML file for reflectance band */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure */
    Espa_global_meta_t *gmeta = NULL;   /* pointer to global meta */

    /* Validate the input metadata file */
    if (validate_xml_file (xml_infile, ESPA_SCHEMA) != SUCCESS)
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

    /* Assign the corners for return to calling function */
    *east = gmeta->proj_info.lr_corner[0];
    *west = gmeta->proj_info.ul_corner[0];
    *south = gmeta->proj_info.lr_corner[1];
    *north = gmeta->proj_info.ul_corner[1];

    /* Handle projection corners that were specified as the center of the
       pixel */
    if (!strcmp (gmeta->proj_info.grid_origin, "CENTER"))
    {
        *west -= xml_metadata.band[refl_indx].pixel_size[0] * 0.5;
        *east += xml_metadata.band[refl_indx].pixel_size[0] * 0.5;
        *north += xml_metadata.band[refl_indx].pixel_size[1] * 0.5;
        *south -= xml_metadata.band[refl_indx].pixel_size[1] * 0.5;
    }

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Successful completion */
    return (SUCCESS);
}
