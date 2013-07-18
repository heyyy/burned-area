/*
 * input.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: jlriegle
 */

#include "date.h"
#include "input.h"
#include <opencv/cv.h>
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"
#include "PredictBurnedArea.h"
#include <iostream>
#include <stdint.h>

using namespace std;


//#define SDS_PREFIX ("band")
//#define INPUT_PROVIDER ("DataProvider")
//#define INPUT_SAT ("Satellite")
//#define INPUT_INST ("Instrument")
//#define INPUT_ACQ_DATE ("AcquisitionDate")
//#define INPUT_PROD_DATE ("Level1ProductionDate")
//#define INPUT_SUN_ZEN ("SolarZenith")
//#define INPUT_SUN_AZ ("SolarAzimuth")
//#define INPUT_WRS_SYS ("WRS_System")
//#define INPUT_WRS_PATH ("WRS_Path")
//#define INPUT_WRS_ROW ("WRS_Row")
//#define INPUT_WBC ("WestBoundingCoordinate")
//#define INPUT_EBC ("EastBoundingCoordinate")
//#define INPUT_NBC ("NorthBoundingCoordinate")
//#define INPUT_SBC ("SouthBoundingCoordinate")
//#define INPUT_NBAND ("NumberOfBands")
//#define INPUT_BANDS ("BandNumbers")
//#define INPUT_FILL_VALUE ("_FillValue")

const char *SDS_PREFIX = "band";
const char *INPUT_PROVIDER = "DataProvider";
const char *INPUT_SAT = "Satellite";
const char *INPUT_INST = "Instrument";
const char *INPUT_ACQ_DATE = "AcquisitionDate";
const char *INPUT_PROD_DATE = "Level1ProductionDate";
const char *INPUT_SUN_ZEN = "SolarZenith";
const char *INPUT_SUN_AZ = "SolarAzimuth";
const char *INPUT_WRS_SYS = "WRS_System";
const char *INPUT_WRS_PATH = "WRS_Path";
const char *INPUT_WRS_ROW = "WRS_Row";
const char *INPUT_WBC = "WestBoundingCoordinate";
const char *INPUT_EBC = "EastBoundingCoordinate";
const char *INPUT_NBC = "NorthBoundingCoordinate";
const char *INPUT_SBC = "SouthBoundingCoordinate";
const char *INPUT_NBAND = "NumberOfBands";
const char *INPUT_BANDS = "BandNumbers";
const char *INPUT_FILL_VALUE = "_FillValue";

#define N_LSAT_WRS1_ROWS  (251)
#define N_LSAT_WRS1_PATHS (233)
#define N_LSAT_WRS2_ROWS  (248)
#define N_LSAT_WRS2_PATHS (233)

/* Band names for the QA bands */
const char *qa_band_names[NUM_QA_BAND] = {"fill_QA", "DDV_QA", "cloud_QA",
  "cloud_shadow_QA", "snow_QA", "land_water_QA", "adjacent_cloud_QA"};


/******************************************************************************
!Description: 'OpenInput' sets up the 'input' data structure, opens the
 input file for read access, allocates space, and stores some of the metadata.

!Input Parameters:
 file_name      input file name
cd
!Output Parameters:
 (returns)      populated 'input' data structure or NULL when an error occurs

!Team Unique Header:

!Design Notes:
******************************************************************************/
Input_t *OpenInput(char *file_name)
{
// Input_t *input;
  Myhdf_attr_t attr;
  const char *error_string = NULL;
  char sds_name[40];
  int ir;
  Myhdf_dim_t *dim[2];
  int ib;
  double dval[NBAND_REFL_MAX];
  int16 *buf = NULL;
  uint8 *qa_buf = NULL;

  /* Create the Input data structure */
//  ds_input = (Input_t *)malloc(sizeof(Input_t));
  Input_t* ds_input = new Input_t();
  if (ds_input == NULL)
    RETURN_ERROR("allocating Input data structure", "OpenInput", NULL);

  /* Populate the data structure */
  ds_input->file_name = DupString(file_name);
  if (ds_input->file_name == NULL) {
   // free(input);
	  delete ds_input;
    RETURN_ERROR("duplicating file name", "OpenInput", NULL);
  }

  /* Open file for SD access */
  ds_input->sds_file_id = SDstart((char *)file_name, DFACC_RDONLY);
  if (ds_input->sds_file_id == HDF_ERROR) {
	  delete ds_input;
  //  free(input->file_name);
  //  free(input);
    RETURN_ERROR("opening input file", "OpenInput", NULL);
  }
  ds_input->open = true;

  /* Get the input metadata */
  if (!GetInputMeta(ds_input)) {
	  delete ds_input->file_name;
	  delete ds_input;
   // free(ds_input->file_name);
    //free(ds_input);
    RETURN_ERROR("getting input metadata", "OpenInput", NULL);
  }

  /* Get SDS information and start SDS access */
  for (ib = 0; ib < ds_input->nband; ib++) {  /* image data */
    ds_input->sds[ib].name = NULL;
    ds_input->sds[ib].dim[0].name = NULL;
    ds_input->sds[ib].dim[1].name = NULL;
    ds_input->buf[ib] = NULL;
  }

  for (ib = 0; ib < ds_input->nqa_band; ib++) {  /* QA data */
    ds_input->qa_sds[ib].name = NULL;
    ds_input->qa_sds[ib].dim[0].name = NULL;
    ds_input->qa_sds[ib].dim[1].name = NULL;
    ds_input->qa_buf[ib] = NULL;
  }

  /* thermal data */
  ds_input->therm_sds.name = NULL;
  ds_input->therm_sds.dim[0].name = NULL;
  ds_input->therm_sds.dim[1].name = NULL;
  ds_input->therm_buf = NULL;

  /* Loop through the image bands and obtain the SDS information */
  for (ib = 0; ib < ds_input->nband; ib++) {
    if (sprintf(sds_name, "%s%d", SDS_PREFIX, ds_input->meta.band[ib]) < 0) {
      error_string = "creating SDS name";
      break;
    }

    ds_input->sds[ib].name = DupString(sds_name);
    if (ds_input->sds[ib].name == NULL) {
      error_string = "setting SDS name";
      break;
    }

    if (!GetSDSInfo(ds_input->sds_file_id, &ds_input->sds[ib])) {
      error_string = "getting sds info";
      break;
    }

    /* Check rank */
    if (ds_input->sds[ib].rank != 2) {
      error_string = "invalid rank";
      break;
    }


    /* Check SDS type */
    if (ds_input->sds[ib].type != DFNT_INT16) {
      error_string = "invalid number type";
      break;
    }

    /* Get dimensions */
    for (ir = 0; ir < ds_input->sds[ib].rank; ir++) {
      dim[ir] = &ds_input->sds[ib].dim[ir];
      if (!GetSDSDimInfo(ds_input->sds[ib].id, dim[ir], ir)) {
        error_string = "getting dimensions";
        break;
      }
    }

    if (error_string != NULL) break;

    /* Save and check line and sample dimensions */
    if (ib == 0) {
      ds_input->size.l = dim[0]->nval;
      ds_input->size.s = dim[1]->nval;
    } else {
      if (ds_input->size.l != dim[0]->nval) {
        error_string = "all line dimensions do not match";
        break;
      }
      if (ds_input->size.s != dim[1]->nval) {
        error_string = "all sample dimensions do not match";
        break;
      }
    }

    /* If ds_input is the first image band, read the fill value */
    attr.type = DFNT_FLOAT32;
    attr.nval = 1;
    attr.name = (char *) INPUT_FILL_VALUE;
    if (!GetAttrDouble(ds_input->sds[ib].id, &attr, dval))
      RETURN_ERROR("reading band SDS attribute (fill value)", "OpenInput",
        NULL);
    if (attr.nval != 1)
      RETURN_ERROR("invalid number of values (fill value)", "OpenInput", NULL);
    ds_input->meta.fill = dval[0];
  }  /* for ib */

  /* Loop through the QA bands and obtain the SDS information */
  for (ib = 0; ib < ds_input->nqa_band; ib++) {
    strcpy (sds_name, qa_band_names[ib]);
    ds_input->qa_sds[ib].name = DupString(sds_name);

    if (ds_input->qa_sds[ib].name == NULL) {
      error_string = "setting QA SDS name";
      break;
    }

    if (!GetSDSInfo(ds_input->sds_file_id, &ds_input->qa_sds[ib])) {
      error_string = "getting QA sds info";
      break;
    }

    /* Check rank */
    if (ds_input->qa_sds[ib].rank != 2) {
      error_string = "invalid QA rank";
      break;
    }

    /* Check SDS type */
    if (ds_input->qa_sds[ib].type != DFNT_UINT8) {
      error_string = "invalid QA number type";
      break;
    }

    /* Get dimensions */
    for (ir = 0; ir < ds_input->qa_sds[ib].rank; ir++) {
      dim[ir] = &ds_input->qa_sds[ib].dim[ir];
      if (!GetSDSDimInfo(ds_input->qa_sds[ib].id, dim[ir], ir)) {
        error_string = "getting QA dimensions";
        break;
      }
    }

    if (error_string != NULL) break;

    /* Save and check line and sample dimensions */
    if (ib == 0) {
      ds_input->size.l = dim[0]->nval;
      ds_input->size.s = dim[1]->nval;
    } else {
      if (ds_input->size.l != dim[0]->nval) {
        error_string = "all line dimensions do not match";
        break;
      }
      if (ds_input->size.s != dim[1]->nval) {
        error_string = "all sample dimensions do not match";
        break;
      }
    }
  }  /* for ib */

  /* For the single thermal band, obtain the SDS information */
  strcpy (sds_name, "band6");
  ds_input->therm_sds.name = DupString(sds_name);

  if (ds_input->therm_sds.name == NULL)
    error_string = "setting thermal SDS name";

  if (!GetSDSInfo(ds_input->sds_file_id, &ds_input->therm_sds))
    error_string = "getting thermal sds info";

  /* Check rank */
  if (ds_input->therm_sds.rank != 2)
    error_string = "invalid thermal rank";

  /* Check SDS type */
  if (ds_input->therm_sds.type != DFNT_INT16)
    error_string = "invalid thermal number type";

  /* Get dimensions */
  for (ir = 0; ir < ds_input->therm_sds.rank; ir++) {
    dim[ir] = &ds_input->therm_sds.dim[ir];
    if (!GetSDSDimInfo(ds_input->therm_sds.id, dim[ir], ir))
      error_string = "getting thermal dimensions";
  }

  /* Allocate input buffers.  Thermal band only has one band.  Image and QA
     buffers have multiple bands. */
  buf = (int16 *)calloc((size_t)(ds_input->size.s * ds_input->nband), sizeof(int16));
  if (buf == NULL)
    error_string = "allocating input buffer";
  else {
    ds_input->buf[0] = buf;
    for (ib = 1; ib < ds_input->nband; ib++)
      ds_input->buf[ib] = ds_input->buf[ib - 1] + ds_input->size.s;
  }

  qa_buf = (uint8 *)calloc((size_t)(ds_input->size.s * ds_input->nqa_band),
    sizeof(uint8));
  if (qa_buf == NULL)
    error_string = "allocating input QA buffer";
  else {
    ds_input->qa_buf[0] = qa_buf;
    for (ib = 1; ib < ds_input->nqa_band; ib++)
      ds_input->qa_buf[ib] = ds_input->qa_buf[ib - 1] + ds_input->size.s;
  }

  ds_input->therm_buf = (int16 *)calloc((size_t)(ds_input->size.s), sizeof(int16));
  if (ds_input->therm_buf == NULL)
    error_string = "allocating input thermal buffer";

  if (error_string != NULL) {
    FreeInput (ds_input);
    CloseInput (ds_input);
    RETURN_ERROR(error_string, "OpenInput", NULL);
  }

  return ds_input;
}


/******************************************************************************
!Description: 'CloseInput' ends SDS access and closes the input file.

!Input Parameters:
 ds_input           'input' data structure

!Output Parameters:
 ds_input           'input' data structure; the following fields are modified:
                   open
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
******************************************************************************/
bool CloseInput(Input_t *ds_input)
{
  int ib;

  if (!ds_input->open)
    RETURN_ERROR("file not open", "CloseInput", false);

  /* Close image SDSs */
  for (ib = 0; ib < ds_input->nband; ib++) {
    if (SDendaccess(ds_input->sds[ib].id) == HDF_ERROR)
      RETURN_ERROR("ending sds access", "CloseInput", false);
  }

  /* Close QA SDSs */
  for (ib = 0; ib < ds_input->nqa_band; ib++) {
    if (SDendaccess(ds_input->qa_sds[ib].id) == HDF_ERROR)
      RETURN_ERROR("ending qa sds access", "CloseInput", false);
  }

  /* Close thermal SDS */
  if (SDendaccess(ds_input->therm_sds.id) == HDF_ERROR)
    RETURN_ERROR("ending thermal sds access", "CloseInput", false);

  /* Close the HDF file itself */
  SDend(ds_input->sds_file_id);
  ds_input->open = false;

  return true;
}


/******************************************************************************
!Description: 'FreeInput' frees the 'input' data structure memory.
!Input Parameters:
 ds_input           'input' data structure

!Output Parameters:
 (returns)      status:
                  'true' = okay (always returned)

!Team Unique Header:

!Design Notes:
******************************************************************************/
bool FreeInput(Input_t *ds_input)
{
  int ib, ir;

  if (ds_input->open)
    RETURN_ERROR("file still open", "FreeInput", false);

  if (ds_input != NULL) {
    /* Free image band SDSs */
    for (ib = 0; ib < ds_input->nband; ib++) {
      for (ir = 0; ir < ds_input->sds[ib].rank; ir++) {
        if (ds_input->sds[ib].dim[ir].name != NULL)
          //free(ds_input->sds[ib].dim[ir].name);
        	delete ds_input->sds[ib].dim[ir].name;
      }
      if (ds_input->sds[ib].name != NULL)
        //free(ds_input->sds[ib].name);
    	  delete ds_input->sds[ib].name;
    }

    /* Free QA band SDSs */
    for (ib = 0; ib < ds_input->nqa_band; ib++) {
      for (ir = 0; ir < ds_input->qa_sds[ib].rank; ir++) {
        if (ds_input->qa_sds[ib].dim[ir].name != NULL)
          //free(ds_input->qa_sds[ib].dim[ir].name);
        	delete ds_input->qa_sds[ib].dim[ir].name;
      }
      if (ds_input->qa_sds[ib].name != NULL)
        //free(ds_input->qa_sds[ib].name);
    	  delete ds_input->qa_sds[ib].name;
    }

    /* Free thermal band SDS */
    for (ir = 0; ir < ds_input->therm_sds.rank; ir++) {
      if (ds_input->therm_sds.dim[ir].name != NULL)
        //free(ds_input->therm_sds.dim[ir].name);
      delete ds_input->therm_sds.dim[ir].name;
    }
    if (ds_input->therm_sds.name != NULL)
     // free(ds_input->therm_sds.name);
    	delete ds_input->therm_sds.name;

    /* Free the data buffers */
    if (ds_input->buf[0] != NULL)
      free(ds_input->buf[0]);
    if (ds_input->qa_buf[0] != NULL)
      free(ds_input->qa_buf[0]);
    if (ds_input->therm_buf != NULL)
      free(ds_input->therm_buf);

    if (ds_input->file_name != NULL)
      free(ds_input->file_name);
    free(ds_input);
  } /* end if */

  return true;
}


/******************************************************************************
!Description: 'GetInputLine' reads the surface reflectance data for the current
   band

!Input Parameters:
 ds_input           'input' data structure
 iband          current band to be read (0-based)

!Output Parameters:
predMat			cv::Mat that holds the data.

 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
******************************************************************************/

bool PredictBurnedArea::GetInputData(Input_t *ds_input, int iband, int iline)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];

  int16 *data;

  //data = new int16[arrSize];
  data = new int16[ds_input->size.s];

  /* Check the parameters */
  if (ds_input == (Input_t *)NULL)
    RETURN_ERROR("invalid input structure", "GetIntputLine", false);
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputLine", false);
  if (iband < 0 || iband >= ds_input->nband)
    RETURN_ERROR("invalid band number", "GetInputLine", false);

  /* Read the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_input->size.s;


  if (SDreaddata(ds_input->sds[iband].id, start, NULL, nval, (VOIDP)data) == HDF_ERROR)
    RETURN_ERROR("reading input", "GetInputLine", false)


    //Grabbing bands 1-5 & 7 and putting value into predMat
    for (int i=0; i<ds_input->size.s; i++) {
	 predMat.at<float>(i,iband) = data[i];
    	//predMat.at<int>(i,iband) = data[i];

  }

  return true;
}

/******************************************************************************
!Description: 'calcBands' reads the metadata for the image and puts it into the cv::Mat object

!Input Parameters:
 arrSz          number of rows
 ds_input		metadata from Landsat image

!Output Parameters:
predMat			cv::Mat that holds the data.

 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
******************************************************************************/

//bool PredictBurnedArea::calcBands(int32 arrSz, Input_t *ds_input) {
bool PredictBurnedArea::calcBands(Input_t *ds_input) {

	for (int i = 0; i < ds_input->size.s; i++) {

		if (predMat.at<float>(i,3) + predMat.at<float>(i,2) == 0) {
			predMat.at<float>(i,6) = 0; //avoid division by 0
		} else {
			predMat.at<float>(i,6) = ((predMat.at<float>(i,3) - predMat.at<float>(i,2))/(predMat.at<float>(i,3) + predMat.at<float>(i,2))) * 1000; //NDVI - using bands 4 and 3
		}

		if (predMat.at<float>(i,3) + predMat.at<float>(i,4) == 0) {
			predMat.at<float>(i,7) = 0; //avoid division by 0
		} else {
			predMat.at<float>(i,7) = ((predMat.at<float>(i,3) - predMat.at<float>(i,4))/(predMat.at<float>(i,3) + predMat.at<float>(i,4))) * 1000; //NDMI - using bands 4 and 5
		}

		if (predMat.at<float>(i,3) + predMat.at<float>(i,5) == 0) {
			predMat.at<float>(i,8) = 0; //avoid division by 0
		} else {
			predMat.at<float>(i,8) = ((predMat.at<float>(i,3) - predMat.at<float>(i,5))/(predMat.at<float>(i,3) + predMat.at<float>(i,5))) * 1000; //NBR - using bands 4 and 7
		}

		if (predMat.at<float>(i,4) + predMat.at<float>(i,5) == 0) {
			predMat.at<float>(i,9) = 0; //avoid division by 0
		} else {
			predMat.at<float>(i,9) = ((predMat.at<float>(i,4) - predMat.at<float>(i,5))/(predMat.at<float>(i,4) + predMat.at<float>(i,5))) * 1000; //NBR2 - using bands 5 and 7
		}

	}


	return true;
}


/******************************************************************************
!Description: 'GetInputQALine' reads the QA data for the current QA band and
    line

!Input Parameters:
 ds_input           'input' data structure
 iband          current QA band to be read (0-based)
 iline          current line to be read (0-based)

!Output Parameters:
 ds_input           'input' data structure; the following fields are modified:
                   qa_buf -- contains the line read
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
******************************************************************************/
bool PredictBurnedArea::GetInputQALine(Input_t *ds_input, int iband, int iline)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];

  uint8 *data;
  data = new uint8[ds_input->size.s];
  /* Check the parameters */
  if (ds_input == (Input_t *)NULL)
    RETURN_ERROR("invalid input structure", "GetIntputQALine", false);
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputQALine", false);
  if (iband < 0 || iband >= NUM_QA_BAND)
    RETURN_ERROR("invalid band number", "GetInputQALine", false);
  if (iline < 0 || iline >= ds_input->size.l)
    RETURN_ERROR("invalid line number", "GetInputQALine", false);

  /* Read the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_input->size.s;

  if (SDreaddata(ds_input->qa_sds[iband].id, start, NULL, nval, (VOIDP)data) == HDF_ERROR)
    RETURN_ERROR("reading input", "GetInputQALine", false);

  if (strcmp(ds_input->qa_sds[iband].name, "cloud_QA") ==0) {
	  for (int i=0; i<ds_input->size.s; i++) {
	  	 cloudMat.at<int>(i,0) = data[i];
	   }
  }

  if (strcmp(ds_input->qa_sds[iband].name, "cloud_shadow_QA") ==0) {
  	  for (int i=0; i<ds_input->size.s; i++) {
  	  	 cloudShadMat.at<int>(i,0) = data[i];
  	   }
    }

  if (strcmp(ds_input->qa_sds[iband].name, "land_water_QA") ==0) {
    	  for (int i=0; i<ds_input->size.s; i++) {
    	  	 landWaterMat.at<int>(i,0) = data[i];
    	   }
      }
  return true;
}


/******************************************************************************
!Description: 'GetInputThermLine' reads the thermal brightness data for the
current line

!Input Parameters:
 ds_input           'input' data structure
 iline          current line to be read (0-based)

!Output Parameters:
 ds_input           'input' data structure; the following fields are modified:
                   therm_buf -- contains the line read
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
******************************************************************************/
bool GetInputThermLine(Input_t *ds_input, int iline)
{
  int32 start[MYHDF_MAX_RANK], nval[MYHDF_MAX_RANK];
  void *buf = NULL;

  /* Check the parameters */
  if (ds_input == (Input_t *)NULL)
    RETURN_ERROR("invalid input structure", "GetIntputThermLine", false);
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputThermLine", false);
  if (iline < 0 || iline >= ds_input->size.l)
    RETURN_ERROR("invalid line number", "GetInputThermLine", false);

  /* Read the data */
  start[0] = iline;
  start[1] = 0;
  nval[0] = 1;
  nval[1] = ds_input->size.s;
  buf = (void *)ds_input->therm_buf;

  if (SDreaddata(ds_input->therm_sds.id, start, NULL, nval, buf) == HDF_ERROR)
    RETURN_ERROR("reading input", "GetInputThermLine", false);

  return true;
}


/******************************************************************************
!Description: 'GetInputMeta' reads the metadata for input HDF file

!Input Parameters:
 ds_input           'input' data structure

!Output Parameters:
 ds_input           'input' data structure; metadata fields are populated
 (returns)      status:
                  'true' = okay
                  'false' = error return

!Team Unique Header:

!Design Notes:
******************************************************************************/
bool GetInputMeta(Input_t *ds_input)
{
  Myhdf_attr_t attr;
  double dval[NBAND_REFL_MAX];
  char date[MAX_DATE_LEN + 1];
  int ib;
  Input_meta_t *meta = NULL;
  char *error_string = NULL;

  /* Check the parameters */
  if (!ds_input->open)
    RETURN_ERROR("file not open", "GetInputMeta", false);

  meta = &ds_input->meta;

  /* Read the metadata */
  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_PROVIDER;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.provider))
    RETURN_ERROR("reading attribute (data provider)", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_SAT;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.sat))
    RETURN_ERROR("reading attribute (instrument)", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_INST;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.inst))
    RETURN_ERROR("reading attribute (instrument)", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_DATE_LEN;
  attr.name = (char *) INPUT_ACQ_DATE;
  if (!GetAttrString(ds_input->sds_file_id, &attr, date))
    RETURN_ERROR("reading attribute (acquisition date)", "GetInputMeta", false);
  if (!DateInit(&meta->acq_date, date, DATE_FORMAT_DATEA_TIME))
    RETURN_ERROR("converting acquisition date", "GetInputMeta", false);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_DATE_LEN;
  attr.name = (char *) INPUT_PROD_DATE;
  if (!GetAttrString(ds_input->sds_file_id, &attr, date))
    RETURN_ERROR("reading attribute (production date)", "GetInputMeta", false);
  if (!DateInit(&meta->prod_date, date, DATE_FORMAT_DATEA_TIME))
    RETURN_ERROR("converting production date", "GetInputMeta", false);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_SUN_ZEN;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (solar zenith)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (solar zenith)",
                  "GetInputMeta", false);
  if (dval[0] < -90.0  ||  dval[0] > 90.0)
    RETURN_ERROR("solar zenith angle out of range", "GetInputMeta", false);
  meta->sun_zen = (float)(dval[0] * RAD);

  attr.type = DFNT_FLOAT32;
  attr.nval = 1;
  attr.name = (char *) INPUT_SUN_AZ;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (solar azimuth)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (solar azimuth)",
                 "GetInputMeta", false);
  if (dval[0] < -360.0  ||  dval[0] > 360.0)
    RETURN_ERROR("solar azimuth angle out of range", "GetInputMeta", false);
  meta->sun_az = (float)(dval[0] * RAD);

  attr.type = DFNT_CHAR8;
  attr.nval = MAX_STR_LEN;
  attr.name = (char *) INPUT_WRS_SYS;
  if (!GetAttrString(ds_input->sds_file_id, &attr, ds_input->meta.wrs_sys))
    RETURN_ERROR("reading attribute (WRS system)", "GetInputMeta", false);

  attr.type = DFNT_INT16;
  attr.nval = 1;
  attr.name = (char *) INPUT_WRS_PATH;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (WRS path)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (WRS path)", "GetInputMeta", false);
  meta->path = (int)floor(dval[0] + 0.5);
  if (meta->path < 1)
    RETURN_ERROR("WRS path out of range", "GetInputMeta", false);

  attr.type = DFNT_INT16;
  attr.nval = 1;
  attr.name = (char *) INPUT_WRS_ROW;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (WRS row)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (WRS row)", "GetInputMeta", false);
  meta->row = (int)floor(dval[0] + 0.5);
  if (meta->row < 1)
    RETURN_ERROR("WRS path out of range", "GetInputMeta", false);

  attr.type = DFNT_INT8;
  attr.nval = 1;
  attr.name = (char *) INPUT_NBAND;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (number of bands)", "GetInputMeta", false);
  if (attr.nval != 1)
    RETURN_ERROR("invalid number of values (number of bands)",
                 "GetInputMeta", false);
  ds_input->nband = (int)floor(dval[0] + 0.5);
  if (ds_input->nband < 1  ||  ds_input->nband > NBAND_REFL_MAX)
    RETURN_ERROR("number of bands out of range", "GetInputMeta", false);

  attr.type = DFNT_INT8;
  attr.nval = ds_input->nband;
  attr.name = (char *) INPUT_BANDS;
  if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
    RETURN_ERROR("reading attribute (band numbers)", "GetInputMeta", false);
  if (attr.nval != ds_input->nband)
    RETURN_ERROR("invalid number of values (band numbers)",
                 "GetInputMeta", false);
  for (ib = 0; ib < ds_input->nband; ib++) {
    meta->band[ib] = (int)floor(dval[ib] + 0.5);
    if (meta->band[ib] < 1)
      RETURN_ERROR("band number out of range", "GetInputMeta", false);
  }

   attr.type = DFNT_FLOAT32;
   attr.nval = 1;
   attr.name = (char *) INPUT_WBC;
   if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
     RETURN_ERROR("reading attribute (west bound coord)", "GetInputMeta", false);
   if (attr.nval != 1)
     RETURN_ERROR("invalid number of values (west bound coord)",
                  "GetInputMeta", false);
   if (dval[0] < -180.0  ||  dval[0] > 180.0)
     RETURN_ERROR("west bound coord out of range", "GetInputMeta", false);
   meta->wbc = dval[0];

	attr.type = DFNT_FLOAT32;
	attr.nval = 1;
	attr.name = (char *) INPUT_EBC;
	if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
	RETURN_ERROR("reading attribute (east bound coord)", "GetInputMeta", false);
	if (attr.nval != 1)
	RETURN_ERROR("invalid number of values (east bound coord)",
				 "GetInputMeta", false);
	if (dval[0] < -180.0  ||  dval[0] > 180.0)
	RETURN_ERROR("east bound coord out of range", "GetInputMeta", false);
	meta->ebc = dval[0];

	attr.type = DFNT_FLOAT32;
	attr.nval = 1;
	attr.name = (char *) INPUT_NBC;
	if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
	RETURN_ERROR("reading attribute (north bound coord)", "GetInputMeta", false);
	if (attr.nval != 1)
	RETURN_ERROR("invalid number of values (north bound coord)",
				"GetInputMeta", false);
	if (dval[0] < -90.0  ||  dval[0] > 90.0)
	RETURN_ERROR("north bound coord out of range", "GetInputMeta", false);
	meta->nbc = dval[0];

	attr.type = DFNT_FLOAT32;
	attr.nval = 1;
	attr.name = (char *) INPUT_SBC;
	if (!GetAttrDouble(ds_input->sds_file_id, &attr, dval))
	  RETURN_ERROR("reading attribute (south bound coord)", "GetInputMeta", false);
	if (attr.nval != 1)
	  RETURN_ERROR("invalid number of values (south bound coord)",
				   "GetInputMeta", false);
	if (dval[0] < -90.0  ||  dval[0] > 90.0)
	  RETURN_ERROR("south bound coord out of range", "GetInputMeta", false);
	meta->sbc = dval[0];

  /* Set the number of QA bands */
  ds_input->nqa_band = NUM_QA_BAND;

  /* Check WRS path/rows */
  error_string = (char *)NULL;

  if (!strcmp (meta->wrs_sys, "1")) {
    if (meta->path > N_LSAT_WRS1_PATHS)
      error_string = (char *) "WRS path number out of range";
    else if (meta->row > N_LSAT_WRS1_ROWS)
      error_string = (char *) "WRS row number out of range";
  } else if (!strcmp (meta->wrs_sys, "2")) {
    if (meta->path > N_LSAT_WRS2_PATHS)
      error_string = (char *) "WRS path number out of range";
    else if (meta->row > N_LSAT_WRS2_ROWS)
      error_string = (char *) "WRS row number out of range";
  } else
    error_string = (char *) "invalid WRS system";

  if (error_string != (char *)NULL)
    RETURN_ERROR(error_string, "GetInputMeta", false);

  return true;
}




