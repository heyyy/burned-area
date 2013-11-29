#! /usr/bin/env python
import sys
import os
import re
import commands
import datetime
from optparse import OptionParser

ERROR = 1
SUCCESS = 0

#############################################################################
# Created on September 25, 2013 by Gail Schmidt, USGS/EROS
# Created Python script to generate the configuration files for the boosted
#   regression modeling
#
# History:
#
# Usage: generate_boosted_regression_config.py --help prints the help message
############################################################################
class BoostedRegressionConfig():

    def __init__(self):
        pass


    ########################################################################
    # Description: runGenerateConfig will use the input parameters to generate
    # the configuration file needed for running the boosted regression.  If
    # config file is None (i.e. not specified) then the command-line parameters
    # will be parsed for the information.
    #
    # Inputs:
    #   config_file - name of the configuration file to be created or
    #                 overwritten
    #
    # Returns:
    #     ERROR - error generating the configuration file
    #     SUCCESS - successful creation
    #
    # Notes:
    #######################################################################
    def runGenerateConfig (self, config_file=None, seasonal_sum_dir=None,
        input_hdf_file=None, model_file=None):
        # if no parameters were passed then get the info from the command line
        if config_file == None:
            # get the command line argument for the reflectance file
            parser = OptionParser()
            parser.add_option ("-c", "--config_file", type="string",
                dest="config_file",
                help="name of configuration file", metavar="FILE")
            parser.add_option ("-s", "--seasonal_sum_dir", type="string",
                dest="seasonal_sum_dir",
                help="directory location of the seasonal summaries for this" \
                  "scene", metavar="FILE")
            parser.add_option ("-i", "--input_hdf_file", type="string",
                dest="input_hdf_file",
                help="name of the input HDF file to be processed",
                metavar="FILE")
            parser.add_option ("-m", "--model_file", type="string",
                dest="model_file", help="name of the XML model to load",
                metavar="FILE")
            (options, args) = parser.parse_args()
    
            # validate the input info
            if options.config_file == None:
                parser.error ("missing configuration file command-line " \
                    "argument");
                return ERROR
            config_file = options.config_file

            if options.seasonal_sum_dir == None:
                parser.error ("missing seasonal summary directory " \
                    "command-line argument");
                return ERROR
            seasonal_sum_dir = options.seasonal_sum_dir

            if options.input_hdf_file == None:
                parser.error ("missing the input HDF file command-line " \
                    "argument");
                return ERROR
            input_hdf_file = options.input_hdf_file

            if options.model_file == None:
                parser.error ("missing the model file command-line argument");
                return ERROR
            model_file = options.model_file

        # make sure the seasonal summary directory exists
        if not os.path.isdir(seasonal_sum_dir):
            msg = "Error: seasonal summary directory does not exist or is " \
                "not accessible: " + seasonal_sum_dir
            print msg
            return ERROR

        # make sure the input HDF file exists
        if not os.path.isfile(input_hdf_file):
            msg = "Error: input HDF file does not exist or is not " \
                "accessible: " + input_hdf_file
            print msg
            return ERROR

        # make sure the model file exists
        if not os.path.isfile(model_file):
            msg = "Error: XML model file does not exist or is not " \
                "accessible: " + model_file
            print msg
            return ERROR

        # determine the output filename using the input HDF filename; split
        # the input string into a list where the second element in the list
        # is the scene name for the file.  Example input filename is
        # lndsr.LT50350322002237LGS01.hdf.
        infile_list = input_hdf_file.split('.')
        output_hdf_file = '%s_burn_probability.hdf' % infile_list[1]

        # open the configuration file for writing
        config_handler = open (config_file, 'w')
        if config_handler == None:
            msg = "Error opening/creating the configuration file for write"
            print msg
            return ERROR

        # create the config file
        config_line = 'INPUT_HDF_FILE=%s' % input_hdf_file
        config_handler.write (config_line + '\n')
        config_line = 'INPUT_HEADER_FILE=%s.hdr' % input_hdf_file
        config_handler.write (config_line + '\n')
        config_line = 'SEASONAL_SUMMARIES_DIR=%s' % seasonal_sum_dir
        config_handler.write (config_line + '\n')
        config_line = 'OUTPUT_HDF_FILE=%s' % output_hdf_file
        config_handler.write (config_line + '\n')
        config_line = 'OUTPUT_HEADER_FILE=%s.hdr' % output_hdf_file
        config_handler.write (config_line + '\n')
        config_line = 'LOAD_MODEL_XML=%s' % model_file
        config_handler.write (config_line + '\n')
        config_line = 'OUTPUT_TIFF_FILE=garbage.tiff'
        config_handler.write (config_line + '\n')

        # successful completion
        config_handler.close()
        return SUCCESS

######end of BoostedRegressionConfig class######

if __name__ == "__main__":
    sys.exit (BoostedRegressionConfig().runGenerateConfig())
