/*
 * main.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: jlriegle
 */

#include "error.h"
#include "input.h"
#include "output.h"
#include "PredictBurnedArea.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
//#define DEBUG

using namespace boost::posix_time;

int main(int argc, char* argv[]) {

    PredictBurnedArea pba;
    char errstr[MAX_STR_LEN]; /* error string */
    char *output_header_name = NULL; /* output filename */
    char *input_header_name = NULL; /* output filename */
    char *output_file_name = NULL; /* output filename */
    int ib; /* band and line counters */
    char sds_names[NBAND_REFL_MAX][MAX_STR_LEN]; /* array of image SDS names */
    char qa_sds_names[NUM_QA_BAND][MAX_STR_LEN]; /* array of QA SDS names */
    Input_t *input = NULL; /* input data and metadata */
    Output_t *output = NULL; /* output structure and metadata */


    /* Validate the command-line parameter for the config file exists */
    if (argc != 2 || strstr(argv[1], "-help")) {
        sprintf(errstr, "Usage: predict_burned_area config_file");
        ERROR(errstr, "main");
    }

    /* Get the config file name */
    string config_file = strdup(argv[1]);
    if (config_file == "") {
        sprintf(errstr, "Config file was not provided.");
        ERROR(errstr, "main");
    }

    /* Get the input file name */
    pba.loadParametersFromFile(config_file, argc, argv);
    const char* csvFile = pba.CSV_FILE.c_str();
    input = OpenInput(strdup(pba.INPUT_HDF_FILE.c_str()));
    if (input == (Input_t *) NULL) {
        sprintf(errstr, "opening the input config file: %s",
            pba.INPUT_HDF_FILE.c_str());
        ERROR(errstr, "main");
    }

    /* Call gdalinfo to obtain some metadata for the input HDF file */
#ifdef DEBUG
    string gdalinfo = "gdalinfo";
    const char* writeCommand2 = (gdalinfo + " " + pba.INPUT_HDF_FILE).c_str();
    system(writeCommand2);
#endif

    /* Print some info to show how the input metadata works */
#ifdef DEBUG
    cout << "DEBUG: Copying reflective and QA bands from " << pba.INPUT_HDF_FILE
         << " to " << pba.OUTPUT_FILE_NAME << endl;
    cout << "DEBUG: Number of input reflective bands: " << input->nband << endl;
    cout << "DEBUG: Number of input QA bands: " << input->nqa_band << endl;
    cout << "DEBUG: Number of input thermal bands: " << 1 << endl;
    cout << "DEBUG: Number of input lines: " << input->size.l << endl;
    cout << "DEBUG: Number of input samples: " << input->size.s << endl;
    cout << "DEBUG: Provider is " << input->meta.provider << endl;
    cout << "DEBUG: Satellite is " << input->meta.sat << endl;
    cout << "DEBUG: Instrument is " << input->meta.inst << endl;
    cout << "DEBUG: WRS system is " << input->meta.wrs_sys << endl;
    cout << "DEBUG: Path is " << input->meta.path << endl;
    cout << "DEBUG: Row is " << input->meta.row << endl;
    cout << "DEBUG: Fill value is " << input->meta.fill << endl;
    for (ib = 0; ib < input->nband; ib++) {
        cout << "DEBUG: Band -->" << ib << endl;
        cout << "DEBUG:   SDS name is " << input->sds[ib].name << endl;
        cout << "DEBUG:   SDS rank: " << input->sds[ib].rank << endl;
    }
    for (ib = 0; ib < input->nqa_band; ib++) {
        cout << "DEBUG: QA Band -->" << ib << endl;
        cout << "DEBUG:   SDS name is " << input->qa_sds[ib].name << endl;
        cout << "DEBUG:   SDS rank: " << input->qa_sds[ib].rank << endl;
    }
    cout << "DEBUG: Thermal Band -->" << endl;
    cout << "DEBUG:   SDS name is " << input->therm_sds.name << endl;
    cout << "DEBUG:   SDS rank: " << input->therm_sds.rank << endl;
#endif

    for (ib = 0; ib < input->nband; ib++)
        strcpy(&sds_names[ib][0], input->sds[ib].name);
    for (ib = 0; ib < input->nqa_band; ib++)
        strcpy(&qa_sds_names[ib][0], input->qa_sds[ib].name);

    /* Create and open output file */
    output_file_name = strdup(pba.OUTPUT_FILE_NAME.c_str());
    input_header_name = strdup(pba.INPUT_HEADER_FILE.c_str());
    output_header_name = strdup(pba.OUTPUT_HEADER_NAME.c_str());
    if (!CreateOutput(output_file_name, input_header_name,
        output_header_name)) {
        sprintf(errstr, "creating output file - %s",
            pba.OUTPUT_FILE_NAME.c_str());
        ERROR(errstr, "main");
    }

    output = OpenOutput(output_file_name, 1, 0, sds_names, qa_sds_names,
        &input->size);
    if (output == NULL) {
        sprintf(errstr, "opening output file: %s",
            pba.OUTPUT_FILE_NAME.c_str());
        ERROR(errstr, "main");
    }

    /* Train the model */
    if (!pba.train(pba.TREE_CNT, pba.SHRINKAGE, pba.MAX_DEPTH, csvFile,
        pba.PREDICT_OUT)) {
        sprintf(errstr, "Error training data ");
        ERROR(errstr, "main");
    }

    /* Set up arrays for the predicted data, cloud, cloud shadow, and
       land/water data.  These will hold a single line and single/multiple
       bands, depending on what is being represented.  For predMat (predicted
       matrix), bands 0-5 are the reflective bands (1-5, and 7), 6=NDVI, 7=NDMI,
       8=NBR, 9=NBR2.  cloudMat represents the cloud_QA band.  cloudShadMat
       represents the cloud_shadow_QA.  landWaterMat represents the
       land_water_QA band. */
    pba.predMat.create(input->size.s, 10, CV_32FC1);
    pba.cloudMat.create(input->size.s, 1, CV_8U);
    pba.cloudShadMat.create(input->size.s, 1, CV_8U);
    pba.landWaterMat.create(input->size.s, 1, CV_8U);

    cout << second_clock::local_time() << " ======= Predict Started ======== "
         << endl;

    /* Loop through the lines in the image, read the reflective data, compute
       needed index products, read the QA data, and run the predictions */
    for (int iline = 0; iline < input->size.l; iline++) {
        if ((iline % 100 == 0)) {
            cout << second_clock::local_time() << " ======= line " << iline
                 << " ======== " << endl;
        }

        /* Read each reflective band for the current line */
        for (int ib = 0; ib < input->nband; ib++) {
            if (!pba.GetInputData(input, ib, iline)) {
                sprintf(errstr, "Reading input image data for line %d, "
                        "band %d", 0, 1);
                ERROR(errstr, "main");
            }
        }

        /* Compute the NDVI, NDMI, NBR, and NBR2 for the current line */
        if (!pba.calcBands(input)) {
            sprintf(errstr, "Reading input image data for line %d, band %d",
                0, 1);
            ERROR(errstr, "main");
        }

        /* Read the QA bands for the current line */
        for (int ib = 0; ib < input->nqa_band; ib++) {
            if (!pba.GetInputQALine(input, ib, iline)) {
                sprintf(errstr, "Reading input image data for line %d, "
                        "band %d", 0, 1);
                ERROR(errstr, "main");
            }
        }

        /* Run the predictions for the current line */
        pba.predictModel(iline, output);
    }

    cout << second_clock::local_time()
         << " ======= Predict Completed ======== " << endl;

    /* Close the input file and free the structure */
    CloseInput(input);
    FreeInput(input);

    /* Close the output file and free the structure */
    CloseOutput(output);
    FreeOutput(output);

    /* write output hdf to tiff */
    stringstream ss1, ss2, ss3, ss4;
    ss1 << input->meta.wbc;
    string ulx = ss1.str();
    ss2 << input->meta.nbc;
    string uly = ss2.str();
    ss3 << input->meta.ebc;
    string llx = ss3.str();
    ss4 << input->meta.sbc;
    string lly = ss4.str();

    pba.readHDR(pba.OUTPUT_HEADER_NAME);

    string gdalTrans = "gdal_translate -a_srs '+proj=" + pba.projection + " +zone=" + pba.zone + " +datum=" + pba.datum + "' -a_ullr " + boost::lexical_cast<string>(pba.ulx) + " " + boost::lexical_cast<string>(pba.uly) + " " + boost::lexical_cast<string>(pba.lrx) + " " + boost::lexical_cast<string>(pba.lry) + " -of GTiff -co TFW=YES -b 1 -mask none " + output_file_name + " " +  pba.OUTPUT_TIFF_NAME;

    const char* writeCommand = gdalTrans.c_str();
    system(writeCommand);

    pba.predMat.release();
    pba.cloudMat.release();
    pba.cloudShadMat.release();
    pba.landWaterMat.release();
};

