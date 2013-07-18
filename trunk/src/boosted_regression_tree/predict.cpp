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


  char errstr[MAX_STR_LEN];           /* error string */


  bool PredictBurnedArea::train (int nTrees, float shrink, int depth, const char* csvfile, string predictFile) {
	  ofstream predictOut;
	  predictOut.open(predictFile.c_str());
	  predictOut << "Trees: " << nTrees << " depth " <<  depth << " shrinkage " << shrink << endl << endl;
	    int response_idx = 10;
	  	CvTrainTestSplit spl( 0.5f, true );

	  	cout << second_clock::local_time() << " ======Reading=====" << endl;

	  	//Read in csv file to cvMLData object
	  	cvml.read_csv( csvfile );

	  	cvml.set_response_idx(response_idx);
	  	cvml.change_var_type(response_idx,CV_VAR_CATEGORICAL);
	  	cvml.set_train_test_split( &spl );

	  	cout << second_clock::local_time() << " ======Training Using CvMLData=====" << endl;
	  	gbtrees.train( &cvml, CvGBTreesParams(CvGBTrees::DEVIANCE_LOSS, nTrees, shrink, 0.5f, depth, true));
	  	predictOut << "Train error: " << gbtrees.calc_error( &cvml, CV_TRAIN_ERROR)<< " Test error: " << gbtrees.calc_error( &cvml, CV_TEST_ERROR ) << endl << endl;

	  	cout << second_clock::local_time() << " ======Training Completed=====" << endl;
	  	predictOut.close();

	  		return true;
  }

  bool PredictBurnedArea::predictModel(int iline, Output_t *output) {

	//predict
	cv::Mat sample(1,11,CV_32FC1);

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

		if (cloudMat.at<int>(y,0) == 0 && cloudShadMat.at<int>(y,0) == 0 && landWaterMat.at<int>(y,0) == 0) {
			if (sample.at<float>(0) != -9999) {
				float response = gbtrees.predict( sample);  //returns class label.
				output->buf[0][y] = response * 100;
			} else {
				output->buf[0][y] = -1001;
			}
		} else {
			output->buf[0][y] = 1001;
		}

	}

	PutOutputLine(output, 0, iline);

	sample.release();
	return true;

  }

