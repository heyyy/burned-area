#! /usr/bin/env python
import sys
import os

### Error/Success codes ###
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

