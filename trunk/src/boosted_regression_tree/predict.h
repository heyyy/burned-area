/*
 * predict.h
 *
 *  Created on: Nov 26, 2012
 *      Author: jlriegle
 */

#ifndef PREDICT_H_
#define PREDICT_H_

#include <string>
#include <opencv/cv.h>
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"

#include "PredictBurnedArea.h"

using namespace std;

// define the pixel values for cloud, cloud shadow, and water for the output
// prediction scenes
#define PBA_CLOUD_WATER 1001

// define the pixel values for fill for the output prediction scenes
#define PBA_FILL -1001

// define the fill value for the input surface reflectance products
#define LNDSR_FILL -9999

#endif /* PREDICT_H_ */
