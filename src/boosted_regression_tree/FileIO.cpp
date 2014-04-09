/*****************************************************************************
FILE: FileIO.cpp
  
PURPOSE: Contains functions for reading the command-line and configuration
file parameters.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date        Programmer       Reason
---------   --------------   -----------------------------------------
12/7/2012   Jodi Riegle      Original development
9/3/2013    Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/

#include "PredictBurnedArea.h"

#include <boost/program_options.hpp>
#include<boost/lexical_cast.hpp>

namespace po = boost::program_options;
using namespace boost;

/******************************************************************************
MODULE: loadParametersFromFile

PURPOSE: Reads the command-line parameters, determines the configuration file
name, and reads the configuration file parameters.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading command-line or config file parameters
true           Successful processing of the parameters

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
12/7/2012     Jodi Riegle      Original development
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment
3/26/2014     Gail Schmidt     Modified to read ESPA internal file format.
                               Also, we now expect the input surface reflectance
                               product to be resampled to the same geographic
                               extents as the seasonal summaries and annual
                               maximums.  We will use the mask file generated
                               as part of the seasonal summaries for this
                               scene.
NOTES:
  1. The following parameters are required for training the model.
     TREE_CNT
     SHRINKAGE
     MAX_DEPTH
     SUBSAMPLE_FRACTION
     CSV_FILE
     NCSV_INPUTS (and this must match the expected value noted in
                  PredictBurnedArea.h)
      
  2. The following parameters are required for loading the model.
     INPUT_BASE_FILE
     INPUT_MASK_FILE
     INPUT_FILL_VALUE
     SEASONAL_SUMMARIES_DIR
     OUTPUT_IMG_FILE
     LOAD_MODEL_XML

  3. If saving the model, after training, then the following parameter is
     required in addition to the training parameters.
     SAVE_MODEL_XML
*****************************************************************************/
bool PredictBurnedArea::loadParametersFromFile(int ac, char* av[]) {
    string config_filename;            /* configuration filename */
    char errmsg[MAX_STR_LEN];          /* error message */

    po::options_description cmd_line("Command-line options");
    cmd_line.add_options()
        ("config_file", po::value<string>(), "configuration file")
        ("verbose", "print extra processing information (default is off)")
        ("help", "produce help message");

    po::options_description config("Configuration file parameters");
    config.add_options()
        ("INPUT_BASE_FILE", po::value<string>(),
            "base filename of the input surface reflectance file (resampled "
            "to the same geographic extents as the seasonal summaries and "
            "annual maximums")
        ("INPUT_MASK_FILE", po::value<string>(),
            "mask file for the input surface reflectance file (resampled "
            "to the same geographic extents as the seasonal summaries and "
            "annual maximums")
        ("INPUT_FILL_VALUE", po::value<int>(),
            "fill value used for the input surface reflectance files")
        ("SEASONAL_SUMMARIES_DIR", po::value<string>(),
            "seasonal summaries directory")
        ("OUTPUT_IMG_FILE", po::value<string>(), "output image filename (.img)")

        /* training related */
        ("SAVE_MODEL_XML", po::value<string>(),
            "specifies to save the model after training to be used for "
            "future prediction runs without the need for retraining "
            "(default is to not save the model)")
        ("LOAD_MODEL_XML", po::value<string>(),
            "specifies to use model from previous training run; the specified "
            "XML file is the name of the previously trained model XML file "
            "(default is to run training)")
        ("TREE_CNT", po::value<int>(),
            "number of trees used for training (i.e. 1000)")
        ("SHRINKAGE", po::value<float>(),
            "shrinkage value for training (i.e. 0.05)")
        ("MAX_DEPTH", po::value<int>(),
            "maximal depth of each decision tree used for training (i.e. 3)")
        ("SUBSAMPLE_FRACTION", po::value<float>(),
            "fraction of input data to be used for training (i.e. 0.50)")
        ("CSV_FILE", po::value<string>(),
            "csv training file; reflectance inputs should be scaled as they "
            "are in the lndsr files; indices should be scaled by 1000 as they "
            "are in the input seasonal summaries")
        ("NCSV_INPUTS", po::value<int>(),
            "number of inputs per line in the training file, not counting "
            "the response index; also the number of inputs used for each "
            "prediction")
        ("PREDICT_OUT", po::value<string>(),
            "output file for training - includes test error, train error and "
            "variables of importance (default is predict_out.txt)");

    po::options_description cmdline_options;
    cmdline_options.add(cmd_line);

    po::options_description config_file_options;
    config_file_options.add(config);

    /* Parse the command-line options */
    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(cmd_line).allow_unregistered().run(), vm);
    notify(vm);
    VERBOSE = false;
    if (vm.count("verbose")) {
        cout << "Verbose mode: ON" << endl;
        VERBOSE = true;
    }

    if (vm.count("help")) {
        cout << cmdline_options;
        cout << config_file_options;
        return false;
    }

    if (vm.count("config_file")) {
        config_filename = vm["config_file"].as<string>();
    }
    else {
        sprintf (errmsg, "config_file is a required command-line parameter. "
            "Use predict_burned_area --help for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    /* Parse the config file options */
    po::variables_map config_vm;
    po::store(po::command_line_parser(ac, av).options(config).allow_unregistered().run(), config_vm);
    notify(config_vm);

    ifstream ifs(config_filename.c_str());
    if (!ifs) {
        sprintf (errmsg, "unable to open config file: %s",
            config_filename.c_str());
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    } else {
        store (parse_config_file (ifs, config_file_options), config_vm);
        notify (config_vm);
    }

    load_model = false;
    if (config_vm.count("LOAD_MODEL_XML")) {
        LOAD_MODEL_XML = config_vm["LOAD_MODEL_XML"].as<string>();
        load_model = true;
    }

    /* Prediction related inputs */
    predict_model = false;
    if (config_vm.count("INPUT_BASE_FILE")) {
        INPUT_BASE_FILE = config_vm["INPUT_BASE_FILE"].as<string>();
        predict_model = true;
    }

    if (config_vm.count("INPUT_MASK_FILE")) {
        INPUT_MASK_FILE = config_vm["INPUT_MASK_FILE"].as<string>();
    }
    else if (predict_model) {
        sprintf (errmsg, "INPUT_MASK_FILE is a required config file "
            "parameter for model prediction. Use predict_burned_area --help "
            "for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("INPUT_FILL_VALUE")) {
        INPUT_FILL_VALUE = config_vm["INPUT_FILL_VALUE"].as<int>();
    }
    else if (predict_model) {
        sprintf (errmsg, "INPUT_FILL_VALUE is a required config file "
            "parameter for model prediction. Use predict_burned_area --help "
            "for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("SEASONAL_SUMMARIES_DIR")) {
        SEASONAL_SUMMARIES_DIR =
            config_vm["SEASONAL_SUMMARIES_DIR"].as<string>();
    }
    else if (predict_model) {
        sprintf (errmsg, "SEASONAL_SUMMARIES_DIR is a required config file "
            "parameter for model prediction. Use predict_burned_area --help "
            "for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("OUTPUT_IMG_FILE")) {
        OUTPUT_IMG_FILE = config_vm["OUTPUT_IMG_FILE"].as<string>();
    }
    else if (predict_model) {
        sprintf (errmsg, "OUTPUT_IMG_FILE is a required config file "
            "parameter for model prediction. Use predict_burned_area --help "
            "for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    /* Training related inputs */
    train_model = false;
    if (config_vm.count("CSV_FILE")) {
        CSV_FILE = config_vm["CSV_FILE"].as<string>();
        train_model = true;
    }

    if (config_vm.count("TREE_CNT")) {
        TREE_CNT = config_vm["TREE_CNT"].as<int>();
    }
    else if (train_model) {
        sprintf (errmsg, "TREE_CNT is a required config file parameter for "
            "training. Use predict_burned_area --help for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("SHRINKAGE")) {
        SHRINKAGE = config_vm["SHRINKAGE"].as<float>();
    }
    else if (train_model) {
        sprintf (errmsg, "SHRINKAGE is a required config file parameter for "
            "training. Use predict_burned_area --help for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("MAX_DEPTH")) {
        MAX_DEPTH = config_vm["MAX_DEPTH"].as<int>();
    }
    else if (train_model) {
        sprintf (errmsg, "MAX_DEPTH is a required config file parameter for "
            "training. Use predict_burned_area --help for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("SUBSAMPLE_FRACTION")) {
        SUBSAMPLE_FRACTION = config_vm["SUBSAMPLE_FRACTION"].as<float>();
    }
    else if (train_model) {
        sprintf (errmsg, "SUBSAMPLE_FRACTION is a required config file "
            "parameter for training. Use predict_burned_area --help for more "
            "information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("PREDICT_OUT")) {
        PREDICT_OUT = config_vm["PREDICT_OUT"].as<string>();
    }
    else if (train_model) {
        PREDICT_OUT = "predict_out.txt";
    }

    /* Read the user-specified number of inputs per training sample in the
       CSV file then verify it matches the expected number of CSV inputs
       for running predictions.  If they don't match, then flag the error.
       The stack of values provided during prediction for each sample needs
       to match the number of values used on input for training the model. */
    if (config_vm.count("NCSV_INPUTS")) {
        NCSV_INPUTS = config_vm["NCSV_INPUTS"].as<int>();
        if (NCSV_INPUTS != EXPECTED_CSV_INPUTS) {
            sprintf (errmsg, "NCSV_INPUTS does not match the "
                "expected/supported number of CSV inputs for training and "
                "prediction. Expected number of CSV inputs (not including "
                "the final classification value) is %d.", EXPECTED_CSV_INPUTS);
            RETURN_ERROR (errmsg, "loadParametersFromFile", false);
        }
    }
    else if (train_model) {
        sprintf (errmsg, "NCSV_INPUTS is a required config file parameter for "
            "training. Use predict_burned_area --help for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }
    else
        NCSV_INPUTS = EXPECTED_CSV_INPUTS;

    /* Inputs for saving the model */
    save_model = false;
    if (config_vm.count("SAVE_MODEL_XML")) {
        SAVE_MODEL_XML = config_vm["SAVE_MODEL_XML"].as<string>();
        save_model = true;
    }

    /* Can't duplicate training the model and loading the model */
    if (load_model && train_model) {
        sprintf (errmsg, "Both the input CSV_FILE for training the model "
            "and the LOAD_MODEL_XML file have been specified.  The model "
            "can only be trained or loaded from an XML file, but not both.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    return true;
}

