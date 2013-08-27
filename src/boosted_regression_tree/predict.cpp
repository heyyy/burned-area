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

void PredictBurnedArea::loadModel () {
  	 gbtrees.load (LOAD_MODEL_XML.c_str());
}

bool PredictBurnedArea::trainModel () {
    int nTrees = TREE_CNT;          /* number of trees in the ensemble */
    float shrink = SHRINKAGE;       /* shrinkage factor */
    int depth = MAX_DEPTH;          /* maximum depth of each tree */
    float subsample_fraction = SUBSAMPLE_FRACTION;  /* fraction of the training
                                       data to be used for training vs.
                                       testing */
    int ncsv_inputs = NCSV_INPUTS;  /* number of inputs per training sample in
                                       the CSV file, not including the response
                                       value */
    string predictFile = PREDICT_OUT; /* output text file for writing the
                                         information regarding the training
                                         parameters and errors */

    /* Identify the index in the training data of the response or
       classification values (0-based) */
    int response_idx = ncsv_inputs;

    /* Document the settings used for training */
    ofstream predictOut;
    predictOut.open (predictFile.c_str());
    predictOut << "Number of trees (weak count): " << nTrees << endl;
    predictOut << "Max tree depth: " << depth << endl;
    predictOut << "Shrinkage: " << shrink << endl;
    predictOut << "Subsample portion (training vs. testing): " <<
        subsample_fraction * 100 << "%" << endl;
    predictOut << "Response index (0-based) in training data: " <<
        response_idx << endl;

    cout << second_clock::local_time() << " ======Reading=====" << endl;

    /* Read in csv file containing the training data to CvMLData object */
    cvml.read_csv (CSV_FILE.c_str());

    /* Change the response value in the training dataset to indicate it
       represents a category/classification and indicate the portion of the
       data that will be used for training vs. testing */
    CvTrainTestSplit spl (subsample_fraction, true);
    cvml.set_response_idx (response_idx);
    cvml.change_var_type (response_idx, CV_VAR_CATEGORICAL);
    cvml.set_train_test_split (&spl);

    /* Train the model using the data read from the input CSV file and the
       specified training parameters */
    cout << second_clock::local_time() <<
        " ======Training Using CvMLData=====" << endl;
    predictOut << "Loss function type: DEVIANCE_LOSS (for classification)" <<
        endl;
    gbtrees.train (&cvml, CvGBTreesParams(CvGBTrees::DEVIANCE_LOSS, nTrees,
        shrink, subsample_fraction, depth, true));
    predictOut << "Train misclassification: " << gbtrees.calc_error (&cvml,
        CV_TRAIN_ERROR) << "%" << endl;
    predictOut << "Test misclassification: " << gbtrees.calc_error (&cvml,
        CV_TEST_ERROR) << "%" << endl;

    cout << second_clock::local_time() <<
        " ======Training Completed=====" << endl;
    predictOut.close();

    /* Save the model if specified */
    if (save_model) {
        gbtrees.save (SAVE_MODEL_XML.c_str());
    }

    return true;
}

bool PredictBurnedArea::predictModel(int iline, Output_t *output) {
    int bnd;                     /* band/index looping variable */
    int season;                  /* season looping variable */
    int indx;                    /* indices looping variable */
    int sample_indx;             /* current sample index for stacking data */
    char errmsg[MAX_STR_LEN];    /* error message */
    cv::Mat sample (1,NCSV_INPUTS+1,CV_32FC1);
                                 /* cvMat to hold the stacks of prediction
                                    information for each of the samples; size
                                    of this must be the same size as the array
                                    of data set to the training module */

    /* Loop through the predicted matrix rows which currently represent
       the samples in the input image.  The columns represent each band. */
    for( int y = 0; y < predMat.rows; y++ ) {
        /* Add the surface reflectance and indices */
        sample.at<float>(0) = predMat.at<float>(y,PREDMAT_B1);
        sample.at<float>(1) = predMat.at<float>(y,PREDMAT_B2);
        sample.at<float>(2) = predMat.at<float>(y,PREDMAT_B3);
        sample.at<float>(3) = predMat.at<float>(y,PREDMAT_B4);
        sample.at<float>(4) = predMat.at<float>(y,PREDMAT_B5);
        sample.at<float>(5) = predMat.at<float>(y,PREDMAT_B7);
        sample.at<float>(6) = predMat.at<float>(y,PREDMAT_NDVI);
        sample.at<float>(7) = predMat.at<float>(y,PREDMAT_NDMI);
        sample.at<float>(8) = predMat.at<float>(y,PREDMAT_NBR);
        sample.at<float>(9) = predMat.at<float>(y,PREDMAT_NBR2);
 
        /* Add the current year and last year seasonal summaries, and add
           them as a group of bands/indices per season. */
        sample_indx = PREDMAT_NBR2+1;
        for (season = 0; season < PBA_NSEASONS; season++) {
            for (bnd = 0; bnd < PBA_NBANDS; bnd++) {
                sample.at<float>(sample_indx++) =
                    cySummaryMat.at<float>(y,season*PBA_NBANDS+bnd);
            }
        }
        for (season = 0; season < PBA_NSEASONS; season++) {
            for (bnd = 0; bnd < PBA_NBANDS; bnd++) {
                sample.at<float>(sample_indx++) =
                    lySummaryMat.at<float>(y,season*PBA_NBANDS+bnd);
            }
        }

        /* Add the last year annual maximums for the indices */
        for (indx = 0; indx < PBA_NINDXS; indx++)
            sample.at<float>(sample_indx++) = maxIndxMat.at<float>(y,indx);

        /* Add the deltas of the annual maximums for the indices */
        if (fillMat.at<unsigned char>(y) != 0) { // fill
            for (indx = 0; indx < PBA_NINDXS; indx++)
                sample.at<float>(sample_indx++) = LNDSR_FILL;
        }
        else { // not fill
            for (indx = 0; indx < PBA_NINDXS; indx++) {
                sample.at<float>(sample_indx++) =
                    predMat.at<float>(y,PREDMAT_NDVI+indx) -
                    maxIndxMat.at<float>(y,indx);
            }
        }

        /* Validate that the stack is not larger than the sample matrix was
           set up for */
        if (sample_indx > NCSV_INPUTS) {
            sprintf (errmsg, "The number of bands stacked in this sample "
                "(%d) is greater than the defined matrix size (%d).",
            sample_indx, NCSV_INPUTS);
            RETURN_ERROR (errmsg, "predict_model", false);
        }

/*printf ("DEBUG: Prediction sample for line %d, sample %d:", iline, y);
for (indx = 0; indx < NCSV_INPUTS; indx++)
    printf (" %.1f", sample.at<float>(indx));
printf ("\n");
*/

        /* If the current pixel isn't cloudy, water, or fill, then run the
           prediction for this pixel. If the pixel is cloud, shadow, or water,
           then set it to PBA_CLOUD_WATER. If the pixel is fill then set it to
           PBA_FILL. */
        if (cloudMat.at<unsigned char>(y) == 0 &&
            cloudShadMat.at<unsigned char>(y) == 0 &&
            landWaterMat.at<unsigned char>(y) == 0) {
            if (fillMat.at<unsigned char>(y) == 0) {
                /* do the probability mapping for burned (class of 1) */
                float response = gbtrees.predict_prob (sample, 1);
                output->buf[0][y] = (int16) (response * 100.0 + 0.5);
            } else {  /* fill pixel */
                output->buf[0][y] = PBA_FILL;
            }
        } else {  /* cloudy or water pixel */
            output->buf[0][y] = PBA_CLOUD_WATER;
        }
    }

    /* Write the line of probability mappings to the output file */
    PutOutputLine (output, 0, iline);
    sample.release();

    return true;
}
