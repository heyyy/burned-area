/*****************************************************************************
FILE: predict.cpp
  
PURPOSE: Contains functions for training the model and running predictions
using the model.  All methods are part of the PredictBurnedArea class.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
11/26/2012    Jodi Riegle      Original development
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
                               Modified to support saving the model and then
                               reload the model

NOTES:
*****************************************************************************/

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

/******************************************************************************
MODULE: loadModel (class PredictBurnedArea)

PURPOSE: Loads a previously trained and saved model.
 
RETURN VALUE:
Type = None
Value          Description
-----          -----------

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/3/2013      Gail Schmidt     Original development

NOTES:
*****************************************************************************/
void PredictBurnedArea::loadModel ()
{
  	gbtrees.load (LOAD_MODEL_XML.c_str());
}


/******************************************************************************
MODULE: trainModel (class PredictBurnedArea)

PURPOSE: Trains the model using a gradient boosted regression tree with the
specified parameters from the configuration file.  The model will be saved,
if specified, to the user-specified output XML file.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error training the model
true           Successful training of the model

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
11/26/2012    Jodi Riegle      Original development
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
  1. It's assumed the configuration parameters (class members) have already
     been initialized.
*****************************************************************************/
bool PredictBurnedArea::trainModel ()
{
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
        shrink, subsample_fraction, depth, true), false);
    predictOut << "Train misclassification: " << gbtrees.calc_error (&cvml,
        CV_TRAIN_ERROR, 0) << "%" << endl;
    predictOut << "Test misclassification: " << gbtrees.calc_error (&cvml,
        CV_TEST_ERROR, 0) << "%" << endl;

    cout << second_clock::local_time() <<
        " ======Training Completed=====" << endl;
    predictOut.close();

    /* Save the model if specified */
    if (save_model) {
        gbtrees.save (SAVE_MODEL_XML.c_str());
    }

    return true;
}


/******************************************************************************
MODULE: predictModel (class PredictBurnedArea)

PURPOSE: Run the gradient boosted regression tree model predictions to
obtain the probability mappings for burned probabilities.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error running the model
true           Model ran successfully

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
11/26/2012    Jodi Riegle      Original development
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
                               Modified to write probability mappings vs.
                               simple burn/unburned classifications
9/10/2011     Gail Schmidt     Modified to use the cfmask QA values which are
                               more accurate than the SR QA values

NOTES:
  1. It's assumed the model has already been trained and/or loaded.
*****************************************************************************/
bool PredictBurnedArea::predictModel
(
    int iline,            /* I: line to be processed (0-based) */
    Output_t *output      /* O: 'output' data structure where buf contains
                                the probability mapping values */
)
{
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
 
        /* Add the last year seasonal summaries, and add them as a group of
           bands/indices per season. */
        sample_indx = PREDMAT_NBR2+1;
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
        if (cfmaskMat.at<unsigned char>(y) == CFMASK_FILL) { // fill
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

        /* If the current pixel isn't cloudy, water, or fill, then run the
           prediction for this pixel. If the pixel is cloud, shadow, or water,
           then set it to PBA_CLOUD_WATER. If the pixel is fill then set it to
           PBA_FILL. */
        if (cfmaskMat.at<unsigned char>(y) != CFMASK_CLOUD &&
            cfmaskMat.at<unsigned char>(y) != CFMASK_SHADOW &&
            cfmaskMat.at<unsigned char>(y) != CFMASK_WATER) {
            if (cfmaskMat.at<unsigned char>(y) != CFMASK_FILL) {
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
