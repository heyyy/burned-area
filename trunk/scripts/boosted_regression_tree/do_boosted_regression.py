#! /usr/bin/env python
import sys
import os
import re
import commands
import datetime
from optparse import OptionParser

ERROR = 1
SUCCESS = 0

############################################################################
# Description: logIt logs the information to the logfile (if valid) or to
# stdout if the logfile is None.
#
# Inputs:
#   msg - message to be printed/logged
#   log_handler - log file handler; if None then print to stdout
#
# Returns: nothing
#
# Notes:
############################################################################
def logIt (msg, log_handler):
    if log_handler == None:
        print msg
    else:
        log_handler.write (msg + '\n')


#############################################################################
# Created on September 3, 2013 by Gail Schmidt, USGS/EROS
# Created Python script to run the boosted regression tree algorithm.
#
# History:
#
# Usage: do_boosted_regression.py --help prints the help message
############################################################################
class BoostedRegression():

    def __init__(self):
        pass


    ########################################################################
    # Description: runBoostedRegression will use the parameter passed for
    # the input configuration file.  If input config file is None (i.e. not
    # specified) then the command-line parameters will be parsed for this
    # information.  The boosted regression tree application is then executed
    # to run the regression on the specified input surface reflectance file
    # (specified in the input configuration file).  If a log file was
    # specified, then the output from this application will be logged to that
    # file.
    #
    # Inputs:
    #   config_file - name of the input configuration file to be processed
    #   logfile - name of the logfile for logging information; if None then
    #       the output will be written to stdout
    #   usebin - this specifies if the boosted regression tree exe resides
    #       in the $BIN directory; if None then the boosted regression exe
    #       is expected to be in the PATH
    #
    # Returns:
    #     ERROR - error running the boosted regression tree application
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def runBoostedRegression (self, config_file=None, logfile=None, \
        usebin=None):
        # if no parameters were passed then get the info from the command line
        if config_file == None:
            # get the command line argument for the reflectance file
            parser = OptionParser()
            parser.add_option ("-c", "--config_file", type="string",
                dest="config_file",
                help="name of configuration file", metavar="FILE")
            parser.add_option ("--usebin", dest="usebin", default=False,
                action="store_true",
                help="use BIN environment variable as the location of " \
                     "boosted regression tree application")
            parser.add_option ("-l", "--logfile", type="string", dest="logfile",
                help="name of optional log file", metavar="FILE")
            (options, args) = parser.parse_args()
    
            # validate the command-line options
            usebin = options.usebin          # should $BIN directory be used
            logfile = options.logfile        # name of the log file

            # surface reflectance file
            config_file = options.config_file
            if config_file == None:
                parser.error ("missing configuration file command-line argument");
                return ERROR

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile != None:
            log_handler = open (logfile, 'w', buffering=1)
        
        # should we expect the boosted regression application to be in the PATH
        # or in the BIN directory?
        if usebin:
            # get the BIN dir environment variable
            bin_dir = os.environ.get('BIN')
            bin_dir = bin_dir + '/'
            msg = 'BIN environment variable: %s' % bin_dir
            logIt (msg, log_handler)
        else:
            # don't use a path to the boosted regression application
            bin_dir = ""
            msg = 'boosted regression executable expected to be in the PATH'
            logIt (msg, log_handler)
        
        # make sure the configuration file exists
        if not os.path.isfile(config_file):
            msg = "Error: configuration file does not exist or is not " \
                "accessible: " + config_file
            logIt (msg, log_handler)
            return ERROR

        # run boosted regression algorithm, checking the return status.  exit
        # if any errors occur.
        cmdstr = "%spredict_burned_area --config_file %s --verbose" %  \
            (bin_dir, config_file)
        print 'DEBUG: boosted regression command: %s' % cmdstr
        (status, output) = commands.getstatusoutput (cmdstr)
        logIt (output, log_handler)
        exit_code = status >> 8
        if exit_code != 0:
            msg = 'Error running boosted regression. Processing will terminate.'
            logIt (msg, log_handler)
            return ERROR
        
        # successful completion.
        msg = 'Completion of boosted regression.'
        logIt (msg, log_handler)
        if logfile != None:
            log_handler.close()
        return SUCCESS

######end of BoostedRegression class######

if __name__ == "__main__":
    sys.exit (BoostedRegression().runBoostedRegression())
