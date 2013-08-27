/*
 * FileIO.cpp
 *
 *  Created on: Dec 7, 2012
 *      Author: jlriegle
 *
 *      Reads Config file
 */

#include "PredictBurnedArea.h"

#include <boost/program_options.hpp>
#include<boost/lexical_cast.hpp>

namespace po = boost::program_options;
using namespace boost;

bool PredictBurnedArea::loadParametersFromFile(int ac,char* av[]) {
    string config_filename;            /* configuration filename */
    char errmsg[MAX_STR_LEN];          /* error message */

    po::options_description cmd_line("Command-line options");
    cmd_line.add_options()
        ("config_file", po::value<string>(), "configuration file")
        ("verbose", "print extra processing information (default is off)")
        ("help", "produce help message");

    po::options_description config("Configuration file parameters");
    config.add_options()
        ("INPUT_HDF_FILE", po::value<string>(),
            "input surface reflectance hdf filename")
        ("SEASONAL_SUMMARIES_DIR", po::value<string>(),
            "seasonal summaries directory")
        ("OUTPUT_HDF_FILE", po::value<string>(), "output hdf filename")
        ("INPUT_HEADER_FILE", po::value<string>(), "input header filename")
        ("OUTPUT_HEADER_FILE", po::value<string>(), "output header filename")
        ("OUTPUT_TIFF_FILE", po::value<string>(), "output tiff filename")

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

    if (config_vm.count("LOAD_MODEL_XML")) {
        LOAD_MODEL_XML = config_vm["LOAD_MODEL_XML"].as<string>();
        load_model = true;
    }
    else {
        load_model = false;
    }

    /* Prediction related inputs */
    if (config_vm.count("INPUT_HDF_FILE")) {
        INPUT_HDF_FILE = config_vm["INPUT_HDF_FILE"].as<string>();
        predict_model = true;
    }
    else {
        predict_model = false;
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

    if (config_vm.count("OUTPUT_HDF_FILE")) {
        OUTPUT_HDF_FILE = config_vm["OUTPUT_HDF_FILE"].as<string>();
    }
    else if (predict_model) {
        sprintf (errmsg, "OUTPUT_HDF_FILE is a required config file parameter "
            "for model prediction. Use predict_burned_area --help for more "
            "information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("INPUT_HEADER_FILE")) {
        INPUT_HEADER_FILE = config_vm["INPUT_HEADER_FILE"].as<string>();
    }
    else if (predict_model) {
        sprintf (errmsg, "INPUT_HEADER_FILE is a required config file "
            "parameter for model prediction. Use predict_burned_area --help "
            "for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("OUTPUT_HEADER_FILE")) {
        OUTPUT_HEADER_FILE = config_vm["OUTPUT_HEADER_FILE"].as<string>();
    }
    else if (predict_model) {
        sprintf (errmsg, "OUTPUT_HEADER_FILE is a required config file "
            "parameter for model prediction. Use predict_burned_area --help "
            "for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    if (config_vm.count("OUTPUT_TIFF_FILE")) {
        OUTPUT_TIFF_FILE = config_vm["OUTPUT_TIFF_FILE"].as<string>();
    }
    else if (predict_model) {
        sprintf (errmsg, "OUTPUT_TIFF_FILE is a required config file "
            "parameter for model prediction. Use predict_burned_area --help "
            "for more information.");
        RETURN_ERROR (errmsg, "loadParametersFromFile", false);
    }

    /* Training related inputs */
    if (config_vm.count("CSV_FILE")) {
        CSV_FILE = config_vm["CSV_FILE"].as<string>();
        train_model = true;
    }
    else {
        train_model = false;
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
    if (config_vm.count("SAVE_MODEL_XML")) {
        SAVE_MODEL_XML = config_vm["SAVE_MODEL_XML"].as<string>();
        save_model = true;
    }
    else {
        save_model = false;
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

//Reads the header file for extent and projection information, used to create geoTiff

bool PredictBurnedArea::readHDR(string filename) {
    string instring;

    string variable_name;
    string variable_value;
    size_t i;
    std::ifstream header_file(filename.c_str());
    int samples = 0;
    int lines = 0;
    string mapInfo;
    vector<string> vMapInfo;

    while (!header_file.eof()) {
        getline(header_file,instring);
        i = instring.find_first_of("=");
        if (i >= 0) {
            variable_name = instring.substr(0,i);
            variable_value = instring.substr(i+2,instring.length() - i);
            if (variable_name == "samples ") {
                samples = boost::lexical_cast<int>(variable_value);
            } else if (variable_name == "lines   ") {
                lines = boost::lexical_cast<int>(variable_value);
            } else if (variable_name == "map info ") {
                mapInfo = variable_value;
            }
        }

        stringstream mi(mapInfo);
        string j;
        while (mi >> j) {
            vMapInfo.push_back(j.substr(0,j.size()-1));
        }
    }
    char chars[] = "-";

    projection = vMapInfo[0].substr(1);
    ulx = boost::lexical_cast<float>(vMapInfo[3]);
    uly = boost::lexical_cast<float>(vMapInfo[4]);
    lrx = ulx + samples * boost::lexical_cast<float>(vMapInfo[5]);
    lry = uly - lines * boost::lexical_cast<float>(vMapInfo[6]);
    zone =  vMapInfo[7];
    datum = vMapInfo[9];
    datum.erase(remove(datum.begin(), datum.end(), chars[0]), datum.end());

    return true;
}
