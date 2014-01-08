#! /usr/bin/env python
import sys
import os
import re
import datetime
import time
import numpy
import zipfile
from model_hash import get_model_name
from argparse import ArgumentParser
from process_temporal_ba_stack import temporalBAStack
from generate_boosted_regression_config import BoostedRegressionConfig
from do_boosted_regression import BoostedRegression
from do_threshold_stack import BurnAreaThreshold
from do_annual_burn_summaries import AnnualBurnSummary
from do_spectral_indices import SpectralIndices

ERROR = 1
SUCCESS = 0

def logIt (msg, log_handler):
    """Logs the user-specified message.
    logIt logs the information to the logfile (if valid) or to stdout if the
    logfile is None.
    
    Args:
      msg - message to be printed/logged
      log_handler - log file handler; if None then print to stdout
    
    Returns: nothing
    """

    if log_handler is None:
        print msg
    else:
        log_handler.write (msg + '\n')


#############################################################################
# Created on December 5, 2013 by Gail Schmidt, USGS/EROS
# Created Python script to run the burned area algorithms (end-to-end) based
#     on a temporal stack of input surface reflectance products.
#
# History:
#
# Usage: do_burned_area.py --help prints the help message
############################################################################
class BurnedArea():
    """Class for handling the burned area end-to-end processing for a
       path/row temporal stack of surface reflectance products.
    """

    def __init__(self):
        pass

    def runBurnedArea(self, sr_list_file=None, input_dir=None,  \
        output_dir=None, model_dir=None, logfile=None):
        """Runs the burned area processing from end-to-end for a given
           stack of surface reflectance products.
        Description: Reads the HDF list file to determine the path/row and
            start/end year of data to be processed for burned area.  The
            seasonal summaries and annual maximums will be generated for the
            stack.  Then, for each HDF file in the stack, the boosted
            regression algorithm will be run to determine the burn
            probabilities.  The boosted regression algorithm needs the
            seasonal summaries and annual maximums for the previous year, so
            processing of the individual scenes will start at start_year+1.
            Next the burn classifications will be processed for each scene,
            followed by the annual summaries for the maximum burn probability,
            DOY when the burn scar first appeared, number of times an area
            was burned, etc.  Lastly the annual summary burned area products
            will be zipped up into one file to be delivered.

        History:
          Created on December 5, 2013 by Gail Schmidt, USGS/EROS LSRD Project

        Args:
          sr_list_file - input file listing the surface reflectance scenes
              to be processed for a single path/row. Each scene to be
              processed should be listed on a separate line. This file is
              only used to determine the path/row and the starting/ending
              dates of the stack.  It is not used for further processing
              beyond that.  Filenames should include directory names.
          input_dir - location of the input stack of scenes to process
          output_dir - location to write the output burned area products
          model_dir - location of the geographic models for the boosted
              regression algorithm
          logfile - name of the logfile for logging information; if None then
              the output will be written to stdout
        
        Returns:
            ERROR - error running the burned area applications
            SUCCESS - successful processing

        Algorithm:
            1. Parse the path/row from the input HDF list
            2. Parse the start and end dates from the HDF list
            3. Process the seasonal summaries for the entire stack
            4. Run the boosted regression algorithm for each scene in the stack
            5. Process spectral indices for each scene in the stack
            6. Run the burn threshold classification for the entire stack
            7. Run the annual burn summaries
            8. Zip the annual burn summaries
        """

        # if no parameters were passed then get the info from the command line
        if sr_list_file is None:
            # get the command line argument for the input parameters
            parser = ArgumentParser(  \
                description='Run burned area processing for the input '  \
                    'temporal stack of surface reflectance products')
            parser.add_argument ('-s', '--sr_list_file', type=str,
                dest='sr_list_file',
                help='input file, each row contains the full pathname of '  \
                     'surface reflectance products to be processed',
                metavar='FILE', required=True)
            parser.add_argument ('-i', '--input_dir', type=str,
                dest='input_dir',
                help='input directory, location of input scenes to be '  \
                     'processed',
                metavar='DIR', required=True)
            parser.add_argument ('-o', '--output_dir', type=str,
                dest='output_dir',
                help='output directory, location to write output burned '  \
                     'area products which have been processed',
                metavar='DIR', required=True)
            parser.add_argument ('-m', '--model_dir', type=str,
                dest='model_dir',
                help='input directory, location of the geographic models ' \
                     'for the boosted regression algorithm',
                metavar='DIR', required=True)
            parser.add_argument ('-l', '--logfile', type=str, dest='logfile',
                help='name of optional log file', metavar='FILE')

            options = parser.parse_args()

            # validate command-line options and arguments
            logfile = options.logfile
            sr_list_file = options.sr_list_file
            if sr_list_file is None:
                parser.error ('missing surface reflectance list file '  \
                    'cmd-line argument')
                return ERROR

            input_dir = options.input_dir
            if input_dir is None:
                parser.error ('missing input directory cmd-line argument')
                return ERROR

            output_dir = options.output_dir
            if output_dir is None:
                parser.error ('missing output directory cmd-line argument')
                return ERROR

            model_dir = options.model_dir
            if model_dir is None:
                parser.error ('missing model directory cmd-line argument')
                return ERROR

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile is not None:
            log_handler = open (logfile, 'w', buffering=1)

        # validate options and arguments
        if not os.path.exists(sr_list_file):
            msg = 'Input surface reflectance list file does not exist: ' +  \
                sr_list_file
            logIt (msg, log_handler)
            return ERROR

        if not os.path.exists(input_dir):
            msg = 'Input directory does not exist: ' + input_dir
            logIt (msg, log_handler)
            return ERROR

        if not os.path.exists(output_dir):
            msg = 'Output directory does not exist: %s. Creating ...' % \
                output_dir
            logIt (msg, log_handler)
            os.makedirs(output_dir, 0755)

        if not os.path.exists(model_dir):
            msg = 'Model directory does not exist: ' + model_dir
            logIt (msg, log_handler)
            return ERROR

        # save the current working directory for return to upon error or when
        # processing is complete
        mydir = os.getcwd()
        msg = 'Changing directories for burned area processing: ' +  \
            output_dir
        logIt (msg, log_handler)
        os.chdir (output_dir)

        # start of threshold processing
        start_time = time.time()

        # open and read the input stack of scenes
        text_file = open(sr_list_file, "r")
        sr_list = text_file.readlines()
        text_file.close()
        num_scenes = len(sr_list)
        msg = 'Number of scenes in the list: %d' % num_scenes
        logIt (msg, log_handler)
        if num_scenes == 0:
            msg = 'error reading the list of scenes in ' + sr_list_file
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # loop through the scenes and determine the path/row along with the
        # starting and ending year in the stack
        start_year = 9999
        end_year = 0
        for i in range(num_scenes):
            curr_file = sr_list[i].rstrip('\n')

            # get the scene name from the current file
            # (Ex. lndsr.LT50170391984072XXX07.hdf)
            base_file = os.path.basename(curr_file)
            scene_name = base_file.replace('lndsr.', '')
            scene_name = scene_name.replace('.hdf', '')

            # get the path/row from the first file
            if i == 0:
                path = int(scene_name[3:6])
                row = int(scene_name[6:9])

            # get the year from this file update the start_year and end_year
            # if appropriate
            year = int(scene_name[9:13])
            if year < start_year:
                start_year = year
            if year > end_year:
                end_year = year

        # validate starting and ending year
        if start_year is not None:
            if (start_year < 1984) | (start_year > 2013):
                msg = 'start_year falls outside 1984-2013: %d' % start_year
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR

        if end_year is not None:
            if (end_year < 1984) | (end_year > 2013):
                msg = 'end_year falls outside 1984-2013: %d' % end_year
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR

        if (end_year is not None) & (start_year is not None):
            if end_year < start_year:
                msg = 'end_year (%d) is less than start_year (%d)' %  \
                    (end_year, start_year)
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR

        # information about what we are doing
        msg = 'Processing burned area products for'
        logIt (msg, log_handler)
        msg = '    path/row: %d, %d' % (path, row)
        logIt (msg, log_handler)
        msg = '    years: %d - %d' % (start_year, end_year)
        logIt (msg, log_handler)

        # run the seasonal summaries and annual maximums for this stack
        msg = '\nProcessing seasonal summaries and annual maximums ...'
        status = temporalBAStack().processStack(input_dir=input_dir,  \
            exclude_l1g=True, logfile=logfile)
        if status != SUCCESS:
            msg = 'Error running seasonal summaries and annual maximums'
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # TODO - GAIL update the model hash table
        # determine the model file for this path/row
        model_base_file = get_model_name(path, row)
        model_file = '%s/%s' % (model_dir, model_base_file)
        if not os.path.exists(model_file):
            msg = 'Model file for path/row %d, %d does not exist: %s' %  \
                (path, row, model_file)
            logIt (msg, log_handler)
            return ERROR

        # run the boosted regression algorithm for each scene
        msg = '\nRunning boosted regression for each scene from %d - %d ...' % \
            (start_year+1, end_year)
        logIt (msg, log_handler)
        config_file = 'temp_%03d_%03d.config' % (path, row)
        for i in range(num_scenes):
            curr_file = sr_list[i].rstrip('\n')

            # filter out the start_year scenes since we need the previous
            # year to run the boosted regression algorithm
            base_file = os.path.basename(curr_file)
            scene_name = base_file.replace('lndsr.', '')
            scene_name = scene_name.replace('.hdf', '')
            year = int(scene_name[9:13])
            if year == start_year:
                # skip to the next scene
                continue

            # generate the configuration file for boosted regression
            status = BoostedRegressionConfig().runGenerateConfig(
                config_file=config_file, seasonal_sum_dir=input_dir,
                input_hdf_file=curr_file, output_dir=output_dir,
                model_file=model_file)
            if status != SUCCESS:
                msg = 'Error creating the configuration file for ' + curr_file
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR

            # run the boosted regression, passing the configuration file
            status = BoostedRegression().runBoostedRegression(  \
                config_file=config_file, logfile=logfile)
            if status != SUCCESS:
                msg = 'Error running boosted regression for ' + curr_file
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR

        # clean up the temporary configuration file
        os.remove(config_file)

        # run the burn threshold algorithm to identify burn scars
        stack_file = input_dir + '/hdf_stack.csv'
        status = BurnAreaThreshold().runBurnThreshold(stack_file=stack_file,
            input_dir=output_dir, output_dir=output_dir,
            start_year=start_year+1, end_year=end_year)
        if status != SUCCESS:
            msg = 'Error running burn thresholds'
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # run the algorithm to generate annual summaries for the burn
        # probabilities and burn scars
        bounding_extents_file = input_dir + '/bounding_box_coordinates.csv'
        status = AnnualBurnSummary().runAnnualBurnSummaries(
            stack_file=stack_file, bounding_extents_file=bounding_extents_file,
            bp_dir=output_dir, bc_dir=output_dir, output_dir=output_dir,
            start_year=start_year+1, end_year=end_year)
        if status != SUCCESS:
            msg = 'Error running annual burn summaries'
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # zip the burn area annual summaries
        zip_file = 'burn_scar_%03d_%03d.zip' % (path, row)
        msg = '\nZipping the annual summaries to ' + zip_file
        logIt (msg, log_handler)
        cmdstr = 'zip %s burn_scar_* burn_count_* good_looks_count_* '  \
            'max_burn_prob_*' % zip_file
        os.system(cmdstr)
        if not os.path.exists(zip_file):
            msg = 'Error creating the zip file of all the annual burn ' \
                'summaries: ' + zip_file
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # successful processing
        end_time = time.time()
        msg = '***Total scene processing time = %f hours' %  \
            ((end_time - start_time) / 1440.0)
        logIt (msg, log_handler)
        msg = 'Success running burned area processing'
        logIt (msg, log_handler)
        os.chdir (mydir)
        return SUCCESS

######end of BurnedArea class######

if __name__ == "__main__":
    sys.exit (BurnedArea().runBurnedArea())
