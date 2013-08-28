/*****************************************************************************
!File: output.c
  
!Description: Functions creating and writting data to the product output file.

!Revision History:
 Revision 1.0 2012/10/22
 Gail Schmidt
 Original Version - borrowed from some of the LEDAPS libraries.

!Team Unique Header:
  ds_output software was developed by the Landsat Science, Research, and
  Development (LSRD) Team at the USGS EROS.

!Design Notes:
 1. The following public functions handle the output files:
    CreateOutput - Create new output file.
    OutputFile - Setup 'output' data structure.
    CloseOutput - Close the output file.
    FreeOutput - Free the 'output' data structure memory.
    PutMetadata - Write the output product metadata.
    WriteOutput - Write a line of data to the output product file.
*****************************************************************************/

#include "output.h"


#define SDS_PREFIX ("band")

#define OUTPUT_PROVIDER ("DataProvider")
#define OUTPUT_SAT ("Satellite")
#define OUTPUT_INST ("Instrument")
#define OUTPUT_ACQ_DATE ("AcquisitionDate")
#define OUTPUT_L1_PROD_DATE ("Level1ProductionDate")
#define OUTPUT_SUN_ZEN ("SolarZenith")
#define OUTPUT_SUN_AZ ("SolarAzimuth")
#define OUTPUT_WRS_SYS ("WRS_System")
#define OUTPUT_WRS_PATH ("WRS_Path")
#define OUTPUT_WRS_ROW ("WRS_Row")
#define OUTPUT_NBAND ("NumberOfBands")
#define OUTPUT_BANDS ("BandNumbers")
#define OUTPUT_SHORT_NAME ("ShortName")
#define OUTPUT_LOCAL_GRAN_ID ("LocalGranuleID")
#define OUTPUT_PROD_DATE ("ProductionDate")
#define OUTPUT_PGEVERSION ("PGEVersion")
#define OUTPUT_PROCESSVERSION ("ProcessVersion")

#define OUTPUT_WEST_BOUND  ("WestBoundingCoordinate")
#define OUTPUT_EAST_BOUND  ("EastBoundingCoordinate")
#define OUTPUT_NORTH_BOUND ("NorthBoundingCoordinate")
#define OUTPUT_SOUTH_BOUND ("SouthBoundingCoordinate")

#define OUTPUT_LONG_NAME        ("long_name")
#define OUTPUT_UNITS            ("units")
#define OUTPUT_VALID_RANGE      ("valid_range")
#define OUTPUT_FILL_VALUE       ("_FillValue")
#define OUTPUT_SATU_VALUE       ("_SaturateValue")
#define OUTPUT_SCALE_FACTOR     ("scale_factor")
#define OUTPUT_ADD_OFFSET       ("add_offset")
#define OUTPUT_SCALE_FACTOR_ERR ("scale_factor_err")
#define OUTPUT_ADD_OFFSET_ERR   ("add_offset_err")
#define OUTPUT_CALIBRATED_NT    ("calibrated_nt")
#define OUTPUT_QAMAP_INDEX      ("qa_bitmap_index")

/******************************************************************************
!Description: 'CreateOuptut' creates a new output file.
 
!Input Parameters:
 file_name      output file name

!Output Parameters:
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
*****************************************************************************/
bool CreateOutput(char *file_name, char *input_header, char *output_header)
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
!Description: 'OpenOutput' sets up the 'output' data structure, opens the
 output file for write access, and creates the output Science Data Set (SDS).
 
!Input Parameters:ds_output
 file_name      output file name
 nband          number of image bands (SDSs) to be created
 nband_qa       number of QA bands (SDSs) to be created
 sds_names      array of SDS names for each image band
 qa_sds_names   array of SDS names for each QA band
 size           SDS size

!Output Parameters:
 (returns)      'output' data structure or NULL when an error occurs

!Team Unique Header:

!Design Notes:
*****************************************************************************/
Output_t *OpenOutput(char *file_name, int nband, int nband_qa,
   char sds_names[NBAND_REFL_MAX_OUT][MAX_STR_LEN],
   char qa_sds_names[NUM_QA_BAND][MAX_STR_LEN], Img_coord_int_t *size)
{
  Output_t *ds_output = NULL;
  char *error_string = NULL;
  Myhdf_dim_t *dim[MYHDF_MAX_RANK];
  Myhdf_sds_t *sds = NULL;
  int ir, ib;
  int16 *buf = NULL;
  uint8 *qa_buf = NULL;

  /* Check parameters */
  if (size->l < 1)
    RETURN_ERROR("invalid number of output lines", "OpenOutput", NULL);

  if (size->s < 1)
    RETURN_ERROR("invalid number of samples per output line", "OpenOutput",
      NULL);

  if (nband < 1  ||  nband > NBAND_REFL_MAX_OUT)
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
  ds_output->nqa_band = nband_qa;
  ds_output->size.l = size->l;
  ds_output->size.s = size->s;

  for (ib = 0; ib < ds_output->nband; ib++) {
    ds_output->sds[ib].name = NULL;
    ds_output->sds[ib].dim[0].name = NULL;
    ds_output->sds[ib].dim[1].name = NULL;
    ds_output->buf[ib] = NULL;
  }

  for (ib = 0; ib < ds_output->nqa_band; ib++) {
    ds_output->qa_sds[ib].name = NULL;
    ds_output->qa_sds[ib].dim[0].name = NULL;
    ds_output->qa_sds[ib].dim[1].name = NULL;
    ds_output->qa_buf[ib] = NULL;
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
      error_string = (char *) "duplicating dim name (l)";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }
    dim[1]->name = DupString((char *) "XDim_Grid");
    if (dim[1]->name == NULL) {
      error_string = (char *) "duplicating dim name (s)";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }

    if (!PutSDSInfo(ds_output->sds_file_id, sds)) {
      error_string = (char *) "setting up the SDS";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }

    for (ir = 0; ir < sds->rank; ir++) {
      if (!PutSDSDimInfo(sds->id, dim[ir], ir)) {
        error_string = (char *) "setting up the dimension";
        RETURN_ERROR(error_string, "OpenOutput", NULL); 
      }
    }
  }  /* end for image bands */

  /* Set up the QA SDSs */
  for (ib = 0; ib < ds_output->nqa_band; ib++) {
    sds = &ds_output->qa_sds[ib];
    sds->rank = 2;
    sds->type = DFNT_UINT8;
    sds->name = DupString(qa_sds_names[ib]);
    if (sds->name == NULL) {
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
      error_string = (char *) "duplicating dim name (l)";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }
    dim[1]->name = DupString((char *) "XDim_Grid");
    if (dim[1]->name == NULL) {
      error_string = (char *) "duplicating dim name (s)";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }

    if (!PutSDSInfo(ds_output->sds_file_id, sds)) {
      error_string = (char *) "setting up the SDS";
      RETURN_ERROR(error_string, "OpenOutput", NULL); 
    }

    for (ir = 0; ir < sds->rank; ir++) {
      if (!PutSDSDimInfo(sds->id, dim[ir], ir)) {
        error_string = (char *) "setting up the dimension";
        RETURN_ERROR(error_string, "OpenOutput", NULL); 
      }
    }
  }  /* end for QA bands */

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

  qa_buf = (uint8 *)calloc((size_t)(ds_output->size.s * ds_output->nqa_band),
    sizeof(uint8));
  if (qa_buf == NULL)
    error_string = (char *) "allocating output QA buffer";
  else {
    ds_output->qa_buf[0] = qa_buf;
    for (ib = 1; ib < ds_output->nqa_band; ib++)
      ds_output->qa_buf[ib] = ds_output->qa_buf[ib - 1] + ds_output->size.s;
  }

  if (error_string != NULL) {
    FreeOutput (ds_output);
    CloseOutput (ds_output);
    RETURN_ERROR(error_string, "OpenOutput", (Output_t *)NULL); 
  }

  return ds_output;
}


/******************************************************************************
!Description: 'CloseOutput' ends SDS access and closes the output file.
 
!Input Parameters:
 ds_output           'output' data structure

!Output Parameters:
 ds_output           'output' data structure
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
*****************************************************************************/
bool CloseOutput(Output_t *ds_output)
{
  int ib;

  if (!ds_output->open)
    RETURN_ERROR("file not open", "CloseOutput", false);

  /* Close image SDSs */
  for (ib = 0; ib < ds_output->nband; ib++) {
    if (SDendaccess(ds_output->sds[ib].id) == HDF_ERROR)
      RETURN_ERROR("ending sds access", "CloseOutput", false);
  }

  /* Close QA SDSs */
  for (ib = 0; ib < ds_output->nqa_band; ib++) {
    if (SDendaccess(ds_output->qa_sds[ib].id) == HDF_ERROR)
      RETURN_ERROR("ending qa sds access", "CloseOutput", false);
  }

  /* Close the HDF file itself */
  SDend(ds_output->sds_file_id);
  ds_output->open = false;

  return true;
}


/******************************************************************************
!Description: 'FreeOutput' frees the 'output' data structure memory.
 
!Input Parameters:
 ds_output           'output' data structure

!Output Parameters:
 ds_output           'output' data structure
 (returns)      status:
                  'true' = okay (always returned)

!Team Unique Header:

!Design Notes:
*****************************************************************************/
bool FreeOutput(Output_t *ds_output)
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

    /* Free QA band SDSs */
    for (ib = 0; ib < ds_output->nqa_band; ib++) {
      for (ir = 0; ir < ds_output->qa_sds[ib].rank; ir++) {
        if (ds_output->qa_sds[ib].dim[ir].name != NULL)
          free(ds_output->qa_sds[ib].dim[ir].name);
      }
      if (ds_output->qa_sds[ib].name != NULL)
        free(ds_output->qa_sds[ib].name);
    }

    if (ds_output->buf[0] != NULL)
      free(ds_output->buf[0]);
    if (ds_output->qa_buf[0] != NULL)
      free(ds_output->qa_buf[0]);

    if (ds_output->file_name != NULL)
      free(ds_output->file_name);
    free(ds_output);
  }

  return true;
}


/******************************************************************************
!Description: 'PutOutputLine' writes a line of data to the output file.
 
!Input Parameters:
 ds_output           'output' data structure; the following fields are written:
                buf -- contains the line to be written
 iband          current band to be written (0-based)
 iline          current line to be written (0-based)

!Output Parameters:
 ds_output           'output' data structure; the following fields are modified:
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
*****************************************************************************/
bool PredictBurnedArea::PutOutputLine(Output_t *ds_output, int iband, int iline)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];
  void *buf = NULL;

  /* Check the parameters */
  if (ds_output == (Output_t *)NULL)
    RETURN_ERROR("invalid input structure", "PutOutputLine", false);
  if (!ds_output->open)
    RETURN_ERROR("file not open", "PutOutputLine", false);
  if (iband < 0  ||  iband >= ds_output->nband)
    RETURN_ERROR("invalid band number", "PutOutputLine", false);
  if (iline < 0  ||  iline >= ds_output->size.l)
    RETURN_ERROR("invalid line number", "PutOutputLine", false);


  /* Write the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_output->size.s;
  buf = (void *)ds_output->buf[iband];
  if (SDwritedata(ds_output->sds[iband].id, start, NULL, nval, buf) == HDF_ERROR)
    RETURN_ERROR("writing output", "PutOutputLine", false);

  return true;
}

////bool PredictBurnedArea::PutOutputLine(int16 *buf, int iband, int iline)
//{
//  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];
 // void *buf = NULL;

  /* Check the parameters */



  /* Write the data */
//  start[0] = iline;
//  start[1] = 0;
//  nval[0] = 1;
 // nval[1] = 8071;
  //buf = (void *)ds_output->buf[iband];

 // if (SDwritedata(, NULL, nval, (VOIDP)buf) == HDF_ERROR)
 //   RETURN_ERROR("writing output", "PutOutputLine", false);

 // return true;
//}


/******************************************************************************
!Description: 'PutOutputQALine' writes a line of QA data to the output file.
 
!Input Parameters:
 ds_output           'output' data structure; the following fields are written:
                qa_buf -- contains the line to be written
 iband          current band to be written (0-based)
 iline          current line to be written (0-based)

!Output Parameters:
 ds_output           'output' data structure; the following fields are modified:
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
*****************************************************************************/
bool PutOutputQALine(Output_t *ds_output, int iband, int iline)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];
  void *buf = NULL;

  /* Check the parameters */
  if (ds_output == (Output_t *)NULL)
    RETURN_ERROR("invalid input structure", "PutOutputQALine", false);
  if (!ds_output->open)
    RETURN_ERROR("file not open", "PutOutputQALine", false);
  if (iband < 0  ||  iband >= ds_output->nqa_band)
    RETURN_ERROR("invalid band number", "PutOutputQALine", false);
  if (iline < 0  ||  iline >= ds_output->size.l)
    RETURN_ERROR("invalid line number", "PutOutputQALine", false);


  /* Write the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_output->size.s;
  buf = (void *)ds_output->qa_buf[iband];

  if (SDwritedata(ds_output->qa_sds[iband].id, start, NULL, nval, buf) == HDF_ERROR)
    RETURN_ERROR("writing output", "PutOutputQALine", false);
  
  return true;
}


#ifdef GAIL
bool PutMetadata(Output_t *ds_output, int nband, Input_meta_t *meta, Param_t *param,
                 Lut_t *lut, Geo_bounds_t* bounds)
/* 
!C******************************************************************************

!Description: 'PutMetadata' writes metadata to the output file.
 
!Input Parameters:
 ds_output           'output' data structure; the following fields are input:
                   open
 geoloc         'geoloc' data structure; the following fields are input:
                   (none)
 input          'input' data structure;  the following fields are input:
                   (none)

!Output Parameters:
 ds_output           'output' data structure; the following fields are modified:
                   (none)
 (returns)      status:
                  'true' = okay
          'false' = error return

!Team Unique Header:

 ! Design Notes:
   1. ds_output routine is currently a 'stub' and will be implemented later.
   2. An error status is returned when:
       a. the file is not open for access.
   3. Error messages are handled with the 'RETURN_ERROR' macro.
   4. 'OutputFile' must be called before ds_output routine is called.

!END****************************************************************************
*/
{
  Myhdf_attr_t attr;
  char date[MAX_DATE_LEN + 1];
/*double dval[1];     */
  double dval[NBAND_REFL_MAX];
  char *string
     , short_name[250]
     , local_granule_id[250]
     , production_date[MAX_DATE_LEN + 1]
     , pge_ver[100]
     , process_ver[100]
     , long_name[250]
    ;
  int ib;
  const char *qa_band_names[NUM_QA_BAND] = {"fill_QA", "DDV_QA", "cloud_QA",
    "cloud_shadow_QA", "snow_QA", "land_water_QA", "adjacent_cloud_QA"};
  char *QA_on[NBAND_SR_EXTRA] = {"N/A", "fill", "dark dense vegetation",
    "cloud", "cloud shadow", "snow", "water", "adjacent cloud", "N/A", "N/A",
    "N/A"};
  char *QA_off[NBAND_SR_EXTRA] = {"N/A", "not fill", "clear", "clear",
    "clear", "clear", "land", "clear", "N/A", "N/A", "N/A"};
  char* units_b;
  char*  message;
  char lndsr_QAMAP[1000];

  /* Check the parameters */

  if (!ds_output->open)
    RETURN_ERROR("file not open", "PutMetadata", false);

  if (nband < 1  ||  nband > NBAND_REFL_MAX)
    RETURN_ERROR("invalid number of bands", "PutMetadata", false);

  /* Write the metadata */

  attr.id = -1;

  if (meta->provider != PROVIDER_NULL) {
    string = Provider_string[meta->provider].string;
    attr.type = DFNT_CHAR8;
    attr.nval = strlen(string);
    attr.name = OUTPUT_PROVIDER;
    if (!PutAttrString(ds_output->sds_file_id, &attr, string))
      RETURN_ERROR("writing attribute (data provider)", "PutMetadata", false);
  }

  if (meta->sat != SAT_NULL) {
    string = Sat_string[meta->sat].string;
    attr.type = DFNT_CHAR8;
    attr.nval = strlen(string);
    attr.name = OUTPUT_SAT;
    if (!PutAttrString(ds_output->sds_file_id, &attr, string))
      RETURN_ERROR("writing attribute (satellite)", "PutMetadata", false);
  }

  if (meta->inst != INST_NULL) {
    string = Inst_string[meta->inst].string;
    attr.type = DFNT_CHAR8;
    attr.nval = strlen(string);
    attr.name = OUTPUT_INST;
    if (!PutAttrString(ds_output->sds_file_id, &attr, string))
      RETURN_ERROR("writing attribute (instrument)", "PutMetadata", false);
  }

  if (!FormatDate(&meta->acq_date, DATE_FORMAT_DATEA_TIME, date))
    RETURN_ERROR("formating acquisition date", "PutMetadata", false);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(date);
  attr.name = OUTPUT_ACQ_DATE;
  if (!PutAttrString(ds_output->sds_file_id, &attr, date))
    RETURN_ERROR("writing attribute (acquisition date)", "PutMetadata", false);

  if (!FormatDate(&meta->prod_date, DATE_FORMAT_DATEA_TIME, date))
    RETURN_ERROR("formating production date", "PutMetadata", false);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(date);
  attr.name = OUTPUT_L1_PROD_DATE;
  if (!PutAttrString(ds_output->sds_file_id, &attr, date))
    RETURN_ERROR("writing attribute (production date)", "PutMetadata", false);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = OUTPUT_SUN_ZEN;
  dval[0] = (double)meta->sun_zen * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (solar zenith)", "PutMetadata", false);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = OUTPUT_SUN_AZ;
  dval[0] = (double)meta->sun_az * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (solar azimuth)", "PutMetadata", false);

  if (meta->wrs_sys != WRS_NULL) {

    string = Wrs_string[meta->wrs_sys].string;
    attr.type = DFNT_CHAR8;
    attr.nval = strlen(string);
    attr.name = OUTPUT_WRS_SYS;
    if (!PutAttrString(ds_output->sds_file_id, &attr, string))
      RETURN_ERROR("writing attribute (WRS system)", "PutMetadata", false);

    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = OUTPUT_WRS_PATH;
    dval[0] = (double)meta->ipath;
    if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (WRS path)", "PutMetadata", false);

    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = OUTPUT_WRS_ROW;
    dval[0] = (double)meta->irow;
    if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
      RETURN_ERROR("writing attribute (WRS row)", "PutMetadata", false);
  }

  attr.type = DFNT_INT8;
  attr.nval = 1;
  attr.name = OUTPUT_NBAND;
  dval[0] = (double)nband;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (number of bands)", "PutMetadata", false);

  attr.type = DFNT_INT8;
  attr.nval = nband;
  attr.name = OUTPUT_BANDS;
  for (ib = 0; ib < nband; ib++)
    dval[ib] = (double)meta->iband[ib];
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (band numbers)", "PutMetadata", false);

  /* Get the short name, local granule id and production date/time */
  
  if (!Names(meta->sat, meta->inst, "SR", &meta->acq_date, 
             meta->wrs_sys, meta->ipath, meta->irow, 
         short_name, local_granule_id, production_date))
    RETURN_ERROR("creating the short name and local granule id", 
                 "PutMetadata", false);

  attr.type = DFNT_CHAR8;
  attr.nval = strlen(short_name);
  attr.name = OUTPUT_SHORT_NAME;
  if (!PutAttrString(ds_output->sds_file_id, &attr, short_name))
    RETURN_ERROR("writing attribute (short name)", "PutMetadata", false);

  attr.type = DFNT_CHAR8;
  attr.nval = strlen(local_granule_id);
  attr.name = OUTPUT_LOCAL_GRAN_ID;
  if (!PutAttrString(ds_output->sds_file_id, &attr, local_granule_id))
    RETURN_ERROR("writing attribute (local granule id)", "PutMetadata", false);

  attr.type = DFNT_CHAR8;
  attr.nval = strlen(production_date);
  attr.name = OUTPUT_PROD_DATE;
  if (!PutAttrString(ds_output->sds_file_id, &attr, production_date))
    RETURN_ERROR("writing attribute (production date)", "PutMetadata", false);

  if (sprintf(pge_ver, "%s", param->PGEVersion) < 0)
    RETURN_ERROR("creating PGEVersion","PutMetadata", false);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(pge_ver);
  attr.name = OUTPUT_PGEVERSION;
  if (!PutAttrString(ds_output->sds_file_id, &attr, pge_ver))
    RETURN_ERROR("writing attribute (PGE Version)", "PutMetadata", false);

  if (sprintf(process_ver, "%s", param->ProcessVersion) < 0)
    RETURN_ERROR("creating ProcessVersion","PutMetadata", false);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(process_ver);
  attr.name = OUTPUT_PROCESSVERSION;
  if (!PutAttrString(ds_output->sds_file_id, &attr, process_ver))
    RETURN_ERROR("writing attribute (Process Version)", "PutMetadata", false);

  attr.type = DFNT_FLOAT64;
  attr.nval = 1;
  attr.name = OUTPUT_WEST_BOUND;
  dval[0] = bounds->min_lon * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (West Bounding Coords)", "PutMetadata", false);

  attr.type = DFNT_FLOAT64;
  attr.nval = 1;
  attr.name = OUTPUT_EAST_BOUND;
  dval[0] = bounds->max_lon * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (East Bounding Coords)", "PutMetadata", false);

  attr.type = DFNT_FLOAT64;
  attr.nval = 1;
  attr.name = OUTPUT_NORTH_BOUND;
  dval[0] = bounds->max_lat * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (North Bounding Coords)", "PutMetadata", false);

  attr.type = DFNT_FLOAT64;
  attr.nval = 1;
  attr.name = OUTPUT_SOUTH_BOUND;
  dval[0] = bounds->min_lat * DEG;
  if (!PutAttrDouble(ds_output->sds_file_id, &attr, dval))
    RETURN_ERROR("writing attribute (South Bounding Coords)", "PutMetadata", false);

  /* now write out the per sds attributes */

/*for (ib = 0; ib < NBAND_REFL_MAX; ib++) {     */
/*  for (ib = 0; ib < NBAND_SR_MAX; ib++) { EV 9/7/2009 */
  for (ib = 0; ib < NBAND_SR_MAX-3; ib++) {
//printf ("DEBUG: SDS metadata for band %d\n", ib);

  sprintf(long_name, lut->long_name_prefix, meta->iband[ib]); 
  if (ib >= NBAND_REFL_MAX){sprintf(long_name,"%s", DupString(qa_band_names[ib - NBAND_REFL_MAX])); }
//printf ("DEBUG: long name: %s\n", long_name);
  attr.type = DFNT_CHAR8;
  attr.nval = strlen(long_name);
  attr.name = OUTPUT_LONG_NAME;
  if (!PutAttrString(ds_output->sds_sr[ib].id, &attr, long_name))
    RETURN_ERROR("writing attribute (long name)", "PutMetadata", false);

  attr.type = DFNT_CHAR8;
  if (ib <= nband) {  /* reflective bands and atmos_opacity */
    attr.nval = strlen(lut->units);
    attr.name = OUTPUT_UNITS;
    if (!PutAttrString(ds_output->sds_sr[ib].id, &attr, lut->units))
      RETURN_ERROR("writing attribute (units ref)", "PutMetadata", false);
  } else {  /* QA bands */
    units_b=DupString("quality/feature classification");
    attr.nval = strlen(units_b);
    attr.name = OUTPUT_UNITS;
    if (!PutAttrString(ds_output->sds_sr[ib].id, &attr, units_b))
      RETURN_ERROR("writing attribute (units ref)", "PutMetadata", false);  
  }
  attr.type = DFNT_INT16;
  attr.nval = 2;
  attr.name = OUTPUT_VALID_RANGE;
  dval[0] = (double)lut->min_valid_sr;
  dval[1] = (double)lut->max_valid_sr;
  if(ib >= nband+FILL && ib <= nband+ADJ_CLOUD) { /* QA bands */
     dval[0] = (double)(0);
     dval[1] = (double)(255);
  }
  if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
    RETURN_ERROR("writing attribute (valid range ref)","PutMetadata",false);

  if (ib <= nband) {  /* reflective bands and atmos_opacity */
    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = OUTPUT_FILL_VALUE;
    dval[0] = (double)lut->output_fill;
    if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
      RETURN_ERROR("writing attribute (valid range ref)","PutMetadata",false);

    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = OUTPUT_SATU_VALUE;
    dval[0] = (double)lut->out_satu;
    if (ib != nband+ATMOS_OPACITY) /* doesn't apply for atmos opacity */
      if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
        RETURN_ERROR("writing attribute (saturate value ref)","PutMetadata",false);

    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = OUTPUT_SCALE_FACTOR;
    dval[0] = (double)lut->scale_factor;
    if (ib == nband+ATMOS_OPACITY)
      dval[0] = (double) 0.001;
    if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
      RETURN_ERROR("writing attribute (scale factor ref)", "PutMetadata", false);
  
    if (ib != nband+ATMOS_OPACITY) { /* don't apply for atmos opacity */
      attr.type = DFNT_FLOAT64;
      attr.nval = 1;
      attr.name = OUTPUT_ADD_OFFSET;
      dval[0] = (double)lut->add_offset;
      if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
        RETURN_ERROR("writing attribute (add offset ref)", "PutMetadata", false);
  
      attr.type = DFNT_FLOAT64;
      attr.nval = 1;
      attr.name = OUTPUT_SCALE_FACTOR_ERR;
      dval[0] = (double)lut->scale_factor_err;
      if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
        RETURN_ERROR("writing attribute (scale factor err ref)", "PutMetadata", false);
  
      attr.type = DFNT_FLOAT64;
      attr.nval = 1;
      attr.name = OUTPUT_ADD_OFFSET_ERR;
      dval[0] = (double)lut->add_offset_err;
      if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
        RETURN_ERROR("writing attribute (add offset err ref)", "PutMetadata", false);
  
      attr.type = DFNT_FLOAT32;
      attr.nval = 1;
      attr.name = OUTPUT_CALIBRATED_NT;
      dval[0] = (double)lut->calibrated_nt;
      if (!PutAttrDouble(ds_output->sds_sr[ib].id, &attr, dval))
        RETURN_ERROR("writing attribute (calibrated nt ref)","PutMetadata",false);
    } /* end if not atmos opacity */
  } /* end if no QA bands */

  if (ib >= nband+FILL && ib <= nband+ADJ_CLOUD) {
//printf ("DEBUG: Writing lndsr_QAMAP for band %d\n", ib);
    attr.type = DFNT_CHAR8;
    sprintf (lndsr_QAMAP,
      "\n\tQA pixel values are either off or on:\n"
      "\tValue  Description\n"
      "\t0\t%s\n"
      "\t255\t%s", QA_off[ib-nband], QA_on[ib-nband]);
    message=DupString(lndsr_QAMAP);
//printf ("DEBUG: %s\n", lndsr_QAMAP);
    attr.nval = strlen(message);
    attr.name = "QA index";
    if (!PutAttrString(ds_output->sds_sr[ib].id, &attr, message))
      RETURN_ERROR("writing attribute (QA index)", "PutMetadata", false);
   }
  }  /* end for ib */

  return true;
}
#endif
