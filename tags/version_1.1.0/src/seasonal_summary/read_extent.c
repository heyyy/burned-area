#include "determine_max_extent.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"

/******************************************************************************
MODULE:  read_extent

PURPOSE:  Open the input reflectance and read the bounding extents for that
file in projection coords.

RETURN VALUE:
Type = None

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date        Programmer       Reason
----------  ---------------  -------------------------------------
04/24/2013  Gail Schmidt     Original Development

NOTES:
******************************************************************************/
int read_extent
(
    char *infile,    /* I: input reflectance file to open and process */
    char *grid_name, /* I: name of the grid to read metadata from */
    double *east,    /* O: eastern projection coordinate of the file */
    double *west,    /* O: western projection coordinate of the file */
    double *north,   /* O: northern projection coordinate of the file */
    double *south    /* O: southern projection coordinate of the file */
)
{
    char FUNC_NAME[] = "read_extent";   /* function name */
    char errmsg[STR_SIZE];    /* error message */
    int32 gd_file_id;         /* HDF-EOS file ID */
    int32 gd_id;              /* HDF-EOS grid ID */
    int32 xdim_size;          /* number of elements in the x dimension */
    int32 ydim_size;          /* number of elements in the y dimension */
    float64 ul_corner[2];     /* UL corner projection coords (x,y) */
    float64 lr_corner[2];     /* LR corner projection coords (x,y) */

    /* Open the HDF-EOS file for reading */
    gd_file_id = GDopen (infile, DFACC_READ);
    if (gd_file_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening the HDF-EOS file: %s", infile);
        error_handler (false, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Attach to the grid */
    gd_id = GDattach (gd_file_id, grid_name);
    if (gd_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error attaching to HDF-EOS grid: %s", grid_name);
        error_handler (false, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Get the grid information */
    if (GDgridinfo (gd_id, &xdim_size, &ydim_size, ul_corner, lr_corner) ==
        HDF_ERROR)
    {
        sprintf (errmsg, "Error getting the HDF-EOS grid information");
        error_handler (false, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Assign the corners for return to calling function */
    *east = lr_corner[0];
    *west = ul_corner[0];
    *south = lr_corner[1];
    *north = ul_corner[1];

    /* Detach from the grid */
    if (GDdetach (gd_id) == HDF_ERROR)
    {
        sprintf (errmsg, "Error detaching from the current grid");
        error_handler (false, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Close the HDF-EOS file */
    if (GDclose (gd_file_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error closing the HDF-EOS file.");
        error_handler (false, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Successful completion */
    return (SUCCESS);
}
