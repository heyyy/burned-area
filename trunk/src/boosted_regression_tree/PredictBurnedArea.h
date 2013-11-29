/*****************************************************************************
FILE: PredictBurnedArea.h
  
PURPOSE: Contains the PredictBurnedArea class variables, methods, definitions,
and structs.

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

#ifndef PredictBurnedArea_H_
#define PredictBurnedArea_H_

/* NOTE: make sure to include the myhdf.h file before the xtiffio.h file,
   due to the fact that myhdf.h defines the HAVE_INT8 to prevent conflicts
   in the definition of int8 between HDF and TIFF. */
#include <iostream>
#include <fstream>
#include <stdint.h>
#include "cv.h"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"
#include "const.h"
#include "date.h"
#include "error.h"
#include "mystring.h"
#include "space.h"
#include "myhdf.h"
#include "xtiffio.h"
#include "geotiffio.h"

using namespace std;

#define BA_VERSION "1.0.0"

/* Type definitions */
typedef enum {FAILURE=0, SUCCESS=1} Status_t;
typedef enum {WINTER=0, SPRING, SUMMER, FALL, PBA_NSEASONS} Season_t;
typedef enum {B3=0, B4, B5, B7, BND_NDVI, BND_NDMI, BND_NBR, BND_NBR2,
    PBA_NBANDS} BandIndex_t;
typedef enum {NDVI=0, NDMI, NBR, NBR2, PBA_NINDXS} Index_t;
typedef enum {PREDMAT_B1=0, PREDMAT_B2, PREDMAT_B3, PREDMAT_B4, PREDMAT_B5,
    PREDMAT_B7, PREDMAT_NDVI, PREDMAT_NDMI, PREDMAT_NBR, PREDMAT_NBR2,
    PBA_NPREDMAT} Predmat_t;

/* There are currently a maximum of 6 reflective bands in the surface
   reflectance product (1, 2, 3, 4, 5, 7) */
#define NUM_REFL_BAND 6
#define NBAND_REFL_MAX 6

/* Right now we are writing out a single output band and no QA band */
#define NBAND_MAX_OUT 1

/* This is the current number of CSV inputs that we are expecting, not counting
   the class response index.  This is for the CSV training inputs, but is also
   used for filling the sample matrix for the actual predictions. */
#define EXPECTED_CSV_INPUTS 50

/* QA bands to be read from the surface reflectance product - cfmask_QA */
#define CFMASK_FILL 256
typedef enum {
  CFMASK,
  NUM_QA_BAND
} QA_Band_t;

/* Structure for bounding geographic coords */
typedef struct {
  double min_lon;  /* Geodetic longitude coordinate (degrees) */ 
  double min_lat;  /* Geodetic latitude coordinate (degrees) */ 
  double max_lon;  /* Geodetic longitude coordinate (degrees) */ 
  double max_lat;  /* Geodetic latitude coordinate (degrees) */ 
  bool is_fill;    /* Flag to indicate whether the point is a fill value; */
} Geo_bounds_t;

/* Structure for lat/long coordinates */
typedef struct {
  double lon;           /* Geodetic longitude coordinate (degrees) */ 
  double lat;           /* Geodetic latitude coordinate (degrees) */ 
  bool is_fill;         /* Flag to indicate whether the point is a fill value;
                           'true' = fill; 'false' = not fill */
} Geo_coord_t;

typedef struct {
  char provider[MAX_STR_LEN];  /* Data provider type */
  char sat[MAX_STR_LEN];       /* Satellite */
  char inst[MAX_STR_LEN];      /* Instrument */
  Date_t acq_date;             /* Acqsition date/time (scene center) */
  Date_t prod_date;            /* Production date (must be available for ETM) */
  float sun_zen;               /* Solar zenith angle (radians; scene center) */
  float sun_az;                /* Solar azimuth angle (radians; scene center) */
  char wrs_sys[MAX_STR_LEN];   /* WRS system */
  int path;                    /* WRS path number */
  int row;                     /* WRS row number */
  int fill;                    /* Fill value for image data */
  int band[NBAND_REFL_MAX];    /* Band numbers */
  Geo_coord_t ul_corner;   /* UL lat/long corner coord */
  Geo_coord_t lr_corner;   /* LR lat/long corner coord */
  Geo_bounds_t bounds;     /* Geographic bounding coordinates */
} Input_meta_t;

/* Structure for the 'input' data type */
typedef struct {
  char *file_name;         /* Input image file name */
  bool open;               /* Open file flag; open = true */
  Input_meta_t meta;       /* Input metadata */
  int nband;               /* Number of input image bands */
  int nqa_band;            /* Number of input QA bands */
  Img_coord_int_t size;    /* Input file size */
  int32 sds_file_id;       /* SDS file id */
  Myhdf_sds_t sds[NBAND_REFL_MAX];
                           /* SDS data structures for image data */
  int16 *buf[NBAND_REFL_MAX];
                           /* Input data buffer (one line of image data) */
  Myhdf_sds_t therm_sds;   /* SDS data structure for thermal image data */
  int16 *therm_buf;        /* Input data buffer (one line of thermal data) */
  Myhdf_sds_t qa_sds[NUM_QA_BAND];
                           /* SDS data structure for QA data */
  uint8 *qa_buf[NUM_QA_BAND];
                           /* Input data buffer (one line of QA data) */
} Input_t;

typedef struct {
  char *file_name;         /* Input image file name */
  bool open;               /* Open file flag; open = true */
  Img_coord_int_t size;    /* Input file size */
  float ul[2];             /* UL corner (x, y) - UL of UL corner, not center */
  float pixsize[2];        /* pixel size (x, y) */
  TIFF *fp_tiff;           /* File pointer for the TIFF file */
  int16 *buf;              /* Input data buffer (one line of image data) */
} Input_Gtif_t;

typedef struct {
  char *file_name;      /* Output file name */
  bool open;            /* Flag to indicate whether output file is open
                           for access; 'true' = open, 'false' = not open */
  int nband;            /* Number of output image bands */
  Img_coord_int_t size; /* Output image size */
  int32 sds_file_id;    /* SDS file id */
  Myhdf_sds_t sds[NBAND_MAX_OUT];
                        /* SDS data structures for image data */
  int16 *buf[NBAND_MAX_OUT];
                        /* Output data buffer (one line of image data) */
} Output_t;


class PredictBurnedArea {

public:
    PredictBurnedArea();
    ~PredictBurnedArea();

    bool GetInputData(Input_t *ds_input, int iband, int iline);
    bool GetInputQALine(Input_t *ds_input, int iband, int iline);
    bool PutOutputLine(Output_t *ds_output, int iband, int iline);
    bool calcBands(Input_t *ds_input);
    void loadModel();
    bool trainModel();
    bool predictModel(int iline, Output_t *ds_output);
    bool loadParametersFromFile(int ac, char* av[]);
    bool readHDR(string filename);
    bool GetGtifInputLYSummaryData(Input_Gtif_t *ds_input, int line,
        BandIndex_t band, Season_t season);
    bool GetGtifInputAnnualMaxData(Input_Gtif_t *ds_input, int line,
        Index_t indx);

    CvMLData cvml;           // contains the training data
    cv::Mat predMat;         // array for input data and predictions
    cv::Mat cfmaskMat;       // array for cfmask data
    cv::Mat lySummaryMat;    // array for last years seasonal summaries
                             // 1D array representing [PBA_NSEASONS][PBA_NBANDS]
    cv::Mat maxIndxMat;      // array for the maximum indices
                             // 1D array representing [PBA_NINDXS]
    CvGBTrees gbtrees;
    int trueCnt;

    /* Parameters from the input config file */
    string INPUT_HDF_FILE;
    bool predict_model;
    string SEASONAL_SUMMARIES_DIR;
    string OUTPUT_HDF_FILE;
    string INPUT_HEADER_FILE;
    string OUTPUT_HEADER_FILE;
    string OUTPUT_TIFF_FILE;
    int TREE_CNT;
    float SHRINKAGE;
    int MAX_DEPTH;
    float SUBSAMPLE_FRACTION;
    string CSV_FILE;
    bool train_model;
    int NCSV_INPUTS;
    string PREDICT_OUT;
    bool VERBOSE;
    int LNDSR_FILL;
    string LOAD_MODEL_XML;
    bool load_model;
    string SAVE_MODEL_XML;
    bool save_model;

    /* Metadata from the input surface reflectance file */
    string projection;
    string datum;
    string zone;
    float ulx;
    float uly;
    float lrx;
    float lry;
};

#endif /* PredictBurnedArea_H_ */
