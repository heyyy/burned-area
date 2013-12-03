#! /usr/bin/env python
import sys
import os
import re
import commands
import datetime
from argparse import ArgumentParser

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


#######################################################################
# Created on September 3, 2013 by Gail Schmidt, USGS/EROS
#     Created Python script to run the boosted regression tree algorithm.
# 
# History:
# 
# Usage: do_boosted_regression.py --help prints the help message
#######################################################################
class BoostedRegression():
    """Class for handling boosted regression tree processing.
    """

    def __init__(self):
        pass


    def runBoostedRegression (self, config_file=None, logfile=None, \
        usebin=None):
        """Runs the boosted regression algorithm for the specified file.
        Description: runBoostedRegression will use the parameter passed for
        the input configuration file.  If input config file is None (i.e. not
        specified) then the command-line parameters will be parsed for this
        information.  The boosted regression tree application is then executed
        to run the regression on the specified input surface reflectance file
        (specified in the input configuration file).  If a log file was
        specified, then the output from this application will be logged to that
        file.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on Dec. 2, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use argparser vs. optionparser, since optionparser
              is deprecated.
        Args:
          config_file - name of the input configuration file to be processed
          logfile - name of the logfile for logging information; if None then
              the output will be written to stdout
          usebin - this specifies if the boosted regression tree exe resides
              in the $BIN directory; if None then the boosted regression exe
              is expected to be in the PATH
        
        Returns:
            ERROR - error running the boosted regression tree application
            SUCCESS - successful processing
        
        Notes:
          1. The script changes directories to the directory of the
             configuration file.  If absolute paths are not provided in the
             configuration file, then the location of those input/output files
             will need to be the location of the configuration file.
        """

        # if no parameters were passed then get the info from the command line
        if config_file is None:
            # get the command line argument for the reflectance file
            parser = ArgumentParser(  \
                description='Run boosted regression algorithm for the scene')
            parser.add_argument ('-c', '--config_file', type=str,
                dest='config_file',
                help='name of configuration file', metavar='FILE')
            parser.add_argument ('--usebin', dest='usebin', default=False,
                action='store_true',
                help='use BIN environment variable as the location of ' \
                     'boosted regression tree application')
            parser.add_argument ('-l', '--logfile', type=str,
                dest='logfile',
                help='name of optional log file', metavar='FILE')

            options = parser.parse_args()
    
            # validate the command-line options
            usebin = options.usebin          # should $BIN directory be used
            logfile = options.logfile        # name of the log file

            # surface reflectance file
            config_file = options.config_file
            if config_file is None:
                parser.error ('missing configuration file command-line ' \
                    'argument');
                return ERROR

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile is not None:
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
            msg = 'Error: configuration file does not exist or is not ' \
                'accessible: %s' % config_file
            logIt (msg, log_handler)
            return ERROR

        # get the path of the config file and change directory to that location
        # for running this script.  save the current working directory for
        # return to upon error or when processing is complete.  Note: use
        # abspath to handle the case when the filepath is just the filename
        # and doesn't really include a file path (i.e. the current working
        # directory).
        mydir = os.getcwd()
        configdir = os.path.dirname (os.path.abspath (config_file))
        if not os.access(configdir, os.W_OK):
            msg = 'Path of configuration file is not writable: %s.  Boosted ' \
                'regression may need write access to the configuration '  \
                'directory, depending on whether the output files in the '  \
                'configuration file have been specified.' % configdir
            logIt (msg, log_handler)
            return ERROR
        msg = 'Changing directories for boosted regression processing: %s' % \
            configdir
        logIt (msg, log_handler)
        os.chdir (configdir)

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
            os.chdir (mydir)
            return ERROR
        
        # successful completion.  return to the original directory.
        msg = 'Completion of boosted regression.'
        logIt (msg, log_handler)
        if logfile is not None:
            log_handler.close()
        os.chdir (mydir)
        return SUCCESS

######end of BoostedRegression class######

if __name__ == "__main__":
    sys.exit (BoostedRegression().runBoostedRegression())
