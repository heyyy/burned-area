/*
 * predict.cpp\
 *
 *
 *  Created on: Nov 26, 2012
 *      Author: jlriegle
 */

#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "predict.h"
#include "PredictBurnedArea.h"
#include "output.h"
#include "error.h"
#include <math.h>

using namespace boost::posix_time;
using namespace std;

bool PredictBurnedArea::train (int nTrees, float shrink, int depth,
    const char* csvfile, string predictFile) {

    int response_idx = 10;   // index in the training data of the response or
                             // or classification values (0-based)
    float subsample_portion = 0.5;  // portion of training set used for each
                                    // algorithm iteration vs. testing
    ofstream predictOut;
    predictOut.open(predictFile.c_str());
    predictOut << "Number of trees (weak count): " << nTrees << endl;
    predictOut << "Max tree depth: " << depth << endl;
    predictOut << "Shrinkage: " << shrink << endl;
    predictOut << "Subsample portion (training vs. testing): " <<
        subsample_portion * 100 << "%" << endl;
    predictOut << "Response index (0-based) in training data: " <<
        response_idx << endl;

    cout << second_clock::local_time() << " ======Reading=====" << endl;

    //Read in csv file containing the training data to CvMLData object
    cvml.read_csv( csvfile );

    //Change the response value in the training dataset to indicate it
    //represents a category/classification and indicate the portion of the
    //data that will be used for training vs. testing
    CvTrainTestSplit spl( subsample_portion, true );
    cvml.set_response_idx(response_idx);
    cvml.change_var_type(response_idx,CV_VAR_CATEGORICAL);
    cvml.set_train_test_split( &spl );

    cout << second_clock::local_time() <<
        " ======Training Using CvMLData=====" << endl;
    predictOut << "Loss function type: DEVIANCE_LOSS (for classification)" <<
        endl;
    gbtrees.train( &cvml, CvGBTreesParams(CvGBTrees::DEVIANCE_LOSS, nTrees,
        shrink, subsample_portion, depth, true));
    predictOut << "Train error: " << gbtrees.calc_error( &cvml,
        CV_TRAIN_ERROR)<< " Test error: " <<
        gbtrees.calc_error( &cvml, CV_TEST_ERROR ) << endl << endl;

    cout << second_clock::local_time() <<
        " ======Training Completed=====" << endl;
    predictOut.close();

    return true;
}

bool PredictBurnedArea::predictModel(int iline, Output_t *output) {
    //predict
    cv::Mat sample(1,11,CV_32FC1);

    // Loop through the predicted matrix rows which currently represent
    // the samples in the input image.  The columns represent each band.
    for( int y = 0; y < predMat.rows; y++ ) {
        sample.at<float>(0) = predMat.at<float>(y,0);
        sample.at<float>(1) = predMat.at<float>(y,1);
        sample.at<float>(2) = predMat.at<float>(y,2);
        sample.at<float>(3) = predMat.at<float>(y,3);
        sample.at<float>(4) = predMat.at<float>(y,4);
        sample.at<float>(5) = predMat.at<float>(y,5);
        sample.at<float>(6) = predMat.at<float>(y,6);
        sample.at<float>(7) = predMat.at<float>(y,7);
        sample.at<float>(8) = predMat.at<float>(y,8);
        sample.at<float>(9) = predMat.at<float>(y,9);

        // If the current pixel isn't cloudy, water, or fill, then run the
        // prediction for this pixel. If the pixel is cloud, shadow, or water,
        // then set it to PBA_CLOUD_WATER. If the pixel is fill then set it to
        // PBA_FILL.
        if (cloudMat.at<unsigned char>(y,0) == 0 &&
            cloudShadMat.at<unsigned char>(y,0) == 0 &&
            landWaterMat.at<unsigned char>(y,0) == 0) {
            if (sample.at<float>(0) != LNDSR_FILL) {
                // get the probability that the pixel is burned (class of 1)
                float response = gbtrees.predict_prob(sample, 1);
                output->buf[0][y] = (int16) (response * 100.0 + 0.5);
            } else {  // fill pixel
                output->buf[0][y] = PBA_FILL;
            }
        } else {  // cloudy or water pixel
            output->buf[0][y] = PBA_CLOUD_WATER;
        }
    }

    PutOutputLine(output, 0, iline);
    sample.release();
    return true;
}
