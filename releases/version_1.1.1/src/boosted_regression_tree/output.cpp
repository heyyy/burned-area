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

NOTES:
*****************************************************************************/

#include <time.h>
#include "output.h"
#include "predict.h"

const char *OUTPUT_PROVIDER = "DataProvider";
const char *OUTPUT_SAT = "Satellite";
const char *OUTPUT_INST = "Instrument";
const char *OUTPUT_ACQ_DATE = "AcquisitionDate";
const char *OUTPUT_L1_PROD_DATE = "Level1ProductionDate";
const char *OUTPUT_SUN_ZEN = "SolarZenith";
const char *OUTPUT_SUN_AZ = "SolarAzimuth";
const char *OUTPUT_WRS_SYS = "WRS_System";
const char *OUTPUT_WRS_PATH = "WRS_Path";
const char *OUTPUT_WRS_ROW = "WRS_Row";
const char *OUTPUT_SHORT_NAME = "ShortName";
const char *OUTPUT_PROD_DATE = "ProductionDate";
const char *OUTPUT_PGEVERSION = "PGEVersion";
const char *OUTPUT_PROCESSVERSION = "ProcessVersion";
const char *OUTPUT_BAVERSION = "BurnAreaMappingVersion";

const char *OUTPUT_WEST_BOUND = "WestBoundingCoordinate";
const char *OUTPUT_EAST_BOUND = "EastBoundingCoordinate";
const char *OUTPUT_NORTH_BOUND = "NorthBoundingCoordinate";
const char *OUTPUT_SOUTH_BOUND = "SouthBoundingCoordinate";
const char *UL_LAT_LONG = "UpperLeftCornerLatLong";
const char *LR_LAT_LONG = "LowerRightCornerLatLong";

const char *OUTPUT_LONG_NAME = "long_name";
const char *OUTPUT_UNITS = "units";
const char *OUTPUT_VALID_RANGE = "valid_range";
const char *OUTPUT_FILL_VALUE = "_FillValue";
const char *OUTPUT_SATU_VALUE = "_SaturateValue";
const char *OUTPUT_SCALE_FACTOR = "scale_factor";
const char *OUTPUT_ADD_OFFSET = "add_offset";
const char *OUTPUT_SCALE_FACTOR_ERR = "scale_factor_err";
const char *OUTPUT_ADD_OFFSET_ERR = "add_offset_err";
const char *OUTPUT_CALIBRATED_NT = "calibrated_nt";
const char *OUTPUT_QAMAP_INDEX = "qa_bitmap_index";

/******************************************************************************
MODULE: CreateOutput

PURPOSE: Creates a new output HDF file and associated ENVI header
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error creating the output file
true           Successful creation

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool CreateOutput
(
  char *file_name,       /* I: name of the output HDF file to create */
  char *input_header,    /* I: name of input lndsr HDF .hdr ENVI header file */
  char *output_header    /* I: name of output HDF .hdr file to create */
)
{
  int32 hdf_file_id;
  char errmsg[1000];

  /* Create the file with HDF open */
  hdf_file_id = Hopen(file_name, DFACC_CREATE, DEF_NDDS); 
  if (hdf_file_id == HDF_ERROR) {
    RETURN_ERROR("creating output file", "CreateOutput", false); 
  }

  /* Close the file */
  Hclose(hdf_file_id);

  /* Copy input header file to the output header file */
  std::ifstream src(input_header);
  if (!src) {
    sprintf (errmsg, "input header file doesn't exist (%s)", input_header);
    RETURN_ERROR(errmsg, "CreateOutput", false); 
  }

  std::ofstream dst(output_header);
  dst << src.rdbuf();

  return true;
}


/******************************************************************************
MODULE: OpenOutput

PURPOSE: Sets up the 'output' data structure, opens the output file for write
access, and creates the output SDS.
 
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

NOTES:
*****************************************************************************/
Output_t *OpenOutput
(
  char *file_name,        /* I: output filename to be created */
  int nband,              /* I: number of image bands (SDSs) to be created */
  char sds_names[NBAND_MAX_OUT][MAX_STR_LEN],  /* I: array of SDS names for
                                each image band */
  Img_coord_int_t *size   /* I: image size of the file to be created */
)
{
  Output_t *ds_output = NULL;
  char *error_string = NULL;
  Myhdf_dim_t *dim[MYHDF_MAX_RANK];
  Myhdf_sds_t *sds = NULL;
  int ir, ib;
  int16 *buf = NULL;

  /* Check parameters */
  if (size->l < 1)
    RETURN_ERROR("invalid number of output lines", "OpenOutput", NULL);

  if (size->s < 1)
    RETURN_ERROR("invalid number of samples per output line", "OpenOutput",
      NULL);

  if (nband < 1 || nband > NBAND_MAX_OUT)
    RETURN_ERROR("invalid number of bands", "OpenOutput", NULL);

  /* Create the Output data structure */
  ds_output = (Output_t *)malloc(sizeof(Output_t));
  if (ds_output == NULL)
    RETURN_ERROR("allocating Output data structure", "OpenOutput", NULL);

  /* Populate the data structure */
  ds_output->file_name = DupString(file_name);
  if (ds_output->file_name == (char *)NULL) {
    free(ds_output);
    RETURN_ERROR("duplicating file name", "OpenOutput", NULL);
  }

  ds_output->open = false;
  ds_output->nband = nband;
  ds_output->size.l = size->l;
  ds_output->size.s = size->s;

  for (ib = 0; ib < ds_output->nband; ib++) {
    ds_output->sds[ib].name = NULL;
    ds_output->sds[ib].dim[0].name = NULL;
    ds_output->sds[ib].dim[1].name = NULL;
    ds_output->buf[ib] = NULL;
  }

  /* Open file for SD access */
  ds_output->sds_file_id = SDstart((char *)file_name, DFACC_RDWR);
  if (ds_output->sds_file_id == HDF_ERROR) {
    free(ds_output->file_name);
    free(ds_output);
    RETURN_ERROR("opening output file for SD access", "OpenOutput", NULL); 
  }
  ds_output->open = true;

  /* Set up the image SDSs */
  for (ib = 0; ib < ds_output->nband; ib++) {
    sds = &ds_output->sds[ib];
    sds->rank = 2;
    sds->type = DFNT_INT16;
    sds->name = DupString(sds_names[ib]);
    if (sds->name == NULL) {
      CloseOutput (ds_output);
      FreeOutput (ds_output);
      error_string = (char *) "duplicating sds name";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }

    dim[0] = &sds->dim[0];
    dim[1] = &sds->dim[1];

    dim[0]->nval = ds_output->size.l;
    dim[1]->nval = ds_output->size.s;

    dim[0]->type = dim[1]->type = sds->type;

    dim[0]->name = DupString((char *) "YDim_Grid");
    if (dim[0]->name == NULL) {
      CloseOutput (ds_output);
      FreeOutput (ds_output);
      error_string = (char *) "duplicating dim name (l)";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }
    dim[1]->name = DupString((char *) "XDim_Grid");
    if (dim[1]->name == NULL) {
      CloseOutput (ds_output);
      FreeOutput (ds_output);
      error_string = (char *) "duplicating dim name (s)";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }

    if (!PutSDSInfo(ds_output->sds_file_id, sds)) {
      CloseOutput (ds_output);
      FreeOutput (ds_output);
      error_string = (char *) "setting up the SDS";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }

    for (ir = 0; ir < sds->rank; ir++) {
      if (!PutSDSDimInfo(sds->id, dim[ir], ir)) {
        CloseOutput (ds_output);
        FreeOutput (ds_output);
        error_string = (char *) "setting up the dimension";
        RETURN_ERROR(error_string, "OpenOutput", NULL); 
      }
    }
  }  /* end for image bands */

  /* Allocate output buffers */
  buf = (int16 *)calloc((size_t)(ds_output->size.s * ds_output->nband),
    sizeof(int16));
  if (buf == NULL)
    error_string = (char *) "allocating output buffer";
  else {
    ds_output->buf[0] = buf;
    for (ib = 1; ib < ds_output->nband; ib++)
      ds_output->buf[ib] = ds_output->buf[ib - 1] + ds_output->size.s;
  }

  if (error_string != NULL) {
    CloseOutput (ds_output);
    FreeOutput (ds_output);
    RETURN_ERROR(error_string, "OpenOutput", (Output_t *)NULL); 
  }

  return ds_output;
}


/******************************************************************************
MODULE: CloseOutput

PURPOSE: Ends SDS access and closes the output file.
 
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

NOTES:
*****************************************************************************/
bool CloseOutput
(
  Output_t *ds_output   /* I: output data structure to be closed */
)
{
  int ib;

  if (!ds_output->open)
    RETURN_ERROR("file not open", "CloseOutput", false);

  /* Close image SDSs */
  for (ib = 0; ib < ds_output->nband; ib++) {
    if (SDendaccess(ds_output->sds[ib].id) == HDF_ERROR)
      RETURN_ERROR("ending sds access", "CloseOutput", false);
  }

  /* Close the HDF file itself */
  SDend(ds_output->sds_file_id);
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

NOTES:
*****************************************************************************/
bool FreeOutput
(
  Output_t *ds_output   /* I: output data structure to be freed */
)
{
  int ir, ib;

  if (ds_output->open)
    RETURN_ERROR("file still open", "FreeOutput", false);

  if (ds_output != NULL) {
    /* Free image band SDSs */
    for (ib = 0; ib < ds_output->nband; ib++) {
      for (ir = 0; ir < ds_output->sds[ib].rank; ir++) {
        if (ds_output->sds[ib].dim[ir].name != NULL)
          free(ds_output->sds[ib].dim[ir].name);
      }
      if (ds_output->sds[ib].name != NULL)
        free(ds_output->sds[ib].name);
    }

    if (ds_output->buf[0] != NULL)
      free(ds_output->buf[0]);

    if (ds_output->file_name != NULL)
      free(ds_output->file_name);
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

NOTES:
*****************************************************************************/
bool PredictBurnedArea::PutOutputLine
(
  Output_t *ds_output,   /* I: 'output' data structure where buf contains
                               the line to be written */
  int iband,             /* I: current band to be written (0-based) */
  int iline              /* I: current line to be written (0-based) */
)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];
  void *buf = NULL;

  /* Check the parameters */
  if (ds_output == (Output_t *)NULL)
    RETURN_ERROR("invalid input structure", "PutOutputLine", false);
  if (!ds_output->open)
    RETURN_ERROR("file not open", "PutOutputLine", false);
  if (iband < 0 || iband >= ds_output->nband)
    RETURN_ERROR("invalid band number", "PutOutputLine", false);
  if (iline < 0 || iline >= ds_output->size.l)
    RETURN_ERROR("invalid line number", "PutOutputLine", false);

  /* Write the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_output->size.s;
  buf = (void *)ds_output->buf[iband];
  if (SDwritedata(ds_output->sds[iband].id, start, NULL, nval, buf)
      == HDF_ERROR)
    RETURN_ERROR("writing output", "PutOutputLine", false);

  return true;
}


/******************************************************************************
MODULE: PutMetadata

PURPOSE: Writes metadata to the output file.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error writing the metadata to the output file
true           Successfully wrote the metadata to the output file

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool PutMetadata
(
  Output_t *ds_output,   /* I: 'output' data structure */
  int nband,             /* I: number of image bands (SDSs) to be created */
  char sds_names[NBAND_MAX_OUT][MAX_STR_LEN],  /* I: array of SDS names for
                                each image band */
  Input_meta_t *meta     /* I: metadata to be written */
)
{
  Myhdf_attr_t attr;
  double dval[NBAND_MAX_OUT];
  char string[250],
     short_name[250],
     date[MAX_DATE_LEN + 1],
     prod_date[MAX_DATE_LEN + 1],
     process_ver[100],
     long_name[250],
     units_b[250];             /* units string for current band */
  int ib;
  time_t tp;                    /* structure for obtaining current time */
  struct tm *tm = NULL;         /* structure for obtaining current time
                                   in UTC format */
  char ba_map_QAMAP[1000];

  /* Check the parameters */
  if (!ds_output->open)
    RETURN_ERROR("file not open", "PutMetadata", false);

  if (nband < 1 || nband > NBAND_MAX_OUT)
    RETURN_ERROR("invalid number of bands", "PutMetadata", false);

  /* Write the metadata */
  attr.id = -1;

  strcpy (string, "USGS/EROS");
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(string);
  attr.name = (char *) OUTPUT_PROVIDER;
  if (!PutAttrString(ds_output->sds_file_id, &attr, string))
    RETURN_ERROR("writing attribute (data provider)", "PutMetadata", false);

  attr.type = DFNT_CHAR8;
  attr.nval = strlen(meta->sat);
  attr.name = (char *) OUTPUT_SAT;
  if (!PutAttrString(ds_output->sds_file_id, &attr, meta->sat))
    RETURN_ERROR("writing attribute (satellite)", "PutMetadata", false);

  attr.type = DFNT_CHAR8;
  attr.nval = strlen(meta->inst);
  attr.name = (char *) OUTPUT_INST;
  if (!PutAttrString(ds_output->sds_file_id, &attr, meta->inst))
    RETURN_ERROR("writing attribute (instrument)", "PutMetadata", false);

  if (!FormatDate(&meta->acq_date, DATE_FORMAT_DATEA_TIME, date))
    RETURN_ERROR("formating acquisition date", "PutMetadata", false);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(date);
  attr.name = (char *) OUTPUT_ACQ_DATE;
  if (!PutAttrString(ds_output->sds_file_id, &attr, date))
    RETURN_ERROR("writing attribute (acquisition date)", "PutMetadata", false);

  if (!FormatDate(&meta->prod_date, DATE_FORMAT_DATEA_TIME, date))
    RETURN_ERROR("formating production date", "PutMetadata", false);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(date);
  attr.name = (char *) OUTPUT_L1_PROD_DATE;
  if (!PutAttrString(ds_output->sds_file_id, &attr, date))
    RETURN_ERROR("writing attribute (production date)", "PutMetadata", false);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) OUTPUT_SUN_ZEN;
  dval[0] = (double)meta->sun_zen * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (solar zenith)", "PutMetadata", false);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) OUTPUT_SUN_AZ;
  dval[0] = (double)meta->sun_az * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (solar azimuth)", "PutMetadata", false);

  attr.type = DFNT_CHAR8;
  attr.nval = strlen(meta->wrs_sys);
  attr.name = (char *) OUTPUT_WRS_SYS;
  if (!PutAttrString(ds_output->sds_file_id, &attr, meta->wrs_sys))
    RETURN_ERROR("writing attribute (WRS system)", "PutMetadata", false);

  attr.type = DFNT_INT16;
  attr.nval = 1;
  attr.name = (char *) OUTPUT_WRS_PATH;
  dval[0] = (double)meta->path;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (WRS path)", "PutMetadata", false);

  attr.type = DFNT_INT16;
  attr.nval = 1;
  attr.name = (char *) OUTPUT_WRS_ROW;
  dval[0] = (double)meta->row;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (WRS row)", "PutMetadata", false);

  strcpy (string, "BAPM");
  GenerateShortName(meta->sat, meta->inst, string, short_name);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(short_name);
  attr.name = (char *) OUTPUT_SHORT_NAME;
  if (!PutAttrString(ds_output->sds_file_id, &attr, short_name))
    RETURN_ERROR("writing attribute (short name)", "PutMetadata", false);

  /* Get the current time for the production date, convert it to UTC, and
     format it based on other date formats */
  if (time (&tp) == -1)
    RETURN_ERROR("error obtaining current time", "PutMetadata", false);

  tm = gmtime (&tp);
  if (tm == (struct tm *) NULL)
    RETURN_ERROR("error converting current time to UTC", "PutMetadata", false);

  if (strftime (prod_date, (MAX_DATE_LEN+1), "%Y-%m-%dT%H:%M:%SZ", tm) == 0)
    RETURN_ERROR("error formatting production date and time", "PutMetadata",
        false);

  attr.type = DFNT_CHAR8;
  attr.nval = strlen (prod_date);
  attr.name = (char *) OUTPUT_PROD_DATE;
  if (!PutAttrString (ds_output->sds_file_id, &attr, prod_date))
    RETURN_ERROR("writing attribute (burned area production date)",
      "PutMetadata", false);

  sprintf (process_ver, "%s", BA_VERSION);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(process_ver);
  attr.name = (char *) OUTPUT_BAVERSION;
  if (!PutAttrString (ds_output->sds_file_id, &attr, process_ver))
    RETURN_ERROR("writing attribute (burned area version)", "PutMetadata",
      false);

  /* output the UL and LR corners if they are available */
  if (!meta->ul_corner.is_fill && !meta->lr_corner.is_fill) {
    attr.type = DFNT_FLOAT64;
    attr.nval = 2;
    attr.name = (char *) UL_LAT_LONG;
    dval[0] = meta->ul_corner.lat;
    dval[1] = meta->ul_corner.lon;
    if (!PutAttrDouble (ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (UL lat/long)", "PutMetadata", false);

    attr.type = DFNT_FLOAT64;
    attr.nval = 2;
    attr.name = (char *) LR_LAT_LONG;
    dval[0] = meta->lr_corner.lat;
    dval[1] = meta->lr_corner.lon;
    if (!PutAttrDouble (ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (LR lat/long)", "PutMetadata", false);
  }

  /* output the geographic bounding coordinates if they are available */
  if (!meta->bounds.is_fill) {
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *) OUTPUT_WEST_BOUND;
    dval[0] = meta->bounds.min_lon;
    if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (West Bounding Coords)", "PutMetadata",
        false);

    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *) OUTPUT_EAST_BOUND;
    dval[0] = meta->bounds.max_lon;
    if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (East Bounding Coords)", "PutMetadata",
        false);

    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *) OUTPUT_NORTH_BOUND;
    dval[0] = meta->bounds.max_lat;
    if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (North Bounding Coords)", "PutMetadata",
        false);

    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = (char *) OUTPUT_SOUTH_BOUND;
    dval[0] = meta->bounds.min_lat;
    if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (South Bounding Coords)", "PutMetadata",
        false);
  }

  /* now write out the per sds attributes */
  for (ib = 0; ib < nband; ib++) {
    sprintf (long_name, "%s", sds_names[ib]);
    attr.type = DFNT_CHAR8;
    attr.nval = strlen(long_name);
    attr.name = (char *) OUTPUT_LONG_NAME;
    if (!PutAttrString (ds_output->sds[ib].id, &attr, long_name))
      RETURN_ERROR("writing attribute (long name)", "PutMetadata", false);  

    strcpy (units_b, "percentage");
    attr.nval = strlen(units_b);
    attr.name = (char *) OUTPUT_UNITS;
    if (!PutAttrString(ds_output->sds[ib].id, &attr, units_b))
      RETURN_ERROR("writing attribute (units ref)", "PutMetadata", false);  

    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = (char *) OUTPUT_FILL_VALUE;
    dval[0] = (double) PBA_FILL;
    if (!PutAttrDouble(ds_output->sds[ib].id, &attr, dval))
      RETURN_ERROR("writing attribute (valid range ref)","PutMetadata",false);

    attr.type = DFNT_INT16;
    attr.nval = 2;
    attr.name = (char *) OUTPUT_VALID_RANGE;
    dval[0] = (double) 0.0;
    dval[1] = (double) 100.0;
    if (!PutAttrDouble(ds_output->sds[ib].id, &attr, dval))
      RETURN_ERROR("writing attribute (valid range ref)","PutMetadata",false);

    attr.type = DFNT_CHAR8;
    sprintf (ba_map_QAMAP,
      "\n\tProbability mappings are percentages from 0 to 100.\n"
      "\tValue  Description\n"
      "\t%d\tFill\n"
      "\t%d\tCloud or water", PBA_FILL, PBA_CLOUD_WATER);
    attr.nval = strlen(ba_map_QAMAP);
    attr.name = (char *) "Probability Mapping Values";
    if (!PutAttrString(ds_output->sds[ib].id, &attr, ba_map_QAMAP))
      RETURN_ERROR("writing attribute (probability mapping values)",
        "PutMetadata", false);
  }  /* end for ib */

  return true;
}


/******************************************************************************
MODULE:  GenerateShortName

PURPOSE:  Generates the short name of the current product

RETURN VALUE: None

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
7/11/2013    Gail Schmidt     Original Development (based on input routines
                              from the LEDAPS lndsr application)

NOTES:
******************************************************************************/
void GenerateShortName
(
    char *sat,           /* I: satellite type */
    char *inst,          /* I: instrument type */
    char *product_id,    /* I: ID for the current band */
    char *short_name     /* O: short name produced */
)
{
    /* Create the short name */
    sprintf (short_name, "L%c%c%s", sat[strlen(sat)-1], inst[0], product_id);
}

