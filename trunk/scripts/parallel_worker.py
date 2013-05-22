#! /usr/bin/env python
import multiprocessing, Queue
import time
from log_it import *
from process_temporal_ba_stack import temporalBAStack
 
class parallelSceneWorker(multiprocessing.Process):
 
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
                hdf_file = self.work_queue.get_nowait()
            except Queue.Empty:
                break
 
            # process the scene
            print 'Processing ' + hdf_file + ' ...'
            status = SUCCESS
            status = self.stackObject.sceneHDFToTiff (hdf_file)
            if status != SUCCESS:
                msg = 'Error converting the HDF file (%s) to GeoTIFF. ' \
                    'Processing will terminate.' % hdf_file
                logIt (msg, self.stackObject.log_handler)
 
            # store the result
            self.result_queue.put(status)


class parallelSummaryWorker(multiprocessing.Process):
 
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
            print 'Processing year ' + str(year) + ' ...'
            status = SUCCESS
            status = self.stackObject.generateYearSeasonalSummaries (year)
            if status != SUCCESS:
                msg = 'Error processing seasonal summaries for year %d. '  \
                    'Processing will terminate.' % hdf_file
                logIt (msg, self.stackObject.log_handler)
 
            # store the result
            self.result_queue.put(status)

