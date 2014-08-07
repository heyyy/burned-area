#! /usr/bin/env python
import sys
import os

### Error/Success codes ###
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
