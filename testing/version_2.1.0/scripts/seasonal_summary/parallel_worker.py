#! /usr/bin/env python
import multiprocessing, Queue
import time
from log_it import *

#if temporalBAStack is already imported from a higher level script, then
#this import is not needed
#from process_temporal_ba_stack import temporalBAStack
 
class parallelSceneWorker(multiprocessing.Process):
    """Runs the scene resampling in parallel for a stack of scenes.
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
            status = SUCCESS
            status = self.stackObject.sceneResample (xml_file)
            if status != SUCCESS:
                msg = 'Error resampling the surface reflectance bands in ' \
                    'the XML file (%s). Processing will terminate.' % xml_file
                logIt (msg, self.stackObject.log_handler)
 
            # store the result
            self.result_queue.put(status)


class parallelSummaryWorker(multiprocessing.Process):
    """Runs the seasonal summaries in parallel for a temporal stack.
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
                year = self.work_queue.get_nowait()
            except Queue.Empty:
                break
 
            # process the scene
            msg = 'Processing year %d ...' % year
            logIt (msg, self.stackObject.log_handler)
            status = SUCCESS
            status = self.stackObject.generateYearSeasonalSummaries (year)
            if status != SUCCESS:
                msg = 'Error processing seasonal summaries for year %d. '  \
                    'Processing will terminate.' % year
                logIt (msg, self.stackObject.log_handler)
 
            # store the result
            self.result_queue.put(status)


class parallelMaxWorker(multiprocessing.Process):
    """Runs the annual maximums in parallel for a temporal stack.
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
                year = self.work_queue.get_nowait()
            except Queue.Empty:
                break
 
            # process the scene
            msg = 'Processing year %d ...' % year
            logIt (msg, self.stackObject.log_handler)
            status = SUCCESS
            status = self.stackObject.generateYearMaximums (year)
            if status != SUCCESS:
                msg = 'Error processing maximums for year %d. Processing '  \
                    'will terminate.' % year
                logIt (msg, self.stackObject.log_handler)
 
            # store the result
            self.result_queue.put(status)

