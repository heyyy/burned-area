#! /usr/bin/env python
import sys
import os
import re
import datetime
from argparse import ArgumentParser
from log_it import *


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
        input_base_file=None, input_mask_file=None, output_dir=None,
        model_file=None, logfile=None):
        """Generates the configuration file.
        Description: runGenerateConfig will use the input parameters to
        generate the configuration file needed for running the boosted
        regression.  If config file is None (i.e. not specified) then the
        command-line parameters will be parsed for the information.  If a log
        file was specified, then the output from each application will be
        logged to that file.
        
        History:
          Updated on Dec. 3, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use argparser vs. optionparser, since optionparser
              is deprecated.
          Updated on March 26, 2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to support ESPA internal file format as input and output.
          Updated on April 9, 2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to support the use of a log file.

        Args:
          config_file - name of the configuration file to be created or
              overwritten
          seasonal_sum_dir - name of the directory where the seasonal
              summaries reside for this scene
          input_base_file - name of the base surface reflectance file to be
              processed
          input_mask_file - name of the mask file associated with the base
              surface reflectance file
          output_dir - location of burn probability product to be written
          model_file - name of the geographic model to be used
          logfile - name of the logfile for logging information; if None then
              the output will be written to stdout
       
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
            parser.add_argument ('-i', '--input_base_file', type=str,
                dest='input_base_file',
                help='name of the base input image file to be processed',
                metavar='FILE')
            parser.add_argument ('-k', '--input_mask_file', type=str,
                dest='input_mask_file',
                help='name of the mask file for the image to be processed',
                metavar='FILE')
            parser.add_argument ('-o', '--output_dir', type=str,
                dest='output_dir',
                help='location of burn probability product to be written',
                metavar='DIR')
            parser.add_argument ('-m', '--model_file', type=str,
                dest='model_file', help='name of the XML model to load',
                metavar='FILE')
            parser.add_argument ('-l', '--logfile', type=str, dest='logfile',
                help='name of optional log file', metavar='FILE')

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

            if options.input_base_file is None:
                parser.error ('missing the input base image file '  \
                    'command-line argument');
                return ERROR
            input_base_file = options.input_base_file

            if options.input_mask_file is None:
                parser.error ('missing the input mask file command-line ' \
                    'argument');
                return ERROR
            input_mask_file = options.input_mask_file

            if options.model_file is None:
                parser.error ('missing the model file command-line argument');
                return ERROR
            model_file = options.model_file

            logfile = options.logfile

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile is not None:
            log_handler = open (logfile, 'w', buffering=1)

        # make sure the seasonal summary directory exists
        if not os.path.exists(seasonal_sum_dir):
            msg = 'Error: seasonal summary directory does not exist or is ' \
                'not accessible: %s' % seasonal_sum_dir
            logIt (msg, log_handler)
            return ERROR

        # make sure the input band 1 image file exists, just as a minor
        # sanity check.  It doesn't guarantee that all the bands will be
        # there though.
        if not os.path.exists(input_base_file + '_sr_band1.img'):
            msg = 'Error: input base image file does not exist or is not ' \
                'accessible: %s_sr_band1.img' % input_base_file
            logIt (msg, log_handler)
            return ERROR

        # make sure the mask file exists
        if not os.path.exists(input_mask_file):
            msg = 'Error: input mask file does not exist or is not ' \
                'accessible: %s' % input_mask_file
            logIt (msg, log_handler)
            return ERROR

        # make sure the model file exists
        if not os.path.exists(model_file):
            msg = 'Error: XML model file does not exist or is not ' \
                'accessible: %s' % model_file
            logIt (msg, log_handler)
            return ERROR

        # make sure the output directory exists
        if not os.path.exists(output_dir):
            msg = 'Error: output directory does not exist or is not ' \
                'accessible: %s' % output_dir
            logIt (msg, log_handler)
            return ERROR

        # determine the output filename using the input image filename; split
        # the input string into a list where the second element in the list
        # is the scene name for the file.  Example input filename is
        # LT50350322002237LGS01.
        base_file = os.path.basename(input_base_file)
        output_file = '%s/%s_burn_probability.img' % (output_dir, base_file)

        # open the configuration file for writing
        config_handler = open (config_file, 'w')
        if config_handler is None:
            msg = 'Error opening/creating the configuration file for write'
            logIt (msg, log_handler)
            return ERROR

        # create the config file
        config_line = 'INPUT_BASE_FILE=%s' % input_base_file
        config_handler.write (config_line + '\n')
        config_line = 'INPUT_MASK_FILE=%s' % input_mask_file
        config_handler.write (config_line + '\n')
        config_line = 'INPUT_FILL_VALUE=-9999'
        config_handler.write (config_line + '\n')
        config_line = 'SEASONAL_SUMMARIES_DIR=%s' % seasonal_sum_dir
        config_handler.write (config_line + '\n')
        config_line = 'OUTPUT_IMG_FILE=%s' % output_file
        config_handler.write (config_line + '\n')
        config_line = 'LOAD_MODEL_XML=%s' % model_file
        config_handler.write (config_line + '\n')

        # successful completion
        config_handler.close()
        return SUCCESS

######end of BoostedRegressionConfig class######

if __name__ == "__main__":
    sys.exit (BoostedRegressionConfig().runGenerateConfig())
