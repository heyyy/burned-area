#! /usr/bin/env python
import sys
import os
import re
import datetime
import time
import numpy
import tempfile
import zipfile
import multiprocessing, Queue
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
# Created on April 10, 2014 by Gail Schmidt, USGS/EROS
# Created Python class to handle the multiprocessing of a stack of scenes.
#
# History:
#
############################################################################
class parallelSceneRegressionWorker(multiprocessing.Process):
    """Runs the boosted regression in parallel for a stack of scenes.
    """
 
    def __init__ (self, work_queue, result_queue, stackObject):
        # base class initialization
        multiprocessing.Process.__init__(self)
 
        # job management stuff
        self.work_queue = work_queue
        self.result_queue = result_queue
        self.stackObject = stackObject
        self.kill_received = False
 

    def run(self):
        while not self.kill_received:
            # get a task
            try:
                xml_file = self.work_queue.get_nowait()
            except Queue.Empty:
                break
 
            # process the scene
            msg = 'Processing %s ...' % xml_file
            logIt (msg, self.stackObject.log_handler)
            status = self.stackObject.sceneBoostedRegression (xml_file)
            if status != SUCCESS:
                msg = 'Error running boosted regression on the XML file ' \
                    '(%s). Processing will terminate.' % xml_file
                logIt (msg, self.stackObject.log_handler)
 
            # store the result
            self.result_queue.put(status)


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

    def sceneBoostedRegression(self, xml_file):
        """Runs the boosted resgression model on the current scene.
        Description: sceneBoostedRegression will run the boosted regression
            model on the current XML file.  A configuration file is created
            for the model run, then the model is run on the current scene.
            The configuration file is removed at the end of processing.

        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 5/7/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to allow for multiprocessing at the scene level.
          Updated on 3/17/2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use the ESPA internal raw binary format
          Updated on 4/10/2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to run as a multi-threaded process.
        
        Args:
          xml_file - name of XML file to process
        
        Returns:
            ERROR - error running the model on the XML file
            SUCCESS - successful processing
        """
   
        # split the xml file into directory and base name
        dir_name = os.path.dirname(xml_file)
        base_name = os.path.basename(xml_file)

        # create a unique config file since these will potentially be
        # processed in parallel.  if the config directory doesn't already
        # exist then create it.
        config_dir = dir_name + '/config'
        if not os.path.exists(config_dir):
            msg = 'Config directory does not exist: %s. Creating ...' %  \
                config_dir
            logIt (msg, self.log_handler)
            os.makedirs(config_dir, 0755)

        temp_file = tempfile.NamedTemporaryFile(mode='w', prefix='temp',
            suffix=self.config_file, dir=config_dir, delete=True)
        temp_file.close()
        config_file = temp_file.name

        # determine the base surface reflectance filename, already been
        # resampled to the maximum extents to match the seasonal summaries
        # and annual maximums
        base_file = dir_name + '/refl/' + base_name.replace('.xml', '')
        mask_file = dir_name + '/mask/' + base_name.replace('.xml', '_mask.img')

        # generate the configuration file for boosted regression
        status = BoostedRegressionConfig().runGenerateConfig(
            config_file=config_file, seasonal_sum_dir=dir_name,
            input_base_file=base_file, input_mask_file=mask_file,
            output_dir=self.output_dir, model_file=self.model_file)
        if status != SUCCESS:
            msg = 'Error creating the configuration file for ' + xml_file
            logIt (msg, self.log_handler)
            return ERROR

        # run the boosted regression, passing the configuration file
        status = BoostedRegression().runBoostedRegression(  \
            config_file=config_file, logfile=self.logfile)
        if status != SUCCESS:
            msg = 'Error running boosted regression for ' + xml_file
            logIt (msg, self.log_handler)
            return ERROR

        # clean up the temporary configuration file
        os.remove(config_file)

        return SUCCESS


    def runBurnedArea(self, sr_list_file=None, input_dir=None,  \
        output_dir=None, model_dir=None, num_processors=1, logfile=None):
        """Runs the burned area processing from end-to-end for a given
           stack of surface reflectance products.
        Description: Reads the XML list file to determine the path/row and
            start/end year of data to be processed for burned area.  The
            seasonal summaries and annual maximums will be generated for the
            stack.  Then, for each XML file in the stack, the boosted
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
          Modified on April 8, 2014 by Gail Schmidt, USGS/EROS LSRD Project
            Updated to process products in the ESPA internal file format vs.
            the previous HDF format

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
          num_processors - how many processors should be used for parallel
              processing sections of the application
          logfile - name of the logfile for logging information; if None then
              the output will be written to stdout
        
        Returns:
            ERROR - error running the burned area applications
            SUCCESS - successful processing

        Algorithm:
            1. Parse the path/row from the input XML list
            2. Parse the start and end dates from the XML list
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
            parser.add_argument ('-p', '--num_processors', type=int,
                dest='num_processors',
                help='how many processors should be used for parallel '  \
                    'processing sections of the application '  \
                    '(default = 1, single threaded)')
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

            # number of processors
            if options.num_processors is not None:
                num_processors = options.num_processors
        else:
            num_processors = num_processors

        # open the log file if it exists; use line buffering for the output
        self.log_handler = None
        self.logfile = logfile
        if logfile is not None:
            self.log_handler = open (logfile, 'w', buffering=1)

        # validate options and arguments
        if not os.path.exists(sr_list_file):
            msg = 'Input surface reflectance list file does not exist: ' +  \
                sr_list_file
            logIt (msg, self.log_handler)
            return ERROR

        if not os.path.exists(input_dir):
            msg = 'Input directory does not exist: ' + input_dir
            logIt (msg, self.log_handler)
            return ERROR

        if not os.path.exists(output_dir):
            msg = 'Output directory does not exist: %s. Creating ...' % \
                output_dir
            logIt (msg, self.log_handler)
            os.makedirs(output_dir, 0755)

        if not os.path.exists(model_dir):
            msg = 'Model directory does not exist: ' + model_dir
            logIt (msg, self.log_handler)
            return ERROR

        # save the current working directory for return to upon error or when
        # processing is complete
        mydir = os.getcwd()
        msg = 'Changing directories for burned area processing: ' +  \
            output_dir
        logIt (msg, self.log_handler)
        os.chdir (output_dir)

        # start of threshold processing
        start_time = time.time()

        # open and read the input stack of scenes
        text_file = open(sr_list_file, "r")
        sr_list = text_file.readlines()
        text_file.close()
        num_scenes = len(sr_list)
        msg = 'Number of scenes in the list: %d' % num_scenes
        logIt (msg, self.log_handler)
        if num_scenes == 0:
            msg = 'error reading the list of scenes in ' + sr_list_file
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # save the output directory for the configuration file usage
        self.output_dir = output_dir

        # loop through the scenes and determine the path/row along with the
        # starting and ending year in the stack
        start_year = 9999
        end_year = 0
        for i in range(num_scenes):
            curr_file = sr_list[i].rstrip('\n')

            # get the scene name from the current file
            # (Ex. LT50170391984072XXX07.xml)
            base_file = os.path.basename(curr_file)
            scene_name = base_file.replace('.xml', '')

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
            if (start_year < 1984):
                msg = 'start_year cannot begin before 1984: %d' % start_year
                logIt (msg, self.log_handler)
                return ERROR

        if end_year is not None:
            if (end_year < 1984):
                msg = 'end_year cannot begin before 1984: %d' % end_year
                logIt (msg, self.log_handler)
                return ERROR

        if (end_year is not None) & (start_year is not None):
            if end_year < start_year:
                msg = 'end_year (%d) is less than start_year (%d)' %  \
                    (end_year, start_year)
                logIt (msg, self.log_handler)
                os.chdir (mydir)
                return ERROR

        # information about what we are doing
        msg = 'Processing burned area products for'
        logIt (msg, self.log_handler)
        msg = '    path/row: %d, %d' % (path, row)
        logIt (msg, self.log_handler)
        msg = '    years: %d - %d' % (start_year, end_year)
        logIt (msg, self.log_handler)

        # run the seasonal summaries and annual maximums for this stack
        msg = '\nProcessing seasonal summaries and annual maximums ...'
        status = temporalBAStack().processStack(input_dir=input_dir,  \
            exclude_l1g=True, logfile=logfile, num_processors=num_processors)
        if status != SUCCESS:
            msg = 'Error running seasonal summaries and annual maximums'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # open and read the stack file generated by the seasonal summaries
        # which excludes the L1G products if any were found
        text_file = open("input_list.txt", "r")
        sr_list = text_file.readlines()
        text_file.close()
        num_scenes = len(sr_list)
        msg = 'Number of scenes in the list after excluding L1Gs: %d' %  \
            num_scenes
        logIt (msg, self.log_handler)
        if num_scenes == 0:
            msg = 'error reading the list of scenes in ' + sr_list_file + \
                ' or no scenes left after excluding L1G products.'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # TODO - GAIL update the model hash table
        # determine the model file for this path/row
        model_base_file = get_model_name(path, row)
        self.model_file = '%s/%s' % (model_dir, model_base_file)
        if not os.path.exists(self.model_file):
            msg = 'Model file for path/row %d, %d does not exist: %s' %  \
                (path, row, self.model_file)
            logIt (msg, self.log_handler)
            return ERROR

       # run the boosted regression algorithm for each scene
        msg = '\nRunning boosted regression for each scene from %d - %d ...' % \
            (start_year+1, end_year)
        logIt (msg, self.log_handler)

        # load up the work queue for processing scenes in parallel for boosted
        # regression
        self.config_file = 'temp_%03d_%03d.config' % (path, row)
        work_queue = multiprocessing.Queue()
        num_boosted_scenes = 0
        for i in range(num_scenes):
            xml_file = sr_list[i].rstrip('\n')

            # filter out the start_year scenes since we need the previous
            # year to run the boosted regression algorithm
            base_file = os.path.basename(xml_file)
            scene_name = base_file.replace('.xml', '')
            year = int(scene_name[9:13])
            if year == start_year:
                # skip to the next scene
                continue

            # add this file to the queue to be processed
            print 'Pushing on the queue ... ' + xml_file
            work_queue.put(xml_file)
            num_boosted_scenes += 1

        # create a queue to pass to workers to store the processing status
        result_queue = multiprocessing.Queue()
 
        # spawn workers to process each scene in the stack - run the boosted
        # regression model on each scene in the stack
        msg = 'Spawning %d scenes for boosted regression via %d '  \
            'processors ....' % (num_boosted_scenes, num_processors)
        logIt (msg, self.log_handler)
        for i in range(num_processors):
            worker = parallelSceneRegressionWorker(work_queue, result_queue,
                self)
            worker.start()
 
        # collect the boosted regression results off the queue
        for i in range(num_boosted_scenes):
            status = result_queue.get()
            if status != SUCCESS:
                msg = 'Error in boosted regression for XML file %s.' %  \
                    sr_list[i]
                logIt (msg, self.log_handler)
                return ERROR

        # run the burn threshold algorithm to identify burn scars
        stack_file = input_dir + '/input_stack.csv'
        status = BurnAreaThreshold().runBurnThreshold(stack_file=stack_file,
            input_dir=output_dir, output_dir=output_dir,
            start_year=start_year+1, end_year=end_year,
            num_processors=num_processors)
        if status != SUCCESS:
            msg = 'Error running burn thresholds'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # run the algorithm to generate annual summaries for the burn
        # probabilities and burn scars
        status = AnnualBurnSummary().runAnnualBurnSummaries(
            stack_file=stack_file, bp_dir=output_dir, bc_dir=output_dir,
            output_dir=output_dir, start_year=start_year+1, end_year=end_year)
        if status != SUCCESS:
            msg = 'Error running annual burn summaries'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # zip the burn area annual summaries
        zip_file = 'burn_scar_%03d_%03d.zip' % (path, row)
        msg = '\nZipping the annual summaries to ' + zip_file
        logIt (msg, self.log_handler)
        cmdstr = 'zip %s burn_scar_* burn_count_* good_looks_count_* '  \
            'max_burn_prob_*' % zip_file
        os.system(cmdstr)
        if not os.path.exists(zip_file):
            msg = 'Error creating the zip file of all the annual burn ' \
                'summaries: ' + zip_file
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # successful processing
        end_time = time.time()
        msg = '***Total scene processing time = %f hours' %  \
            ((end_time - start_time) / 3600.0)
        logIt (msg, self.log_handler)
        msg = 'Success running burned area processing'
        logIt (msg, self.log_handler)
        os.chdir (mydir)
        return SUCCESS

######end of BurnedArea class######

if __name__ == "__main__":
    sys.exit (BurnedArea().runBurnedArea())
