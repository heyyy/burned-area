/*****************************************************************************
FILE: predict.h
  
PURPOSE: Contains prediction and probability mapping constants.

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

#ifndef PREDICT_H_
#define PREDICT_H_

#include <string>
#include "cv.h"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"

#include "PredictBurnedArea.h"

using namespace std;

/* define the pixel values for cloud, cloud shadow, and water for the output
   prediction scenes */
#define PBA_CLOUD_WATER -9998

/* define the pixel values for fill for the output prediction scenes */
#define PBA_FILL -9999

#endif /* PREDICT_H_ */
