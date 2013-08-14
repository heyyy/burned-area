/*
 *  PredictBurnedArea.h
 *
 *  Created on: Nov 15, 2012
 *      Author: jlriegle
 */

#ifndef PredictBurnedArea_H_
#define PredictBurnedArea_H_

#include "const.h"
#include "date.h"
#include "error.h"
#include "mystring.h"
#include "myhdf.h"
#include <opencv/cv.h>
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"
#include "iostream"
#include "fstream"

using namespace std;

/* Type definitions */
typedef enum {FAILURE = 0, SUCCESS = 1} Status_t;

/* There are currently a maximum of 6 reflective bands in the output surface
       reflectance product */
    #define NBAND_REFL_MAX 6
    #define NBAND_REFL_MAX_OUT 1

    /* QA bands - fill_QA, DDV_QA, cloud_QA, cloud_shadow_QA, snow_QA,
       land_water_QA, adjacent_cloud_QA */
    typedef enum {
      FILL,
      DDV,
      CLOUD,
      CLOUD_SHADOW,
      SNOW,
      LAND_WATER,
      ADJ_CLOUD,
      NUM_QA_BAND
    } QA_Band_t;

    /* Integer image coordinates data structure */
    typedef struct {
      int l;                /* line number */
      int s;                /* sample number */

    } Img_coord_int_t;

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
      float wbc;                   /* West bounding coordinate */
      float ebc;                   /* East bounding coordinate */
      float nbc;                   /* North bounding coordinate */
      float sbc;                   /* South bounding coordinate */
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
      char *file_name;      /* Output file name */
      bool open;            /* Flag to indicate whether output file is open
                               for access; 'true' = open, 'false' = not open */
      int nband;            /* Number of input image bands */
      int nqa_band;         /* Number of input QA bands */
      Img_coord_int_t size; /* Output image size */
      int32 sds_file_id;    /* SDS file id */
      Myhdf_sds_t sds[NBAND_REFL_MAX_OUT];
                            /* SDS data structures for image data */
      //float *buf[NBAND_REFL_MAX_OUT];
      int16 *buf[NBAND_REFL_MAX_OUT];
                            /* Output data buffer (one line of image data) */
      Myhdf_sds_t qa_sds[NUM_QA_BAND];
                            /* SDS data structure for QA data */
      uint8 *qa_buf[NUM_QA_BAND];
                            /* Output data buffer (one line of QA data) */
    } Output_t;



class PredictBurnedArea {

public:

    PredictBurnedArea();
    ~PredictBurnedArea();



    bool GetInputData(Input_t *ds_input, int iband, int iline);
    bool GetInputQALine(Input_t *ds_input, int iband, int iline);
    bool PutOutputLine(Output_t *ds_output, int iband, int iline);
//    bool PutOutputLine(int16 *buf, int iband, int iline);
    bool calcBands(Input_t *ds_input);
    bool train (int nTrees, float shrinkage, int depth, const char* csvfile, string predictFile);
//    bool predictModel(int iline);
    bool predictModel(int iline, Output_t *ds_output);
    bool loadParametersFromFile(const string filename,int ac, char* av[]);
    bool readHDR(string filename);

    cv::Mat predMat;
    CvMLData cvml;           // contains the training data
    cv::Mat cloudMat;
    cv::Mat cloudShadMat;
    cv::Mat landWaterMat;
    CvGBTrees gbtrees;

    //CvGBTrees test;

    int trueCnt;
    string INPUT_HDF_FILE;
    string OUTPUT_FILE_NAME;
    string INPUT_HEADER_FILE;
    string OUTPUT_HEADER_NAME;
    string OUTPUT_TIFF_NAME;

    int TREE_CNT;
    float SHRINKAGE;
    int MAX_DEPTH;
    string CSV_FILE;
    string PREDICT_OUT;
    bool VERBOSE;
    string projection;
    string datum;
    string zone;
    float ulx;
    float uly;
    float lrx;
    float lry;

};


#endif /* PredictBurnedArea_H_ */
