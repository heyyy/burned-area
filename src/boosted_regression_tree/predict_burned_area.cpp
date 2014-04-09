/******************************************************************************
FILE:  predict_burned_area.cpp

PURPOSE:  This is the main file which handles the overall boosted regression
tree application and processing.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
9/15/2012   Jodi Riegle      Original development
9/3/2013    Gail Schmidt     Modified to work in the ESPA environment
                             Modified to produce probability mappings instead
                             of fire / no fire classifications
                             Modified to support training the model only,
                             training and running the model, and loading a
                             previous model for running predictions.
9/25/2013   Gail Schmidt     Modified to use unique filenames for the resized
                             products to allow for processing of multiple
                             scenes in the same directory.  Also added code to
                             remove these temporary resized files.
9/10/2013   Gail Schmidt     Modified to use the cfmask QA values which are
                             more accurate than the SR QA values
12/8/2013   Gail Schmidt     Backed out the use of the cfmask values and
                             returned to using the LEDAPS SR QA values
3/26/2014   Gail Schmidt     Modified to use the ESPA internal file format.
                             Modified to input the scene files that are
                             resampled to the maximum geographic extents so
                             that they match the seasonal summaries and annual
                             maximums boundaries.
                             Modified to use the single mask file created
                             during seasonal summary processing.

NOTES:
******************************************************************************/

#include <time.h>
#include <sys/time.h>
#include "error.h"
#include "input.h"
#include "input_rb.h"
#include "output.h"
#include "PredictBurnedArea.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
//#define DEBUG

using namespace boost::posix_time;
char season_str[PBA_NSEASONS][MAX_STR_LEN] = {"winter", "spring", "summer",
    "fall"};   /* string to represent the seasons */
char band_indx_str[PBA_NBANDS][MAX_STR_LEN] = {"band3", "band4", "band5",
    "band7", "ndvi", "ndmi", "nbr", "nbr2"};   /* string to represent the
     bands and indices in the seasonal summaries */
char indx_str[PBA_NINDXS][MAX_STR_LEN] = {"ndvi", "ndmi", "nbr", "nbr2"};
    /* string to represent the indices in the annual maximums */

/******************************************************************************
MODULE:  main

PURPOSE:  Reads the user specified arguments, reads the config file, handles
training the model and/or loading and running the model on the user-specified
file and using the user-specified configurations for the model.

RETURN VALUE:
Type = int
Value          Description
-----          -----------
EXIT_FAILURE   Non-zero value to indicate an error occurred during processing
EXIT_SUCCESS   Zero value to indicate successful processing

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
12/8/2013     Gail Schmidt     Added support for the adjacent cloud mask for
                               the overall QA values

NOTES:
  1. predict_burned_area --help will provide input information.
  2. This code is a mixture of true object-oriented C++ code and
     traditional C-based code (error handling, file read/write)
******************************************************************************/
int main(int argc, char* argv[]) {
    PredictBurnedArea pba;
    int bnd;                           /* band/index looping variable */
    int season;                        /* season looping variable */
    int indx;                          /* indices looping variable */
    int ib;                            /* band and line counters */
    int acq_year;                      /* acquisition year of input scene */
    char errstr[MAX_STR_LEN];          /* error string */
    char gdal_cmd[MAX_STR_LEN];        /* command string for GDAL merge call */
    char rm_cmd[MAX_STR_LEN];          /* command string for removing temp
                                          files */
    char *output_file_name = NULL;     /* output filename */
    char lySummaryFile[PBA_NSEASONS][PBA_NBANDS][MAX_STR_LEN];/* last year */
    char maxIndxFile[PBA_NINDXS][MAX_STR_LEN];                /* max indices */
    char timestr[MAX_STR_LEN];         /* random string to create random
                                          filenames for resizing */
    Input_t *input = NULL;             /* input data and metadata */
    Output_t *output = NULL;           /* output structure and metadata */
    Input_Rb_t *lySummaryPtr[PBA_NSEASONS][PBA_NBANDS];  /* last year ptr */
    Input_Rb_t *maxIndxPtr[PBA_NINDXS];                  /* max indices ptr */
    Space_def_t space_def;             /* spatial definition information */
    int out_sds_types[NBAND_MAX_OUT] = {DFNT_INT16}; /* array of image SDS
                                          types */

    /* Read the config file */
    if (!pba.loadParametersFromFile (argc, argv)) {
        /* error message already printed in loadParametersFromFile so just
           exit */
        exit (EXIT_FAILURE);
    }
    char* baseFile = (char *) pba.INPUT_BASE_FILE.c_str();
    char* maskFile = (char *) pba.INPUT_MASK_FILE.c_str();
    char* seasonalSummaryDir = (char *) pba.SEASONAL_SUMMARIES_DIR.c_str();

    /* Print some input processing info */
    if (pba.VERBOSE) {
        if (pba.train_model) {
            cout << "Training the model using the following parameters -"
                 << endl;
            cout << "   Tree count: " << pba.TREE_CNT << endl;
            cout << "   Maximum tree depth: " << pba.MAX_DEPTH << endl;
            cout << "   Shrinkage: " << pba.SHRINKAGE << endl;
            cout << "   Subsample fraction: " << pba.SUBSAMPLE_FRACTION << endl;
            cout << "   Input CSV file: " << pba.CSV_FILE.c_str() << endl;
            cout << "   Number of CSV predictors: " << pba.NCSV_INPUTS << endl;
        }
        if (pba.save_model)
            cout << "Model will be saved to XML file: "
                 << pba.SAVE_MODEL_XML.c_str() << endl;
        if (pba.predict_model) {
            cout << "Model predictions will be completed using the following "
                    "parameters -" << endl;
            cout << "  Input surface reflectance file: " << baseFile << endl;
            cout << "  Input mask file: " << maskFile << endl;
            cout << "  Fill value: " << pba.INPUT_FILL_VALUE << endl;
            cout << "  Input seasonal summaries file: " << seasonalSummaryDir
                 << endl;
            if (pba.load_model)
                cout << "Model will be loaded from XML file: "
                     << pba.LOAD_MODEL_XML.c_str() << endl;
        }
    }

    /* Train the model using the data provided in the input CSV file.  If
       training is not specified then load the provided XML file for the
       model. */
    if (pba.train_model) {
        if (!pba.trainModel ()) {
            sprintf (errstr, "error training the model");
            ERROR(errstr, "main");
        }
    }
    else if (pba.load_model) {
        pba.loadModel ();
    }

    /* If not running model predictions, then we are done */
    if (!pba.predict_model)
        exit (EXIT_SUCCESS);

    /* Open the input image and mask files */
    input = OpenInput (baseFile, maskFile, pba.INPUT_FILL_VALUE);
    if (input == NULL) {
        sprintf (errstr, "opening the input image or mask files");
        ERROR(errstr, "main");
    }

    /* Print some input metadata info */
    if (pba.VERBOSE) {
        cout << "Number of input reflective bands: " << input->nband
             << endl;
        cout << "Number of input thermal bands: " << 1 << endl;
        cout << "Number of input mask bands: " << 1 << endl;
        cout << "Number of input lines: " << input->size.l << endl;
        cout << "Number of input samples: " << input->size.s << endl;
        cout << "Acquisition year: " << input->meta.acq_year << endl;
        cout << "Fill value: " << input->meta.fill << endl;
    }

    /* Pull the acquisition year from the acquisition date */
    acq_year = input->meta.acq_year;

    /* Create and open output file */
    output_file_name = strdup(pba.OUTPUT_IMG_FILE.c_str());
    if (!CreateOutputHeader (baseFile, output_file_name)) {
        sprintf(errstr, "creating output header file for %s", output_file_name);
        ERROR(errstr, "main");
    }

    output = OpenOutput (output_file_name, &input->size);
    if (output == NULL) {
        sprintf (errstr, "opening output file: %s", output_file_name);
        ERROR(errstr, "main");
    }

    /* Create the filenames for the seasonal summmaries and annual maximums.
       Files are expected to reside in the seasonal summaries directory with
       subdirectories of refl, ndvi, ndmi, nbr, nbr2.  Open the files and read
       the associated metadata. */
    printf (".... Seasonal summary products\n");
    for (season = 0; season < PBA_NSEASONS; season++) {
        for (bnd = 0; bnd < PBA_NBANDS; bnd++) {
            if (bnd < BND_NDVI) {  /* reflectance bands */
                /* Set up the filenames */
                sprintf (lySummaryFile[season][bnd], "%s/refl/%d_%s_%s.img",
                    seasonalSummaryDir, acq_year-1, season_str[season],
                    band_indx_str[bnd]);
            }
            else {  /* index bands */
                /* Set up the filenames */
                sprintf (lySummaryFile[season][bnd], "%s/%s/%d_%s_%s.img",
                    seasonalSummaryDir, band_indx_str[bnd], acq_year-1,
                    season_str[season], band_indx_str[bnd]);
            }

            /* Open the seasonal summary files */
            lySummaryPtr[season][bnd] = OpenRbInput(lySummaryFile[season][bnd]);
            if (lySummaryPtr[season][bnd] == NULL) {
                sprintf (errstr, "opening file: %s",
                    lySummaryFile[season][bnd]);
                ERROR (errstr, "main");
            }
        }
    }

    printf (".... Annual maximum products\n");
    for (indx = 0; indx < PBA_NINDXS; indx++) {
        /* Set up the filenames - annual max is for last year */
        sprintf (maxIndxFile[indx], "%s/%s/%d_maximum_%s.tif",
            seasonalSummaryDir, indx_str[indx], acq_year-1, indx_str[indx]);

        /* Open the annual maximum files */
        maxIndxPtr[indx] = OpenRbInput (maxIndxFile[indx]);
        if (maxIndxPtr[indx] == NULL) {
            sprintf (errstr, "opening file: %s", maxIndxFile[indx]);
            ERROR (errstr, "main");
        }
    }

    /* Set up arrays for the seasonal summaries and annual maximums */
    pba.lySummaryMat.create (input->size.s, PBA_NBANDS*PBA_NSEASONS, CV_32FC1);
    pba.maxIndxMat.create (input->size.s, PBA_NINDXS, CV_32FC1);

    /* Set up arrays for the predicted data and QA/mask data.  These will hold
       a single line and single/multiple bands, depending on what is being
       represented.  For predMat (predicted matrix), bands 0-5 are the
       reflective bands (1-5, and 7), 6=NDVI, 7=NDMI, 8=NBR, 9=NBR2.  qaMat
       represents the QA band. */
    pba.predMat.create (input->size.s, 10, CV_32FC1);
    pba.qaMat.create (input->size.s, 1, CV_8U);

    cout << second_clock::local_time() << " ======= Predict Started ======== "
         << endl;

    /* Loop through the lines in the image, read the reflective data, compute
       needed index products, read the QA data, and run the predictions */
    for (int iline = 0; iline < input->size.l; iline++) {
        if (iline % 100 == 0) {
            cout << second_clock::local_time() << " ======= line " << iline
                 << " ======== " << endl;
        }

        /* Read each reflective band for the current line */
        for (int ib = 0; ib < input->nband; ib++) {
            if (!pba.GetInputData (input, ib, iline)) {
                sprintf (errstr, "reading input image data for line %d, "
                    "band %d", 0, 1);
                ERROR(errstr, "main");
            }
        }

        /* Compute the NDVI, NDMI, NBR, and NBR2 for the current line */
        if (!pba.calcBands (input)) {
            sprintf (errstr, "reading input image data for line %d, band %d",
                0, 1);
            ERROR(errstr, "main");
        }

        /* Read the QA bands for the current line */
        for (int ib = 0; ib < input->nqa_band; ib++) {
            if (!pba.GetInputQALine (input, ib, iline)) {
                sprintf (errstr, "reading input QA data for line %d, "
                    "band %d", 0, 1);
                ERROR(errstr, "main");
            }
        }

        /* Read the seasonal summaries for the previous year */
        for (bnd = 0; bnd < PBA_NBANDS; bnd++) {
            for (season = 0; season < PBA_NSEASONS; season++) {
                if (!pba.GetRbInputLYSummaryData (lySummaryPtr[season][bnd],
                    iline, (BandIndex_t) bnd, (Season_t) season)) {
                    sprintf (errstr, "reading previous year seasonal summary "
                        "data for line %d, band %s, season %s", iline,
                        band_indx_str[bnd], season_str[season]);
                    ERROR(errstr, "main");
                }
            }
        }

        /* Read the annual maximums for last year */
        for (indx = 0; indx < PBA_NINDXS; indx++) {
            if (!pba.GetRbInputAnnualMaxData (maxIndxPtr[indx], iline,
                (Index_t) indx)) {
                sprintf (errstr, "reading annual maximum data for line %d, "
                    "index %s", iline, indx_str[indx]);
                ERROR(errstr, "main");
            }
        }

        /* Run the predictions for the current line */
        if (!pba.predictModel (iline, output)) {
            sprintf (errstr, "running the probability mappings for line %d",
                iline);
            ERROR(errstr, "main");
        }
    }

    cout << second_clock::local_time()
         << " ======= Predict Completed ======== " << endl;

    /* Close the input file and free the structure */
    if (!CloseInput (input))
        ERROR("closing input surface reflectance file", "main");
    if (!FreeInput (input))
        ERROR("freeing input surface reflectance file memory", "main");

    /* Close the output file and free the structure */
    if (!CloseOutput (output))
        ERROR("closing output burned area file", "main");
    if (!FreeOutput (output))
        ERROR("freeing output burned area file memory", "main");

    /* Close the seasonal summaries and annual maximum files */
    for (season = 0; season < PBA_NSEASONS; season++) {
        for (bnd = 0; bnd < PBA_NBANDS; bnd++) {
            if (!CloseRbInput (lySummaryPtr[season][bnd]))
                ERROR("closing input seasonal summary file", "main");
            if (!FreeRbInput (lySummaryPtr[season][bnd]))
                ERROR("freeing input seasonal summary file", "main");
        }
    }
    for (indx = 0; indx < PBA_NINDXS; indx++) {
        if (!CloseRbInput (maxIndxPtr[indx]))
            ERROR("closing input annual maximum file", "main");
        if (!FreeRbInput (maxIndxPtr[indx]))
            ERROR("freeing input annual maximum file", "main");
    }

    /* Release the data arrays */
    pba.predMat.release();
    pba.qaMat.release();
    pba.lySummaryMat.release();
    pba.maxIndxMat.release();

    exit (EXIT_SUCCESS);
};
