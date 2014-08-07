/*****************************************************************************
FILE: space.cpp
  
PURPOSE: Contains functions for geospatial-related functions for metadata.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
9/3/2013    Gail Schmidt     Original development (based largely on routines
                             from the LEDAPS lndsr application)

NOTES:
*****************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "space.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"
#include "HE2_config.h"
#include "proj.h"
#include "mystring.h"
#include "myproj.h"
#include "myproj_const.h"
#include "const.h"
#include "myhdf.h"
#include "error.h"

/* Constants */
#define MAX_PROJ (99)  /* Maximum map projection number */
#define GCTP_OK (0)    /* Okay status return from the GCTP package */

/* Prototypes for initializing the GCTP projections */
int for_init( int outsys, int outzone, double *outparm, int outdatum,
    char *fn27, char *fn83, int *iflg,
    int (*for_trans[])(double, double, double *, double *));
int inv_init( int insys, int inzone, double *inparm, int indatum,
    char *fn27, char *fn83, int *iflg,
    int (*inv_trans[])(double, double, double*, double*));

/* Functions */
#define NLINE_MAX (20000)
#define NSAMP_MAX (20000)

typedef enum {
  SPACE_NULL = -1,
  SPACE_START = 0,
  SPACE_PROJ_NUM,
  SPACE_PROJ_PARAM,
  SPACE_PIXEL_SIZE,
  SPACE_UL_CORNER,
  SPACE_NSAMPLE,
  SPACE_NLINE,
  SPACE_ZONE,
  SPACE_SPHERE,
  SPACE_ORIEN_ANGLE,
  SPACE_END,
  SPACE_MAX
} Space_key_t;

Key_string_t Space_string[SPACE_MAX] = {
  {(int)SPACE_START,       (char *)"HEADER_FILE"},
  {(int)SPACE_PROJ_NUM,    (char *)"PROJECTION_NUMBER"},
  {(int)SPACE_PROJ_PARAM,  (char *)"PROJECTION_PARAMETERS"},
  {(int)SPACE_PIXEL_SIZE,  (char *)"PIXEL_SIZE"},
  {(int)SPACE_UL_CORNER,   (char *)"UPPER_LEFT_CORNER"},
  {(int)SPACE_NSAMPLE,     (char *)"NSAMPLE"},
  {(int)SPACE_NLINE,       (char *)"NLINE"},
  {(int)SPACE_ZONE,        (char *)"PROJECTION_ZONE"},
  {(int)SPACE_SPHERE,      (char *)"PROJECTION_SPHERE"},
  {(int)SPACE_ORIEN_ANGLE, (char *)"ORIENTATION"},
  {(int)SPACE_END,         (char *)"END"}
};

#define SPACE_NTYPE_HDF (10)

typedef struct {
  int32 type;		/* type values */
  char *name;		/* type name */
} Myhdf_type_t;
Myhdf_type_t space_hdf_type[SPACE_NTYPE_HDF] = {
    {DFNT_CHAR8,   (char *)"DFNT_CHAR8"},
    {DFNT_UCHAR8,  (char *)"DFNT_UCHAR8"},
    {DFNT_INT8,    (char *)"DFNT_INT8"},
    {DFNT_UINT8,   (char *)"DFNT_UINT8"},
    {DFNT_INT16,   (char *)"DFNT_INT16"},
    {DFNT_UINT16,  (char *)"DFNT_UINT16"},
    {DFNT_INT32,   (char *)"DFNT_INT32"},
    {DFNT_UINT32,  (char *)"DFNT_UINT32"},
    {DFNT_FLOAT32, (char *)"DFNT_FLOAT32"},
    {DFNT_FLOAT64, (char *)"DFNT_FLOAT64"}
};

const char *SPACE_HDF_VERSION = "HDFVersion";
const char *SPACE_HDFEOS_VERSION = "HDFEOSVersion";
const char *SPACE_STRUCT_METADATA = "StructMetadata.0";
const char *SPACE_ORIENTATION_ANGLE_HDF = "OrientationAngle";
const char *SPACE_PIXEL_SIZE_HDF = "PixelSize";
#define NPROJ_PARAM_HDFEOS (13)


/******************************************************************************
MODULE:  AppendMeta

PURPOSE:  Appends the string to the metadata buffer.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error in the number of attributes
true       Successful processing

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
2/12/2012    Gail Schmidt     Original Development (based on input routines
                              from the LEDAPS lndsr application)

NOTES:
******************************************************************************/
bool AppendMeta
(
    char *cbuf,  /* I/O: input metadata buffer */
    int *ic,     /* I/O: index of current location in metadata buffer */
    char *s      /* I: string to append to the metadata buffer */
)
{
    int nc, i;
  
    /* Validate the string and number of attributes */
    if (ic < 0)
        return false;
    nc = strlen(s);
    if (nc <= 0)
        return false;
    if (*ic + nc > MYHDF_MAX_NATTR_VAL)
        return false;
  
    /* Add the string to the metadata */
    for (i = 0; i < nc; i++)
    {
        cbuf[*ic] = s[i];
        (*ic)++;
    }
  
    cbuf[*ic] = '\0';
  
    return true;
}

/******************************************************************************
MODULE:  put_space_def_hdf

PURPOSE:  Write the spatial definition attributes to the HDF file and move
the SDSs to the Grid.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error occurred writing the metadata to the HDF file
true       Successful completion

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
2/12/2012    Gail Schmidt     Original Development (based on input routines
                              from the LEDAPS lndsr application)

NOTES:
******************************************************************************/
bool PutSpaceDefHdf
(
    Space_def_t *myspace,  /* I: space definition structure */
    char *file_name,       /* I: HDF file to write attributes to */
    int nsds,              /* I: number of SDS to write */
    char sds_names[][MAX_STR_LEN], /* I: array of SDS names to write */
    int *sds_types,        /* I: array of types for each SDS */
    char *grid_name        /* I: name of the grid to write the SDSs to */
)
{
    int32 sds_file_id;
    char FUNC_NAME[] = "PutSpaceDefHdf";   /* function name */
    char errmsg[MAX_STR_LEN];              /* error message */
    char struct_meta[MYHDF_MAX_NATTR_VAL];
    char cbuf[MYHDF_MAX_NATTR_VAL];
    char hdf_version[] = H4_VERSION;
    char hdfeos_version[] = VERSION;
    char *dim_names[2] = {(char *)"YDim", (char *)"XDim"};
    int ic;
    Map_coord_t lr_corner;
    double ds, dl, dy, dx;
    double sin_orien, cos_orien;
    Myhdf_attr_t attr;
    int isds;
    int ip;
    double f_fractional, f_integral;
    char *cproj;
    double dval[1];
    char *ctype;
    int it;
    int32 hdf_id;
    int32 vgroup_id[3];
    int32 sds_index, sds_id;
  
    /* Check inputs */
    if (nsds <= 0) 
    {
        sprintf (errmsg, "Invalid number of SDSs for writing (less than 0)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    for (isds = 0; isds < nsds; isds++)
    {
        if (strlen(sds_names[isds]) < 1)  
        {
            sprintf (errmsg, "Invalid SDS name: %s", sds_names[isds]);
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }
    }
    if (strlen(grid_name) < 1)  
    {
        sprintf (errmsg, "Invalid grid name (empty string)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Put header */
    ic = 0;
    sprintf (cbuf, 
        "GROUP=SwathStructure\n" 
        "END_GROUP=SwathStructure\n" 
        "GROUP=GridStructure\n" 
        "\tGROUP=GRID_1\n");
  
    if (!AppendMeta (struct_meta, &ic, cbuf))
    {
        sprintf (errmsg, "Error appending to the start of the metadata string");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Compute lower right corner */
    dl = myspace->img_size.l * myspace->pixel_size;
    ds = myspace->img_size.s * myspace->pixel_size;
  
    sin_orien = sin(myspace->orientation_angle);
    cos_orien = cos(myspace->orientation_angle);
  
    dy = (ds * sin_orien) - (dl * cos_orien);
    dx = (ds * cos_orien) + (dl * sin_orien);
  
    lr_corner.y = myspace->ul_corner.y + dy;
    lr_corner.x = myspace->ul_corner.x + dx;
  
    /* Get the projection name string */
    cproj = (char *)NULL;
    for (ip = 0; ip < PROJ_NPROJ; ip++)
    {
        if (myspace->proj_num == ip)
        { 
            cproj = Proj_type[ip].short_name;
            break;
        }
    }
    if (cproj == (char *)NULL)
    {
        sprintf (errmsg, "Error getting the projection name string");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Put Grid information */
    sprintf(cbuf, 
        "\t\tGridName=\"%s\"\n" 
        "\t\tXDim=%d\n" 
        "\t\tYDim=%d\n" 
        "\t\tUpperLeftPointMtrs=(%.6f,%.6f)\n" 
        "\t\tLowerRightMtrs=(%.6f,%.6f)\n" 
        "\t\tProjection=GCTP_%s\n", 
        grid_name, 
        myspace->img_size.s, myspace->img_size.l, 
        myspace->ul_corner.x, myspace->ul_corner.y,
        lr_corner.x, lr_corner.y, 
        cproj);
  
    if (!AppendMeta (struct_meta, &ic, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (grid information "
            "start)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    if (myspace->proj_num == PROJ_UTM  ||  myspace->proj_num == PROJ_SPCS)
    {
        sprintf (cbuf, "\t\tZoneCode=%d\n", myspace->zone);
        if (!AppendMeta (struct_meta, &ic, cbuf))
        {
            sprintf (errmsg, "Error appending to metadata string (zone "
                "number)");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }
    }
    else
    {
        sprintf (cbuf, "\t\tProjParams=(");
        if (!AppendMeta (struct_meta, &ic, cbuf))
        {
            sprintf (errmsg, "Error appending to metadata string (grid "
                "projection parameters start)");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }
  
        for (ip = 0; ip < NPROJ_PARAM_HDFEOS; ip++)
        {
            f_fractional = modf (myspace->proj_param[ip], &f_integral);
            if (fabs (f_fractional) < 0.5e-6)
            {
                if (ip < (NPROJ_PARAM_HDFEOS - 1)) 
                    sprintf(cbuf, "%g,", myspace->proj_param[ip]);
                else 
                    sprintf(cbuf, "%g)", myspace->proj_param[ip]);
            }
            else
            {
                if (ip < (NPROJ_PARAM_HDFEOS + 1)) 
                    sprintf(cbuf, "%.6f,", myspace->proj_param[ip]);
                else 
                    sprintf(cbuf, "%.6f)", myspace->proj_param[ip]);
            }
  
            if (!AppendMeta (struct_meta, &ic, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string ("
                    "individual grid projection parameters)");
                RETURN_ERROR (errmsg, FUNC_NAME, false);
            }
        }
        sprintf(cbuf, "\n");
  
        if (!AppendMeta (struct_meta, &ic, cbuf))
        {
            sprintf (errmsg, "Error appending to metadata string (grid "
                "projection parameters end)");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }
    }
  
    sprintf (cbuf, 
        "\t\tSphereCode=%d\n" 
        "\t\tGridOrigin=HDFE_GD_UL\n",
        myspace->sphere);
  
    if (!AppendMeta (struct_meta, &ic, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (grid information "
            "end)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Put SDS group */
    sprintf (cbuf, 
        "\t\tGROUP=Dimension\n" 
        "\t\tEND_GROUP=Dimension\n"
        "\t\tGROUP=DataField\n");
  
    if (!AppendMeta (struct_meta, &ic, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (SDS group "
            "start)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    for (isds = 0; isds < nsds; isds++)
    {
        /* Get the hdf type name string */
        ctype = (char *)NULL;
        for (it = 0; it < SPACE_NTYPE_HDF; it++)
        {
            if (sds_types[isds] == space_hdf_type[it].type)
            { 
                ctype = space_hdf_type[it].name;
                break;
            }
        }

        if (ctype == (char *)NULL)
        {
            sprintf (errmsg, "Error getting hdf type name string");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }
  
        sprintf (cbuf, 
            "\t\t\tOBJECT=DataField_%d\n"
            "\t\t\t\tDataFieldName=\"%s\"\n"
            "\t\t\t\tDataType=%s\n"
            "\t\t\t\tDimList=(\"%s\",\"%s\")\n"
            "\t\t\tEND_OBJECT=DataField_%d\n",
            isds+1, sds_names[isds], ctype, dim_names[0], dim_names[1], isds+1);
  
        if (!AppendMeta (struct_meta, &ic, cbuf))
        {
            sprintf (errmsg, "Error appending to metadata string (SDS group)");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }
    }
  
    sprintf (cbuf, 
      "\t\tEND_GROUP=DataField\n" 
      "\t\tGROUP=MergedFields\n" 
      "\t\tEND_GROUP=MergedFields\n");
  
    if (!AppendMeta (struct_meta, &ic, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (SDS group end)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Put trailer */
    sprintf (cbuf, 
        "\tEND_GROUP=GRID_1\n"
        "END_GROUP=GridStructure\n"
        "GROUP=PointStructure\n"
        "END_GROUP=PointStructure\n"
        "END\n");
  
    if (!AppendMeta (struct_meta, &ic, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (tail)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Write file attributes */
    sds_file_id = SDstart ((char *)file_name, DFACC_RDWR);
    if (sds_file_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening file for SD access: %s", file_name);
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *)SPACE_ORIENTATION_ANGLE_HDF;
    dval[0] = (double)myspace->orientation_angle * DEG;
    if (!PutAttrDouble (sds_file_id, &attr, dval))
    {
        sprintf (errmsg, "Error writing attribute (orientation angle)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *)SPACE_PIXEL_SIZE_HDF;
    dval[0] = (double)myspace->pixel_size;
    if (!PutAttrDouble (sds_file_id, &attr, dval))
    {
        sprintf (errmsg, "Error writing attribute (pixel_size)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    attr.type = DFNT_CHAR8;
    attr.nval = strlen(hdf_version);
    attr.name = (char *)SPACE_HDF_VERSION;
    if (!PutAttrString (sds_file_id, &attr, hdf_version))
    {
        sprintf (errmsg, "Error writing attribute (hdf_version)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen(hdfeos_version);
    attr.name = (char *)SPACE_HDFEOS_VERSION;
    if (!PutAttrString (sds_file_id, &attr, hdfeos_version))
    {
        sprintf (errmsg, "Error writing attribute (hdfeos_version)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen(struct_meta);
    attr.name = (char *)SPACE_STRUCT_METADATA;
    if (!PutAttrString (sds_file_id, &attr, struct_meta))
    {
        sprintf (errmsg, "Error writing attribute (struct_meta)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    if (SDend (sds_file_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending SD access");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Setup the HDF Vgroup */
    hdf_id = Hopen ((char *)file_name, DFACC_RDWR, 0);
    if (hdf_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening the HDF file for Vgroup access: %s",
            file_name);
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Start the Vgroup access */
    if (Vstart (hdf_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error starting Vgroup access");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Create Root Vgroup for Grid */
    vgroup_id[0] = Vattach (hdf_id, -1, "w");
    if (vgroup_id[0] == HDF_ERROR) 
    {
        sprintf (errmsg, "Error getting Grid Vgroup ID");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vsetname (vgroup_id[0], grid_name) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error setting Grid Vgroup name");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vsetclass (vgroup_id[0], "GRID") == HDF_ERROR) 
    {
        sprintf (errmsg, "Error setting Grid Vgroup class");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Create Data Fields Vgroup */
    vgroup_id[1] = Vattach (hdf_id, -1, "w");
    if (vgroup_id[1] == HDF_ERROR) 
    {
        sprintf (errmsg, "Error getting Data Fields Vgroup ID");
    }
    if (Vsetname (vgroup_id[1], "Data Fields") == HDF_ERROR) 
    {
        sprintf (errmsg, "Error setting Data Fields Vgroup name");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vsetclass (vgroup_id[1], "GRID Vgroup") == HDF_ERROR) 
    {
        sprintf (errmsg, "Error setting Data Fields Vgroup class");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vinsert (vgroup_id[0], vgroup_id[1]) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error inserting Data Fields Vgroup");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Create Attributes Vgroup */
    vgroup_id[2] = Vattach (hdf_id, -1, "w");
    if (vgroup_id[2] == HDF_ERROR) 
    {
        sprintf (errmsg, "Error getting attributes Vgroup ID");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vsetname (vgroup_id[2], "Grid Attributes") == HDF_ERROR) 
    {
        sprintf (errmsg, "Error setting attributes Vgroup name");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vsetclass (vgroup_id[2], "GRID Vgroup") == HDF_ERROR) 
    {
        sprintf (errmsg, "Error setting attributes Vgroup class");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vinsert (vgroup_id[0], vgroup_id[2]) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error inserting attributes Vgroup");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Attach SDSs to Data Fields Vgroup */
    sds_file_id = SDstart ((char *)file_name, DFACC_RDWR);
    if (sds_file_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening output file for SD access");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    for (isds = 0; isds < nsds; isds++)
    {
        sds_index = SDnametoindex (sds_file_id, sds_names[isds]);
        if (sds_index == HDF_ERROR) 
        {
            sprintf (errmsg, "Error getting SDS index for SDS[%d] '%s' in "
                "file %s", isds, sds_names[isds], file_name);
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }

        sds_id = SDselect (sds_file_id, sds_index);
        if (sds_id == HDF_ERROR) 
        {
            sprintf (errmsg, "Error getting SDS ID");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }

        if (Vaddtagref (vgroup_id[1], DFTAG_NDG, SDidtoref(sds_id)) == 
            HDF_ERROR) 
        {
            sprintf (errmsg, "Error adding reference tag to SDS");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }

        if (SDendaccess (sds_id) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error ending access to SDS");
            RETURN_ERROR (errmsg, FUNC_NAME, false);
        }
    }
    
    if (SDend (sds_file_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending SD access");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Detach Vgroups */
    if (Vdetach (vgroup_id[0]) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error detaching from Grid Vgroup");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vdetach (vgroup_id[1]) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error detaching from Data Fields Vgroup");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Vdetach (vgroup_id[2]) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error detaching from Attributes Vgroup");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }

    /* Close access */
    if (Vend (hdf_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending Vgroup access");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (Hclose (hdf_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending HDF access");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    return (true);
}


/******************************************************************************
MODULE:  GetSpaceDefHdf

PURPOSE:  Read the spatial definition attributes from the HDF file.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error occurred reading the metadata from the HDF file
true       Successful completion

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
2/12/2012    Gail Schmidt     Original Development (based on input routines
                              from the LEDAPS lndsr application)

NOTES:
******************************************************************************/
bool GetSpaceDefHdf
(
    Space_def_t *myspace,/* I/O: spatial information structure which will be
                                 populated from the metadata */
    char *file_name,     /* I: name of the HDF file to read */
    char *grid_name      /* I: name of the grid to read metadata from */
)
{
    char FUNC_NAME[] = "GetSpaceDefHdf";   /* function name */
    char errmsg[MAX_STR_LEN]; /* error message */
    int32 gd_id;              /* HDF-EOS grid ID */
    int32 gd_file_id;         /* HDF-EOS file ID */
    int32 sds_file_id;        /* HDF file ID */
    int32 xdim_size;          /* number of elements in the x dimension */
    int32 ydim_size;          /* number of elements in the y dimension */
    float64 ul_corner[2];     /* UL corner projection coords (x,y) */
    float64 lr_corner[2];     /* LR corner projection coords (x,y) */
    int32 proj_num;           /* projection number */
    int32 zone;               /* UTM zone */
    int32 sphere;             /* sphere number */
    float64 proj_param[NPROJ_PARAM_HDFEOS];   /* projection parameters */
    int ip;                   /* looping variable */
    double dval[MYHDF_MAX_NATTR_VAL];  /* double attribute value read */
    Myhdf_attr_t attr;        /* HDF attributes read */

    /* Initialize the spatial information data structure */
    myspace->proj_num = -1;
    for (ip = 0; ip < NPROJ_PARAM; ip++)
        myspace->proj_param[ip] = 0.0;
    myspace->pixel_size = -1.0;
    myspace->ul_corner.x = -1.0;
    myspace->ul_corner.y = -1.0;
    myspace->ul_corner_set = false;
    myspace->img_size.l = -1;
    myspace->img_size.s = -1;
    myspace->zone = 0;
    myspace->zone_set = false;
    myspace->sphere = -1;
    myspace->isin_type = SPACE_NOT_ISIN;
    myspace->orientation_angle = 0.0;
  
    /* Open the HDF-EOS file for reading */
    gd_file_id = GDopen ((char *)file_name, DFACC_READ);
    if (gd_file_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening the HDF-EOS file: %s", file_name);
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Attach to the grid */
    gd_id = GDattach (gd_file_id, grid_name);
    if (gd_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error attaching to HDF-EOS grid: %s", grid_name);
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Get grid information */
    if (GDgridinfo (gd_id, &xdim_size, &ydim_size, ul_corner, lr_corner) ==
        HDF_ERROR)
    {
        sprintf (errmsg, "Error getting the HDF-EOS grid information");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    myspace->img_size.l = ydim_size;
    myspace->img_size.s = xdim_size;
    myspace->ul_corner.x = ul_corner[0];
    myspace->ul_corner.y = ul_corner[1];
    myspace->ul_corner_set = true;
  
    if (GDprojinfo (gd_id, &proj_num, &zone, &sphere, proj_param) == HDF_ERROR)
    {
        sprintf (errmsg, "Error getting HDF-EOS map projection information");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    myspace->proj_num = proj_num;
    if (myspace->proj_num == PROJ_UTM  ||  myspace->proj_num == PROJ_SPCS)
    {
        myspace->zone = zone;
        myspace->zone_set = true;
    }
    myspace->sphere = sphere;
    for (ip = 0; ip < NPROJ_PARAM_HDFEOS; ip++)
        myspace->proj_param[ip] = proj_param[ip];
  
    /* Close the HDF-EOS file */
    if (GDclose (gd_file_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error closing the HDF-EOS file.");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    /* Read file attributes */
    sds_file_id = SDstart ((char *)file_name, DFACC_READ);
    if (sds_file_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening HDF file for SD access: %s", file_name);
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *)SPACE_ORIENTATION_ANGLE_HDF;
    if (!GetAttrDouble (sds_file_id, &attr, dval))
    {
        sprintf (errmsg, "Error reading attribute (orientation angle)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (attr.nval != 1)
    {
        sprintf (errmsg, "Error invalid number of values (orientation angle)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    myspace->orientation_angle = dval[0] * RAD;   /* convert to radians */
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *)SPACE_PIXEL_SIZE_HDF;
    if (!GetAttrDouble(sds_file_id, &attr, dval))
    {
        sprintf (errmsg, "Error reading attribute (pixel size)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    if (attr.nval != 1)
    {
        sprintf (errmsg, "Error invalid number of values (pixel size)");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
    myspace->pixel_size = (float)dval[0];
  
    if (SDend(sds_file_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending SD access");
        RETURN_ERROR (errmsg, FUNC_NAME, false);
    }
  
    return (true);
}
