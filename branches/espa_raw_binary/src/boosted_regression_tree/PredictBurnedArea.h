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

#include <iostream>
#include <fstream>
#include <stdint.h>
#include "cv.h"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"
#include "const.h"
#include "error.h"
#include "mystring.h"

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

/* This is the current number of CSV inputs that we are expecting, not counting
   the class response index.  This is for the CSV training inputs, but is also
   used for filling the sample matrix for the actual predictions. */
#define EXPECTED_CSV_INPUTS 50

typedef struct {
  int acq_year;         /* Acquisition time - year (scene center) */
  int fill;             /* Fill value for image data */
} Input_meta_t;

/* Structure for the 'input' surface reflectance and mask */
typedef struct {
  char *base_name;         /* Input surface reflectance image base file name */
  char *mask_name;         /* Input image mask (QA) file name */
  bool open;               /* Open file flag; open = true */
  int nband;               /* Number of input image (reflectance) bands */
  Img_coord_int_t size;    /* Input file size */
  FILE *fp_img[NBAND_REFL_MAX]; /* File pointers for image data */
  int16 *img_buf;          /* Input data buffer (one line of image data) */
  FILE *fp_qa;             /* File pointer for QA data */
  int8 *qa_buf;            /* Input mask/qa buffer (one line of data) */
} Input_t;

/* Structure for the 'output' burn area data */
typedef struct {
  char *file_name;      /* Output file name */
  bool open;            /* Flag to indicate whether output file is open
                           for access; 'true' = open, 'false' = not open */
  Img_coord_int_t size; /* Output image size */
  FILE *fp_img;         /* File pointer for the image data */
  int16 *buf;           /* Output data buffer (one line of image data) */
} Output_t;

/* Structure for the 'input' seasonal summary and annual max image data */
typedef struct {
  char *file_name;         /* Input image file name */
  bool open;               /* Open file flag; open = true */
  Img_coord_int_t size;    /* Input file size */
  FILE *fp_img;            /* File pointer for the image data */
  int16 *buf;              /* Input data buffer (one line of image data) */
} Input_Rb_t;


class PredictBurnedArea {

public:
    PredictBurnedArea();
    ~PredictBurnedArea();

    bool GetInputData(Input_t *ds_input, int iband);
    bool GetInputQALine(Input_t *ds_input, int iband);
    bool PutOutputLine(Output_t *ds_output, int iline);
    bool calcBands(Input_t *ds_input);
    void loadModel();
    bool trainModel();
    bool predictModel(int iline, Output_t *ds_output);
    bool loadParametersFromFile(int ac, char* av[]);
    bool GetRbInputLYSummaryData(Input_Rb_t *ds_input, int line,
        BandIndex_t band, Season_t season);
    bool GetRbInputAnnualMaxData(Input_Rb_t *ds_input, int line, Index_t indx);

    CvMLData cvml;           // contains the training data
    cv::Mat predMat;         // array for input data and predictions
    cv::Mat qaMat;           // array for QA/mask data
    cv::Mat lySummaryMat;    // array for last years seasonal summaries
                             // 1D array representing [PBA_NSEASONS][PBA_NBANDS]
    cv::Mat maxIndxMat;      // array for the maximum indices
                             // 1D array representing [PBA_NINDXS]
    CvGBTrees gbtrees;
    int trueCnt;

    /* Parameters from the input config file */
    string INPUT_BASE_FILE;
    string INPUT_MASK_FILE;
    int INPUT_FILL_VALUE;
    bool predict_model;
    string SEASONAL_SUMMARIES_DIR;
    string OUTPUT_IMG_FILE;
    int TREE_CNT;
    float SHRINKAGE;
    int MAX_DEPTH;
    float SUBSAMPLE_FRACTION;
    string CSV_FILE;
    bool train_model;
    int NCSV_INPUTS;
    string PREDICT_OUT;
    bool VERBOSE;
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
