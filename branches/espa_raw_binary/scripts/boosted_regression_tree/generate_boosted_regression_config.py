#! /usr/bin/env python
import sys
import os
import re
import datetime
from argparse import ArgumentParser

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
    """Class to handle generating the configuration file for the boosted
    regression processing.
    """

    def __init__(self):
        pass


    def runGenerateConfig (self, config_file=None, seasonal_sum_dir=None,
        input_hdf_file=None, output_dir=None, model_file=None):
        """Generates the configuration file.
        Description: runGenerateConfig will use the input parameters to
        generate the configuration file needed for running the boosted
        regression.  If config file is None (i.e. not specified) then the
        command-line parameters will be parsed for the information.
        
        History:
          Updated on Dec. 3, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use argparser vs. optionparser, since optionparser
              is deprecated.

        Args:
          config_file - name of the configuration file to be created or
              overwritten
          seasonal_sum_dir - name of the directory where the seasonal
              summaries reside for this scene
          input_hdf_file - name of the HDF file to be processed
          output_dir - location of burn probability product to be written
          model_file - name of the geographic model to be used
       
        Returns:
            ERROR - error generating the configuration file
            SUCCESS - successful creation
        """

        # if no parameters were passed then get the info from the command line
        if config_file is None:
            # get the command line argument for the reflectance file
            parser = ArgumentParser(description='Generate the configuration ' \
                'file for boosted regression')
            parser.add_argument ('-c', '--config_file', type=str,
                dest='config_file',
                help='name of configuration file', metavar='FILE')
            parser.add_argument ('-s', '--seasonal_sum_dir', type=str,
                dest='seasonal_sum_dir',
                help='directory location of the seasonal summaries for this ' \
                  'scene', metavar='DIR')
            parser.add_argument ('-i', '--input_hdf_file', type=str,
                dest='input_hdf_file',
                help='name of the input HDF file to be processed',
                metavar='FILE')
            parser.add_argument ('-o', '--output_dir', type=str,
                dest='output_dir',
                help='location of burn probability product to be written',
                metavar='DIR')
            parser.add_argument ('-m', '--model_file', type=str,
                dest='model_file', help='name of the XML model to load',
                metavar='FILE')

            options = parser.parse_args()
    
            # validate the input info
            if options.config_file is None:
                parser.error ('missing configuration file command-line ' \
                    'argument');
                return ERROR
            config_file = options.config_file

            if options.seasonal_sum_dir is None:
                parser.error ('missing seasonal summary directory ' \
                    'command-line argument');
                return ERROR
            seasonal_sum_dir = options.seasonal_sum_dir

            if options.input_hdf_file is None:
                parser.error ('missing the input HDF file command-line ' \
                    'argument');
                return ERROR
            input_hdf_file = options.input_hdf_file

            if options.model_file is None:
                parser.error ('missing the model file command-line argument');
                return ERROR
            model_file = options.model_file

        # make sure the seasonal summary directory exists
        if not os.path.exists(seasonal_sum_dir):
            msg = 'Error: seasonal summary directory does not exist or is ' \
                'not accessible: %s' % seasonal_sum_dir
            print msg
            return ERROR

        # make sure the input HDF file exists
        if not os.path.exists(input_hdf_file):
            msg = 'Error: input HDF file does not exist or is not ' \
                'accessible: %s' % input_hdf_file
            print msg
            return ERROR

        # make sure the model file exists
        if not os.path.exists(model_file):
            msg = 'Error: XML model file does not exist or is not ' \
                'accessible: %s' % model_file
            print msg
            return ERROR

        # make sure the output directory exists
        if not os.path.exists(output_dir):
            msg = 'Error: output directory does not exist or is not ' \
                'accessible: %s' % output_dir
            print msg
            return ERROR

        # determine the output filename using the input HDF filename; split
        # the input string into a list where the second element in the list
        # is the scene name for the file.  Example input filename is
        # lndsr.LT50350322002237LGS01.hdf.
        base_file = os.path.basename(input_hdf_file)
        infile_list = base_file.split('.')
        output_hdf_file = '%s/%s_burn_probability.hdf' %  \
            (output_dir, infile_list[1])

        # open the configuration file for writing
        config_handler = open (config_file, 'w')
        if config_handler is None:
            msg = 'Error opening/creating the configuration file for write'
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
