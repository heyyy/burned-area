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

NOTES:
******************************************************************************/

#include <time.h>
#include <sys/time.h>
#include "error.h"
#include "input.h"
#include "input_gtiff.h"
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

NOTES:
  1. predict_burned_area --help will provide input information.
  2. This code is a mixture of true object-oriented C++ code and
     traditional C-based code (error handling, HDF file read/write)
******************************************************************************/
int main(int argc, char* argv[]) {
    PredictBurnedArea pba;
    unsigned int rand_time;            /* "random" time for the filename */
    int bnd;                           /* band/index looping variable */
    int season;                        /* season looping variable */
    int indx;                          /* indices looping variable */
    int ib;                            /* band and line counters */
    int acq_year;                      /* acquisition year of input scene */
    char errstr[MAX_STR_LEN];          /* error string */
    char gdal_cmd[MAX_STR_LEN];        /* command string for GDAL merge call */
    char rm_cmd[MAX_STR_LEN];          /* command string for removing temp
                                          files */
    char lyResizeFile[MAX_STR_LEN];    /* filename of resized image file */
    char maxResizeFile[MAX_STR_LEN];   /* filename of resized image file */
    char hdf_grid_name[MAX_STR_LEN];   /* name of the grid for HDF-EOS */
    char *output_header_name = NULL;   /* output filename */
    char *input_header_name = NULL;    /* output filename */
    char *output_file_name = NULL;     /* output filename */
    char sds_names[NBAND_MAX_OUT][MAX_STR_LEN]; /* array of image SDS names */
    char lySummaryFile[PBA_NSEASONS][PBA_NBANDS][MAX_STR_LEN];/* last year */
    char maxIndxFile[PBA_NINDXS][MAX_STR_LEN];                /* max indices */
    char timestr[MAX_STR_LEN];         /* random string to create random
                                          filenames for resizing */
    Input_t *input = NULL;             /* input data and metadata */
    Output_t *output = NULL;           /* output structure and metadata */
    Input_Gtif_t *lySummaryPtr[PBA_NSEASONS][PBA_NBANDS];  /* last year ptr */
    Input_Gtif_t *maxIndxPtr[PBA_NINDXS];                  /* max indices ptr */
    Space_def_t space_def;             /* spatial definition information */
    int out_sds_types[NBAND_MAX_OUT] = {DFNT_INT16}; /* array of image SDS
                                          types */
    struct timeval curr_time;          /* current time for unique filename */

    /* Read the config file */
    if (!pba.loadParametersFromFile (argc, argv)) {
        /* error message already printed in loadParametersFromFile so just
           exit */
        exit (EXIT_FAILURE);
    }
    char* hdfFile = (char *) pba.INPUT_HDF_FILE.c_str();
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
            cout << "  Input surface reflectance file: " << hdfFile << endl;
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

    /* Open the input HDF file */
    input = OpenInput (hdfFile);
    if (input == (Input_t *) NULL) {
        sprintf (errstr, "opening the input HDF file: %s", hdfFile);
        ERROR(errstr, "main");
    }
    pba.LNDSR_FILL = input->meta.fill;

    /* Call gdalinfo to obtain some metadata for the input HDF file */
#ifdef DEBUG
    string gdalinfo = "gdalinfo";
    const char* writeCommand2 = (gdalinfo + " " + hdfFile);
    system(writeCommand2);
#endif

    /* Print some input metadata info */
    if (pba.VERBOSE) {
        cout << "Number of input reflective bands: " << input->nband
             << endl;
        cout << "Number of input QA bands: " << input->nqa_band << endl;
        cout << "Number of input thermal bands: " << 1 << endl;
        cout << "Number of input lines: " << input->size.l << endl;
        cout << "Number of input samples: " << input->size.s << endl;
        cout << "Provider is " << input->meta.provider << endl;
        cout << "Satellite is " << input->meta.sat << endl;
        cout << "Instrument is " << input->meta.inst << endl;
        cout << "WRS system is " << input->meta.wrs_sys << endl;
        cout << "Path is " << input->meta.path << endl;
        cout << "Row is " << input->meta.row << endl;
        cout << "Acquisition date is "
             << input->meta.acq_date.year << "-" << input->meta.acq_date.month
             << "-" << input->meta.acq_date.day << endl;
        cout << "Fill value is " << input->meta.fill << endl;
        for (ib = 0; ib < input->nband; ib++) {
            cout << "Band -->" << ib << endl;
            cout << "  SDS name is " << input->sds[ib].name << endl;
            cout << "  SDS rank: " << input->sds[ib].rank << endl;
        }
        for (ib = 0; ib < input->nqa_band; ib++) {
            cout << "QA Band -->" << ib << endl;
            cout << "  SDS name is " << input->qa_sds[ib].name << endl;
            cout << "  SDS rank: " << input->qa_sds[ib].rank << endl;
        }
        cout << "Thermal Band -->" << endl;
        cout << "  SDS name is " << input->therm_sds.name << endl;
        cout << "  SDS rank: " << input->therm_sds.rank << endl;
    }

    /* Pull the acquisition year from the acquisition date */
    acq_year = input->meta.acq_date.year;

    /* Create and open output file */
    output_file_name = strdup(pba.OUTPUT_HDF_FILE.c_str());
    input_header_name = strdup(pba.INPUT_HEADER_FILE.c_str());
    output_header_name = strdup(pba.OUTPUT_HEADER_FILE.c_str());
    if (!CreateOutput (output_file_name, input_header_name,
        output_header_name)) {
        sprintf(errstr, "creating output file - %s", output_file_name);
        ERROR(errstr, "main");
    }

    strcpy (sds_names[0], "burn_probability_mapping");
    output = OpenOutput (output_file_name, NBAND_MAX_OUT, sds_names,
        &input->size);
    if (output == NULL) {
        sprintf (errstr, "opening output file: %s", output_file_name);
        ERROR(errstr, "main");
    }

    /* Read the input header file to get the geographic extents */
    cout << second_clock::local_time() <<
        " ======Resizing Input Seasonal and Annual Data=====" << endl;
    pba.readHDR (pba.INPUT_HEADER_FILE);

    /* Create the seasonal summary and annual max directories in the local
       directory to be used for resizing the imagery, if they don't already
       exist */
    if (access ("refl", F_OK) == -1) {
        printf ("Making directory 'refl' ...\n");
        if (mkdir ("refl", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
            ERROR("error making 'refl' directory with write access", "main");
    }
    for (indx = 0; indx < PBA_NINDXS; indx++) {
        if (access (indx_str[indx], F_OK) == -1) {
            printf ("Making directory '%s' ...\n", indx_str[indx]);
            if (mkdir (indx_str[indx], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
                == -1) {
                sprintf (errstr, "error making '%s' directory with write "
                    "access", indx_str[indx]);
                ERROR(errstr, "main");
            }
        }
    }

    /* Create the filenames for the seasonal summmaries and annual maximums.
       Files are expected to reside in the seasonal summaries directory with
       subdirectories of refl, ndvi, ndmi, nbr, nbr2.  Resize these files to
       match the geospatial extents of the current scene.  Then open the files
       and read the associated metadata.
       The resized files will be given a hexidecimal timestamp to allow for
       somewhat unique filenames to be processed. */
    gettimeofday (&curr_time, NULL);
    rand_time = curr_time.tv_sec * 1000000 + curr_time.tv_usec;
    sprintf (timestr, "%x", rand_time);
printf ("DEBUG: random string %s\n", timestr);
    printf (".... Seasonal summary products\n");
    for (season = 0; season < PBA_NSEASONS; season++) {
        for (bnd = 0; bnd < PBA_NBANDS; bnd++) {
            if (bnd < BND_NDVI) {  /* reflectance bands */
                /* Set up the filenames */
                sprintf (lySummaryFile[season][bnd], "%s/refl/%d_%s_%s.tif",
                    seasonalSummaryDir, acq_year-1, season_str[season],
                    band_indx_str[bnd]);
                sprintf (lyResizeFile, "./refl/resize_%d_%s_%s_%s.tif",
                    acq_year-1, season_str[season], band_indx_str[bnd],
                    timestr);
            }
            else {  /* index bands */
                /* Set up the filenames */
                sprintf (lySummaryFile[season][bnd], "%s/%s/%d_%s_%s.tif",
                    seasonalSummaryDir, band_indx_str[bnd], acq_year-1,
                    season_str[season], band_indx_str[bnd]);
                sprintf (lyResizeFile, "./%s/resize_%d_%s_%s_%s.tif",
                    band_indx_str[bnd], acq_year-1, season_str[season],
                    band_indx_str[bnd], timestr);
            }

            /* Resize the files */
            sprintf (gdal_cmd, "gdal_translate -of Gtiff -a_nodata %d "
                "-projwin %f %f %f %f -q %s %s", pba.LNDSR_FILL,
                pba.ulx, pba.uly, pba.lrx, pba.lry, lySummaryFile[season][bnd],
                lyResizeFile);
            if (system (gdal_cmd) == -1) {
                sprintf (errstr, "error running gdal_translate: %s", gdal_cmd);
                ERROR(errstr, "main");
            }

            /* Open the resized files */
            lySummaryPtr[season][bnd] = OpenGtifInput (lyResizeFile);
            if (lySummaryPtr[season][bnd] == NULL) {
                sprintf (errstr, "opening file: %s", lyResizeFile);
                ERROR (errstr, "main");
            }
        }
    }

    printf (".... Annual maximum products\n");
    for (indx = 0; indx < PBA_NINDXS; indx++) {
        /* Set up the filenames - annual max is for last year */
        sprintf (maxIndxFile[indx], "%s/%s/%d_maximum_%s.tif",
            seasonalSummaryDir, indx_str[indx], acq_year-1, indx_str[indx]);
        sprintf (maxResizeFile, "./%s/resize_%d_maximum_%s_%s.tif",
            indx_str[indx], acq_year-1, indx_str[indx], timestr);

        /* Resize the files */
        sprintf (gdal_cmd, "gdal_translate -of Gtiff -a_nodata %d "
            "-projwin %f %f %f %f -q %s %s", pba.LNDSR_FILL, pba.ulx, pba.uly,
            pba.lrx, pba.lry, maxIndxFile[indx], maxResizeFile);
        if (system (gdal_cmd) == -1) {
            sprintf (errstr, "error running gdal_translate: %s", gdal_cmd);
            ERROR(errstr, "main");
        }

        /* Open the resized files */
        maxIndxPtr[indx] = OpenGtifInput (maxResizeFile);
        if (maxIndxPtr[indx] == NULL) {
            sprintf (errstr, "opening file: %s", maxResizeFile);
            ERROR (errstr, "main");
        }
    }

    /* Set up arrays for the seasonal summaries and annual maximums */
    pba.lySummaryMat.create (input->size.s, PBA_NBANDS*PBA_NSEASONS, CV_32FC1);
    pba.maxIndxMat.create (input->size.s, PBA_NINDXS, CV_32FC1);

    /* Set up arrays for the predicted data, cloud, cloud shadow, and
       land/water data.  These will hold a single line and single/multiple
       bands, depending on what is being represented.  For predMat (predicted
       matrix), bands 0-5 are the reflective bands (1-5, and 7), 6=NDVI, 7=NDMI,
       8=NBR, 9=NBR2.  cloudMat represents the cloud_QA band.  cloudShadMat
       represents the cloud_shadow_QA.  landWaterMat represents the
       land_water_QA band.  fillMat represents the fill_QA band. */
    pba.predMat.create (input->size.s, 10, CV_32FC1);
    pba.fillMat.create (input->size.s, 1, CV_8U);
    pba.cloudMat.create (input->size.s, 1, CV_8U);
    pba.cloudShadMat.create (input->size.s, 1, CV_8U);
    pba.landWaterMat.create (input->size.s, 1, CV_8U);

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
                sprintf (errstr, "reading input image data for line %d, "
                    "band %d", 0, 1);
                ERROR(errstr, "main");
            }
        }

        /* Read the seasonal summaries for the previous year */
        for (bnd = 0; bnd < PBA_NBANDS; bnd++) {
            for (season = 0; season < PBA_NSEASONS; season++) {
                if (!pba.GetGtifInputLYSummaryData (lySummaryPtr[season][bnd],
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
            if (!pba.GetGtifInputAnnualMaxData (maxIndxPtr[indx], iline,
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

    /* Write the output metadata for the HDF file */
    if (!PutMetadata (output, NBAND_MAX_OUT, sds_names, &input->meta))
        ERROR("writing metadata to output HDF file", "main");

    /* Close the input file and free the structure */
    if (!CloseInput (input))
        ERROR("closing input HDF file", "main");
    if (!FreeInput (input))
        ERROR("freeing input HDF file memory", "main");

    /* Close the output file and free the structure */
    if (!CloseOutput (output))
        ERROR("closing output HDF file", "main");
    if (!FreeOutput (output))
        ERROR("freeing output HDF file memory", "main");

    for (season = 0; season < PBA_NSEASONS; season++) {
        for (bnd = 0; bnd < PBA_NBANDS; bnd++)
            CloseGtifInput (lySummaryPtr[season][bnd]);
    }
    for (indx = 0; indx < PBA_NINDXS; indx++)
        CloseGtifInput (maxIndxPtr[indx]);

    /* Release the data arrays */
    pba.predMat.release();
    pba.fillMat.release();
    pba.cloudMat.release();
    pba.cloudShadMat.release();
    pba.landWaterMat.release();
    pba.lySummaryMat.release();
    pba.maxIndxMat.release();

    /* Get the projection and spatial information from the input surface
       reflectance product */
    strcpy (hdf_grid_name, "Grid");
    if (!GetSpaceDefHdf (&space_def, hdfFile, hdf_grid_name))
        ERROR("reading spatial metadata from input HDF file", "main");

    /* Write the spatial information to the output HDF file */
    if (!PutSpaceDefHdf (&space_def, output_file_name, NBAND_MAX_OUT,
        sds_names, out_sds_types, hdf_grid_name))
        ERROR("writing spatial metadata to output HDF file", "main");

    /* Clean up the resized files */
    for (indx = 0; indx < PBA_NINDXS; indx++) {
        sprintf (rm_cmd, "rm ./%s/resize_*%s.tif", indx_str[indx], timestr);
        if (system (rm_cmd) == -1) {
            sprintf (errstr, "error removing temp resize files: %s", rm_cmd);
            ERROR(errstr, "main");
        }
    }

    sprintf (rm_cmd, "rm ./refl/resize_*%s.tif", timestr);
    if (system (rm_cmd) == -1) {
        sprintf (errstr, "error removing temp resize files: %s", rm_cmd);
        ERROR(errstr, "main");
    }

    exit (EXIT_SUCCESS);
};
