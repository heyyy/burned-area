#! /usr/bin/env python
# use true division so we don't have to worry about scalar divided by scalar
# not being a floating point
from __future__ import division
import sys
import os
import re
import commands
import time
import datetime
import csv
import tempfile
import multiprocessing, Queue
from optparse import OptionParser
from HDF_scene import *
from TIF_scene import *
from spectral_indices import *
from spectral_index_from_tif import *
from log_it import *
from parallel_worker import *


#############################################################################
# Created on April 29, 2013 by Gail Schmidt, USGS/EROS
# Created class to hold the methods which process various aspects of the
# temporal stack for burned area processing.
#
# History:
#
# Usage: process_temporal_stack.py --help prints the help message
############################################################################
class temporalBAStack():
    # Data attributes
    input_dir = "None"        # base directory where lndsr products reside
    refl_dir = "None"         # reflective data directory
    ndvi_dir = "None"         # NDVI data directory
    ndmi_dir = "None"         # NDMI data directory
    nbr_dir = "None"          # NBR data directory
    nbr2_dir = "None"         # NBR2 data directory
    mask_dir = "None"         # QA mask data directory
    make_histos = False       # should we make the histograms
    spatial_extent = None     # dictionary for spatial extent corners
    log_handler = None        # file handler for the log file
    num_processors = 1        # default is no parallel processing
    csv_data = None           # CSV data for the stack
    hdf_dir_name = None       # directory of the HDF files in the stack
    nrow = 0                  # number of rows in stack for seasonal summaries
    ncol = 0                  # number of cols in stack for seasonal summaries
    geotrans = None           # geographic trans for seasonal summaries
    prj = None                # geographic projection for seasonal summaries
    nodata = None             # noData value of the HDF files for seasonal summ

    def __init__ (self):
        pass


    ########################################################################
    # Description: generate_stack will determine lndsr files residing in the
    #     input_dir, then write a simple list_file and a more involved
    #     stack_file outlining the files to be processed and some of their
    #     data attributes.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 4/26/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to write a list file as well as the stack file.  Also
    #       added headers for the python functions and support for a log file.
    #
    # Inputs:
    #   input_dir - name of the directory in which to find the lndsr products
    #       to be processed (should be initiated already)
    #   stack_file - name of stack file to create; list of the lndsr products
    #       to be processed in addition to the date, path/row, sensor, bounding
    #       coords, pixel size, and UTM zone
    #   list_file - name of list file to create; simple list of lndsr products
    #       to be processed from the current directory
    #
    # Returns:
    #     ERROR - error generating the stack and list files
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def generate_stack (self, stack_file, list_file):
        # define CSV delimiter
        delim = ','
        
        # check to make sure the output location exists, create the directory
        # if needed
        stack_dirname = os.path.dirname(stack_file)
        if stack_dirname != "" and not os.path.exists (stack_dirname):
            msg = 'Creating directory for output file: ' + stack_dirname
            logIt (msg, self.log_handler)
            os.makedirs (stack_dirname)
    
        list_dirname = os.path.dirname(list_file)
        if list_dirname != "" and not os.path.exists (list_dirname):
            msg = 'Creating directory for output file: ' + list_dirname
            os.makedirs (list_dirname)
    
        # open the output files
        fstack_out = open(stack_file, 'w')
        if not fstack_out:
            msg = 'Could not open stack_file: ' + fstack_out
            logIt (msg, self.log_handler)
            return ERROR
            
        flist_out = open(list_file, 'w')
        if not flist_out:
            msg = 'Could not open list_file: ' + flist_out
            logIt (msg, self.log_handler)
            return ERROR
            
        # write the header for the stack file
        fstack_out.write('file, year, season, month, day, julian, path, ' \
            'row, sensor, west, east, north, south, ncol, nrow, dx, dy, ' \
            'utm_zone\n')
        
        # loop through lndsr*.hdf files in input_directory and gather info
        for f_in in sort(os.listdir(self.input_dir)):
            if f_in.endswith(".hdf") and (f_in.find("lndsr") == 0):
                input_file = self.input_dir + f_in

                # get the attributes from the HDF file
                hdfAttr = HDF_Scene(input_file)

                # determine which season the scene was acquired
                if hdfAttr.month == 12 or hdfAttr.month == 1 or \
                    hdfAttr.month == 2:
                    season = 'winter'
                elif hdfAttr.month >= 3 and hdfAttr.month <= 5:
                    season = 'spring'
                elif hdfAttr.month >= 6 and hdfAttr.month <= 8:
                    season = 'summer'
                else:
                    season = 'fall'
                
                # determine the julian date of the acquisition date
                t = time.mktime( (hdfAttr.year, hdfAttr.month, hdfAttr.day, \
                    0, 0, 0, 0, 0, 0) )
                julian = int(time.strftime("%j", time.gmtime(t)))
                
                # get the projection information, put in quotes so commas in
                # projection string don't confuse .csv readers
                tProjection = hdfAttr.subdataset1.GetProjection()
                t_osr = osr.SpatialReference()
                t_osr.ImportFromWkt(tProjection)
                tUTM = t_osr.GetUTMZone()
            
                # write the stack information for the current lndsr file
                fstack_out.write( input_file + delim + \
                    str(hdfAttr.year) + delim + \
                    season + delim + \
                    str(hdfAttr.month) + delim + \
                    str(hdfAttr.day) + delim + \
                    str(julian) + delim + \
                    str(hdfAttr.WRS_Path) + delim + \
                    str(hdfAttr.WRS_Row) + delim + \
                    hdfAttr.Satellite + delim + \
                    str(hdfAttr.WestBoundingCoordinate) + delim + \
                    str(hdfAttr.EastBoundingCoordinate) + delim + \
                    str(hdfAttr.NorthBoundingCoordinate) + delim + \
                    str(hdfAttr.SouthBoundingCoordinate) + delim + \
                    str(hdfAttr.NRow) + delim + \
                    str(hdfAttr.NCol) + delim + \
                    str(hdfAttr.dX) + delim + \
                    str(hdfAttr.dY) + delim + \
                    str(tUTM) + '\n')
                    #tProjection + '\n')
    
                # write the lndsr file to the output list file
                flist_out.write(input_file + '\n')
    
                # clear the HDF attributes for the next file
                hdfAttr = None
            # end if lndsr*.hdf files
        # end for all files in this directory
    
        # close the output list and stack files
        fstack_out.close()
        flist_out.close()
        return SUCCESS


    ########################################################################
    # Description: stackSpatialExtent will open the input CSV file, which
    #     contains the bounding extents for the current temporal stack,
    #     and returns a dictionary of east, west, north, south extents.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 4/29/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to utilize a log file if passed along.
    #
    # Inputs:
    #   bounding_extents_file - name of file which contains the bounding
    #       extents; first line should identify east, west, north, south
    #       coords; second line should contain the projection coords themselves
    #
    # Returns:
    #     None - error reading the bounding extents
    #     return_dict - dictionary of spatial extents
    #
    # Notes:
    #######################################################################
    def stackSpatialExtent(self, bounding_extents_file):
        # check to make sure the input file exists before opening
        if not os.path.exists (bounding_extents_file):
            msg = 'Bounding extents file does not exist: ' + \
                bounding_extents_file
            logIt (msg, self.log_handler)
            return None

        # open the CSV file, read the column headings in the first line, then
        # read the spatial extents in the second line
        reader = csv.reader (open (bounding_extents_file, 'rb'))
        header = reader.next()  # read the column headings
        values = reader.next()  # read the values

        # return a dictionary of results. strip the leading and ending
        # white space from the header values since they may have white space
        # between the comma separators in the CSV file
        return_dict = dict([
            [header[0].strip(), float(values[0])], \
            [header[1].strip(), float(values[1])], \
            [header[2].strip(), float(values[2])], \
            [header[3].strip(), float(values[3])] \
        ])
        
        return return_dict


    ########################################################################
    # Description: hdf2tif will convert the HDF file to GeoTIFF.  It sets
    #     the noData value to -9999.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 4/30/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Took out the conversion of projection coords (x,y) to
    #           line, sample (i,j) space and vice versa.  This speeds up the
    #           processing time a bit.
    #       Removed some of the redundant tagging of QA as noData.
    #       Removed the redundant histogram and build pyramids code which is
    #           also done by polishImage
    #       Modified to utilize a log file if passed along.
    #
    # Inputs:
    #   input_file - name of the input HDF reflectance file to be converted
    #   output_file - name of the output GeoTIFF file which contains bands 1-7
    #       and a QA band
    #
    # Returns:
    #     ERROR - error converting the HDF file to GeoTIFF
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def hdf2tif(self, input_file, output_file):
        # test to make sure the input file exists
        if not os.path.exists(input_file):
            msg = 'Input file does not exist: ' + input_file
            logIt (msg, self.log_handler)
            return ERROR
    
        # make sure the output directory exists, otherwise create it
        output_dirname = os.path.dirname (output_file)
        if output_dirname != "" and not os.path.exists (output_dirname):
            msg = 'Creating directory for output file: ' + output_dirname
            logIt (msg, self.log_handler)
            os.makedirs (output_dirname)
        
        # open the input file
        hdfAttr = HDF_Scene(input_file, self.log_handler)
        if hdfAttr == None:
            # error message already written in the constructor
            return ERROR
    
        # create an output file with 8 bands which are all int16s and get band
        # pointers to each band. the last band is the QA band.
        num_out_bands = 8
        driver = gdal.GetDriverByName("GTiff")
        output_ds = driver.Create (output_file, hdfAttr.NCol, hdfAttr.NRow, \
            num_out_bands, gdal.GDT_Int16)
        output_ds.SetGeoTransform (hdfAttr.subdataset1.GetGeoTransform())
        output_ds.SetProjection (hdfAttr.subdataset1.GetProjection())
        output_band1 = output_ds.GetRasterBand(1)
        output_band2 = output_ds.GetRasterBand(2)
        output_band3 = output_ds.GetRasterBand(3)
        output_band4 = output_ds.GetRasterBand(4)
        output_band5 = output_ds.GetRasterBand(5)
        output_band6 = output_ds.GetRasterBand(6)
        output_band7 = output_ds.GetRasterBand(7)
        output_band_QA = output_ds.GetRasterBand(8)
    
        # initialize noData value for all bands to -9999, including the QA band
        output_band1.SetNoDataValue(-9999)
        output_band2.SetNoDataValue(-9999)
        output_band3.SetNoDataValue(-9999)
        output_band4.SetNoDataValue(-9999)
        output_band5.SetNoDataValue(-9999)
        output_band6.SetNoDataValue(-9999)
        output_band7.SetNoDataValue(-9999)
        output_band_QA.SetNoDataValue(-9999)
    
        # loop through all the lines in the image, read the line of data from
        # the HDF file, and write back out to GeoTIFF;  the QA band is a
        # combination of all the QA values (negative values flag non-clear
        # pixels and -9999 is the fill pixel).
        for y in range(0, hdfAttr.NRow):
            # read a row of data
            vals = hdfAttr.getLineOfBandValues(y)    
            output_band1.WriteArray (array([vals['band1']]), 0, y)
            output_band2.WriteArray (array([vals['band2']]), 0, y)
            output_band3.WriteArray (array([vals['band3']]), 0, y)
            output_band4.WriteArray (array([vals['band4']]), 0, y)
            output_band5.WriteArray (array([vals['band5']]), 0, y)
            output_band6.WriteArray (array([vals['band6']]), 0, y)
            output_band7.WriteArray (array([vals['band7']]), 0, y)
            output_band_QA.WriteArray (array([vals['QA']]), 0, y)
          
        # build histograms and pyramid overviews if the user specified
        if self.make_histos:
            # create histograms
            histogram = output_band1.GetDefaultHistogram()
            if not histogram == None:
                output_band1.SetDefaultHistogram(histogram[0], histogram[1],  \
                    histogram[3])
            histogram = output_band1.GetDefaultHistogram()
            if not histogram == None:
                output_band2.SetDefaultHistogram(histogram[0], histogram[1],  \
                    histogram[3])
            histogram = output_band2.GetDefaultHistogram()
            if not histogram == None:
                output_band3.SetDefaultHistogram(histogram[0], histogram[1],  \
                    histogram[3])
            histogram = output_band3.GetDefaultHistogram()
            if not histogram == None:
                output_band4.SetDefaultHistogram(histogram[0], histogram[1],  \
                    histogram[3])
            histogram = output_band4.GetDefaultHistogram()
            if not histogram == None:
                output_band5.SetDefaultHistogram(histogram[0], histogram[1],  \
                    histogram[3])
            histogram = output_band5.GetDefaultHistogram()
            if not histogram == None:
                output_band6.SetDefaultHistogram(histogram[0], histogram[1],  \
                    histogram[3])
            histogram = output_band6.GetDefaultHistogram()
            if not histogram == None:
                output_band7.SetDefaultHistogram(histogram[0], histogram[1],  \
                    histogram[3])
            histogram = output_band_QA.GetDefaultHistogram()
            if not histogram == None:
                output_band_QA.SetDefaultHistogram(histogram[0], histogram[1], \
                    histogram[3])  
        
            # build pyramids
            gdal.SetConfigOption('HFA_USE_RRD', 'YES')
            output_ds.BuildOverviews(overviewlist=[3,9,27,81,243,729])  
        
        # cleanup
        vals = None
        output_band1 = None
        output_band2 = None
        output_band3 = None
        output_band4 = None
        output_band5 = None
        output_band6 = None
        output_band7 = None
        output_band_QA = None
        output_ds = None
    
        return SUCCESS


    ########################################################################
    # Description: stackHdfToTiff will convert the HDF files to GeoTIFF and
    #     then resample the GeoTIFF files to the bounding extents.  It also
    #     computes the spectral indices.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 4/29/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to utilize a log file if passed along.
    #       Make the histograms and overviews optional.
    #
    # Inputs:
    #   bounding_extents_file - name of file which contains the bounding
    #       extents
    #   stack_file - name of stack file to create; list of the lndsr products
    #       to be processed in addition to the date, path/row, sensor, bounding
    #       coords, pixel size, and UTM zone
    #
    # Returns:
    #     ERROR - error converting all the HDF files to GeoTIFF and resampling
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def stackHDFToTiff(self, bounding_extents_file, stack_file):
        # read the spatial extents
        self.spatial_extent = self.stackSpatialExtent (bounding_extents_file)
        if self.spatial_extent == None:
            # error message already written
            return ERROR

        # open the stack file and read the header of the stack file
        stack = csv.reader (open (stack_file, 'rb'))
        header_row = stack.next()
        for elem in range (0, len(header_row)):
            header_row[elem] = header_row[elem].strip()

        # define the output directory for each of the resampled and
        # converted files
        self.refl_dir = self.input_dir + "refl/"
        self.ndvi_dir = self.input_dir + "ndvi/"
        self.ndmi_dir = self.input_dir + "ndmi/"
        self.nbr_dir = self.input_dir + "nbr/"
        self.nbr2_dir = self.input_dir + "nbr2/"
        self.mask_dir = self.input_dir + "mask/"

        # make sure each of the output directories exist
        if not os.path.exists (self.refl_dir):
            msg = 'Creating directory for GeoTIFF reflectance files'
            logIt (msg, self.log_handler)
            os.makedirs (self.refl_dir)
        if not os.path.exists (self.ndvi_dir):
            msg = 'Creating directory for GeoTIFF NDVI files'
            logIt (msg, self.log_handler)
            os.makedirs (self.ndvi_dir)
        if not os.path.exists (self.ndmi_dir):
            msg = 'Creating directory for GeoTIFF NDMI files'
            logIt (msg, self.log_handler)
            os.makedirs (self.ndmi_dir)
        if not os.path.exists (self.nbr_dir):
            msg = 'Creating directory for GeoTIFF NBR files'
            os.makedirs (self.nbr_dir)
        if not os.path.exists (self.nbr2_dir):
            msg = 'Creating directory for GeoTIFF NBR2 files'
            logIt (msg, self.log_handler)
            os.makedirs (self.nbr2_dir)
        if not os.path.exists (self.mask_dir):
            msg = 'Creating directory for GeoTIFF mask files'
            logIt (msg, self.log_handler)
            os.makedirs (self.mask_dir)

        # load up the work queue for processing scenes in parallel
        work_queue = multiprocessing.Queue()
        num_scenes = 0
        for scene in enumerate (stack):
            hdf_file = scene[1][header_row.index('file')]
            work_queue.put(hdf_file)
            num_scenes += 1

        # create a queue to pass to workers to store the processing status
        result_queue = multiprocessing.Queue()
 
        # spawn workers to process each scene in the stack - convert from HDF
        # to GeoTIFF, create histograms and pyramids, and calculate the
        # spectral indices
        msg = 'Spawning %d scenes for processing via %d processors ....' %  \
            (num_scenes, self.num_processors)
        logIt (msg, self.log_handler)
        for i in range(self.num_processors):
            worker = parallelSceneWorker(work_queue, result_queue, self)
            worker.start()
 
        # collect the results off the queue
        for i in range(num_scenes):
            status = result_queue.get()
            if status != SUCCESS:
                msg = 'Error converting the HDF file (%d in stack) to ' \
                    'GeoTIFF.' % i
                logIt (msg, self.log_handler)
                return ERROR

        return SUCCESS


    ########################################################################
    # Description: sceneHdfToTiff will convert the HDF file to GeoTIFF and
    #     then resample the GeoTIFF file to the bounding extents.  It will
    #     also compute the spectral indices for the scene.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 5/7/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to allow for multiprocessing at the scene level.
    #
    # Inputs:
    #   hdf_file - name of hdf file to process
    #
    # Returns:
    #     ERROR - error converting all the HDF files to GeoTIFF and resampling
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def sceneHDFToTiff(self, hdf_file):
        startTime0 = time.time()
   
        # generate the HDF, GeoTIFF, and temporary GeoTIFF filename.  use
        # tempfile to get a unique filename and then close it right away.
        # use the filename itself to process the string like we did with
        # just temp.tif.  we have to have unique filenames for running in
        # parallel.
        msg = '############################################################'
        logIt (msg, self.log_handler)
        tif_file = hdf_file.replace('.hdf', '.tif')
        tif_file = self.refl_dir + os.path.basename (tif_file)
        temp_file = tempfile.NamedTemporaryFile(mode='w', prefix='temp',  \
            suffix='.tif', dir=self.refl_dir, delete=True)
        temp_file.close()

        # convert .hdf to .tif
        startTime = time.time()
        msg = '   Converting HDF (%s) to GeoTIFF (%s)' % (hdf_file, \
            temp_file.name)
        logIt (msg, self.log_handler)
        status = self.hdf2tif (hdf_file, temp_file.name)
        if status != SUCCESS:
            msg = 'Error converting the HDF file to GeoTIFF. Processing ' \
                'will terminate.'
            logIt (msg, self.log_handler)
            return ERROR
        endTime = time.time()
        msg = '    Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, self.log_handler)

        # resample the .tif file to our maximum bounding coords
        startTime = time.time()
        msg = '   Resizing temp (%s) to max bounds (%s)' % (temp_file.name, \
            tif_file)
        logIt (msg, self.log_handler)
        cmd = 'gdal_merge.py -o ' + tif_file + ' -co "INTERLEAVE=BAND" -co "TILED=YES" -init -9999 -n -9999 -a_nodata -9999 -ul_lr ' + str(self.spatial_extent['West']) + ' ' + str(self.spatial_extent['North']) + ' ' + str(self.spatial_extent['East']) + ' ' + str(self.spatial_extent['South']) + ' ' + temp_file.name
        msg = '    ' + cmd
        logIt (msg, self.log_handler)
        os.system(cmd)

        endTime = time.time()
        msg = '    Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, self.log_handler)
   
        # remove the temp file since it is no longer needed
        os.remove (temp_file.name)

        # open the tif, calculate histograms for each band in the .tif
        # file, and build pyramids -- if the user specifies this should
        # be done
        if self.make_histos:
            startTime = time.time()
            msg = '    Polishing image ...'
            logIt (msg, self.log_handler)
            status = polishImage (tif_file, self.log_handler)
            if status != SUCCESS:
                msg = 'Error polishing the GeoTIFF file. Processing ' \
                    'will terminate.'
                logIt (msg, self.log_handler)
                return ERROR
            
            endTime = time.time()
            msg = '    Processing time = ' + str(endTime-startTime) +  \
                ' seconds'
            logIt (msg, self.log_handler)

        # calculate ndvi, ndmi, nbr, nbr2 from the converted tif file and
        # create the mask file (basically copy over the QA file)
        startTime = time.time()
        msg = '   Calculating spectral indices...'
        logIt (msg, self.log_handler)
        specIndx = spectralIndex (tif_file, self.log_handler)
        if status != SUCCESS:
            # error message already written by class constructor
            return ERROR

        ndvi_file = self.ndvi_dir + os.path.basename(tif_file)
        ndmi_file = self.ndmi_dir + os.path.basename(tif_file)
        nbr_file = self.nbr_dir + os.path.basename(tif_file)
        nbr2_file = self.nbr2_dir + os.path.basename(tif_file)
        mask_file = self.mask_dir + os.path.basename(tif_file)
        idx_dict = {}
        idx_dict['ndvi'] = ndvi_file
        idx_dict['ndmi'] = ndmi_file
        idx_dict['nbr']  = nbr_file
        idx_dict['nbr2'] = nbr2_file
        idx_dict['mask'] = mask_file
        specIndx.createSpectralIndices (idx_dict, self.make_histos,
            self.log_handler)
        del (idx_dict)
        
        endTime = time.time()
        msg = '    Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, self.log_handler)

        endTime0 = time.time()
        msg = '***Total scene processing time = ' +  \
            str(endTime0 - startTime0) + ' seconds'
        logIt (msg, self.log_handler)
        return SUCCESS


    ########################################################################
    # Description: generateSeasonalSummaries will generate the seasonal
    # summaries for the temporal stack.  If a log file was specified then the
    # output from each application will be logged to that file.
    #
    # Inputs:
    #   stack_file - name of stack file to create; list of the lndsr products
    #       to be processed in addition to the date, path/row, sensor, bounding
    #       coords, pixel size, and UTM zone
    #
    # Returns:
    #     ERROR - error generating the seasonal summaries
    #     SUCCESS - successful processing
    #
    # Notes:
    #   1. Seasons are defined as:
    #      winter = dec (previous year), jan, and feb
    #      spring = mar, apr, may
    #      summer = jun, jul, aug
    #      fall = sep, oct, nov
    #   2. Seasonal summaries are the mean value for each season for bands
    #      3, 4, 5, 7, ndvi, ndmi, nbr, and nbr2.
    #   3. Good count is the number of 'lloks' with no QA flag set
    #######################################################################
    def generateSeasonalSummaries (self, stack_file):
        # make sure the stack file exists
        if not os.path.exists(stack_file):
            msg = 'Could not open stack file: ' + stack_file
            logIt (msg, self.log_handler)
            return ERROR

        # ignore divide by zero and invalid (NaN) values when doing array
        # division.  these will be handled on our own.
        seterr(divide='ignore', invalid='ignore')

        # open and read the stack file
        startTime = time.time()
        f_in = open (stack_file, 'r')
        self.csv_data = recfromcsv (stack_file, delimiter=',', names=True,  \
            dtype="string")
        f_in = None
        if self.csv_data == None:
            msg = 'Error reading the stack file: ' + stack_file
            logIt (msg, self.log_handler)
            return ERROR

        # get the sorted, unique years in the stack; grab the first and last
        # year and use as the range of years to be processed.
        years = unique (self.csv_data['year'])
        start_year = years[0]
        end_year = years[len(years)-1]
        msg = '\nProcessing stack for %d - %d' % (start_year, end_year)
        logIt (msg, self.log_handler)

        # determine the mask file for the first scene listed in the stack
        self.hdf_dir_name = os.path.dirname(self.csv_data['file_'][0])
        first_file = self.csv_data['file_'][0]
        base_file = os.path.basename(first_file.replace('.hdf', '.tif'))
        first_file = '%s/mask/%s' % (self.hdf_dir_name, base_file)

        # open the first file in the stack (GeoTIFF) to get ncols and nrows
        # and other associated info for the stack of scenes
        tifBand1 = TIF_Scene_1_band(first_file, 1, self.log_handler)
        if tifBand1 == None:
            msg = 'Error reading the GeoTIFF file: ' + first_file
            logIt (msg, self.log_handler)
            return ERROR

        self.ncol = tifBand1.NCol
        self.nrow = tifBand1.NRow
        self.geotrans = tifBand1.dataset.GetGeoTransform()
        self.prj = tifBand1.dataset.GetProjectionRef()
        self.nodata = tifBand1.NoData
        tifBand1 = None

        # load up the work queue for processing yearly summaries in parallel
        work_queue = multiprocessing.Queue()
        num_years = end_year - start_year + 1
        for year in range (start_year, end_year+1):
            work_queue.put(year)

        # create a queue to pass to workers to store the processing status
        result_queue = multiprocessing.Queue()
 
        # spawn workers to process each year in the stack - generate the
        # seasonal summaries
        msg = 'Spawning %d years for processing via %d processors ....' %  \
            (num_years, self.num_processors)
        logIt (msg, self.log_handler)
        for i in range(self.num_processors):
            worker = parallelSummaryWorker(work_queue, result_queue, self)
            worker.start()
 
        # collect the results off the queue
        for i in range(num_years):
            status = result_queue.get()
            if status != SUCCESS:
                msg = 'Error processing seasonal summaries for year %d' %  \
                    start_year + i
                logIt (msg, self.log_handler)
                return ERROR

        endTime = time.time()
        msg = 'Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, self.log_handler)
 
        return SUCCESS


    ########################################################################
    # Description: generateYearSeasonalSummaries will generate the seasonal
    # summaries for the current year.  If a log file was specified then the
    # output from each application will be logged to that file.
    #
    # History:
    #   Updated on 5/22/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to process all the indices one line  at a time (vs. the
    #       entire band) since this is faster.
    #   Updated on 9/20/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to process a summary product with fill if there aren't
    #       any valid inputs for the current season/year.
    #
    # Inputs:
    #   year - year to process the seasonal summaries
    #
    # Returns:
    #     ERROR - error generating the seasonal summaries for this year
    #     SUCCESS - successful processing
    #
    # Notes:
    #   1. Seasons are defined as:
    #      winter = dec (previous year), jan, and feb
    #      spring = mar, apr, may
    #      summer = jun, jul, aug
    #      fall = sep, oct, nov
    #   2. Seasonal summaries are the mean value for each season for bands
    #      3, 4, 5, 7, ndvi, ndmi, nbr, and nbr2.
    #   3. Good count is the number of 'lloks' with no QA flag set
    #######################################################################
    def generateYearSeasonalSummaries (self, year):
        # loop through seasons
        last_year = year - 1
        for season in ['winter', 'spring', 'summer', 'fall']:
            # determine which files apply to the current season in the
            # current year
            if season == 'winter':
                season_files =  \
                    ((self.csv_data['year'] == last_year) &  \
                     (self.csv_data['month'] == 12)) |  \
                    ((self.csv_data['year'] == year) &  \
                    ((self.csv_data['month'] == 1) |    \
                     (self.csv_data['month'] == 2)))
            elif season == 'spring':
                season_files = (self.csv_data['year'] == year) &  \
                    ((self.csv_data['month'] >= 3) &   \
                     (self.csv_data['month'] <= 5))
            elif season == 'summer':
                season_files = (self.csv_data['year'] == year) &  \
                    ((self.csv_data['month'] >= 6) &   \
                     (self.csv_data['month'] <= 8))
            elif season=='fall':
                season_files = (self.csv_data['year'] == year) &  \
                    ((self.csv_data['month'] >= 9) &   \
                     (self.csv_data['month'] <= 11))
 
            # how many files do we have for the current year and season?
            # if there aren't any files to process then write out a
            # product with fill
            n_files = sum (season_files)
            msg = '  season = %s,  file count = %d' % (season, n_files)
            logIt (msg, self.log_handler)
 
            # generate the directory name for the mask stack
            dir_name = '%s/mask/' % self.hdf_dir_name
            
            # pull the files for this year and season
            files = self.csv_data['file_'][season_files]
            
            # create the mask datasets -- stack of nrow x ncols
            mask_data = zeros((n_files, self.nrow, self.ncol), dtype=int16)

            # loop through the current set of files, open the mask files,
            # and stack them up in a 3D array
            for i in range(0, n_files):
                temp = files[i]
                base_file = os.path.basename(temp.replace('.hdf', '.tif'))
                mask_file = '%s%s' % (dir_name, base_file)
                mask_dataset = gdal.Open (mask_file, gdalconst.GA_ReadOnly)
                if mask_dataset == None:
                    msg = 'Could not open mask file: ' + mask_file
                    logIt (msg, self.log_handler)
                    return ERROR
                mask_band = mask_dataset.GetRasterBand(1)
                mask_data[i,:,:] = mask_band.ReadAsArray()
                mask_band = None
                mask_dataset = None
            
            # which voxels in the mask have good qa values?
            mask_data_good = mask_data >= 0
            mask_data_bad = mask_data < 0
            mask_data = None
            
            # summarize the number of good pixels in the stack for each
            # line/sample; if there aren't any files for this year and
            # season then just fill with zeros; write data as a byte
            # since there won't be enough total files to go past 256
            msg = '    Generating %d %s good looks using %d '  \
                'files ...' % (year, season, n_files)
            logIt (msg, self.log_handler)
            if n_files > 0:
                good_looks = apply_over_axes(sum, mask_data_good,  \
                    axes=[0])[0,:,:]
            else:
                good_looks = zeros((self.nrow, self.ncol), dtype=uint8)
            
            # save the good looks output to a GeoTIFF file
            good_looks_file = dir_name + str(year) + '_' + season +  \
                '_good_count.tif'
            
            # write the good looks count to an output GeoTIFF file as a
            # byte product.  the noData value for this set will be 0 vs.
            # the traditional nodata value of -9999, since we are working
            # with a byte product.
            driver = gdal.GetDriverByName("GTiff")
            driver.Create(good_looks_file, self.ncol, self.nrow, 1,  \
                gdalconst.GDT_Byte)
            good_looks_dataset = gdal.Open(good_looks_file,  \
                gdalconst.GA_Update)
            if good_looks_dataset is None:
                msg = 'Could not create output file: ', good_looks_file
                logIt (msg, self.log_handler)
                return ERROR
            
            good_looks_dataset.SetGeoTransform(self.geotrans)
            good_looks_dataset.SetProjection(self.prj)
            
            good_looks_band1 = good_looks_dataset.GetRasterBand(1)
            good_looks_band1.SetNoDataValue(0)
            good_looks_band1.WriteArray(good_looks)
            
            good_looks_band1 = None
            good_looks_dataset = None
 
            # create the bad data mask that will hold a stack of
            # mask_data_bad for a single row for all the files
            curr_mask_data_bad = zeros((n_files, self.ncol),
                dtype=mask_data_bad.dtype)
            
            # loop through bands and indices for which we want to generate
            # summaries
            for ind in ['band3', 'band4', 'band5', 'band7', 'ndvi', \
                'ndmi', 'nbr', 'nbr2']:
                msg = '    Generating %d %s summary for %s using %d '  \
                    'files ...' % (year, season, ind, n_files)
                logIt (msg, self.log_handler)
                
                # generate the directory name for the index stack
                if (ind == 'ndvi') or (ind == 'ndmi') or (ind == 'nbr') or \
                    (ind == 'nbr2'):
                    dir_name = '%s/%s/' % (self.hdf_dir_name, ind)
                else:   # tif file
                    dir_name = '%s/refl/' % self.hdf_dir_name
    
                # set up the season summaries GeoTIFF file
                temp_file = dir_name + str(year) + '_' + season + '_' +  \
                    ind + '.tif'
                driver = gdal.GetDriverByName("GTiff")
                driver.Create(temp_file, self.ncol, self.nrow, 1,  \
                    gdalconst.GDT_Int16)
                temp_out_dataset = gdal.Open(temp_file, gdalconst.GA_Update)
                if temp_out_dataset is None:
                    msg = 'Could not create output file: ' + temp_file
                    logIt (msg, self.log_handler)
                    return ERROR
    
                temp_out_dataset.SetGeoTransform(self.geotrans)
                temp_out_dataset.SetProjection(self.prj)
                temp_out = temp_out_dataset.GetRasterBand(1)
                temp_out.SetNoDataValue(self.nodata)

                # create the index/band datasets --stack of ncols
                band_data = zeros((n_files, self.ncol), dtype=int16)
                
                # loop through the current set of files, open them, and
                # attach to the proper band
                input_ds = {}
                temp_band = {}
                for i in range(0, n_files):
                    temp = files[i]
                    base_file = os.path.basename(files[i]).replace( \
                        '.hdf', '.tif')
                    temp_file = '%s%s' % (dir_name, base_file)
                    my_ds = gdal.Open (temp_file, gdalconst.GA_ReadOnly)
                    if my_ds == None:
                        msg = 'Could not open index/band file: ' + temp_file
                        logIt (msg, self.log_handler)
                        return ERROR
                    input_ds[i] = my_ds
                    
                    # open the appropriate band in the input image; if
                    # processing an index product, then read band 1
                    if ind == 'band3':
                        my_temp_band = my_ds.GetRasterBand(3)
                    elif ind == 'band4':
                        my_temp_band = my_ds.GetRasterBand(4)
                    elif ind == 'band5':
                        my_temp_band = my_ds.GetRasterBand(5)
                    elif ind == 'band7':
                        my_temp_band = my_ds.GetRasterBand(7)
                    else:  # index product
                        my_temp_band = my_ds.GetRasterBand(1)

                    # make sure the band is valid
                    if my_temp_band == None:
                        msg = 'Could not open raster band for ' + ind
                        logIt (msg, self.log_handler)
                        return ERROR
                    temp_band[i] = my_temp_band

                # loop through each line in the image and process
                for y in range (0, self.nrow):
#                    print 'Line: ' + str(y)
                    # loop through the current set of files and process them
                    for i in range(0, n_files):
#                        print '  Stacking file: ' + files[i]
                        # read the current row of data
                        my_temp_band = temp_band[i]
                        band_data[i,:] = my_temp_band.ReadAsArray(0, y,  \
                            self.ncol, 1)[0,]

                        # stack up the current row of the bad data mask
                        curr_mask_data_bad[i,:] = mask_data_bad[i,y,:]
                
                    # summarize the good pixels in the stack for each
                    # line/sample
                    if n_files > 0:
                        # replace bad QA values with zeros
                        band_data[curr_mask_data_bad] = 0
                    
                        # calculate totals within each voxel
                        sum_data = apply_over_axes(sum, band_data,  \
                            axes=[0])[0,]
                        
                        # divide by the number of good looks within a voxel
                        mean_data = sum_data / good_looks[y,]
                        
                        # fill with nodata values in places where we would
                        # have divide by zero errors
                        mean_data[good_looks[y,]== 0] = self.nodata
                    else:
                       # create a line of nodata -- nrow=1 x ncols
                        mean_data = zeros((self.ncol), dtype=uint16) +  \
                            self.nodata
    
                    # write the season summaries to a GeoTIFF file
                    mean_data_2d = reshape (mean_data, (1, len(mean_data)))
                    temp_out.WriteArray(mean_data_2d, 0, y)
                # end for y
    
                # clean up the data for the current index
                temp_out = None
                temp_out_dataset = None
                band_data = None
                sum_data = None
                mean_data = None
                input_ds = None
                temp_band = None
            # end for ind
 
            # clean up the masked datasets for the current year and season
            mask_data_good = None
            mask_data_bad = None
            good_looks = None
        # end for season
 
        return SUCCESS


    ########################################################################
    # Description: generateAnnualMaximums will generate the maximum values
    # for each year in the temporal stack.  If a log file was specified then
    # the output from each application will be logged to that file.
    #
    # Inputs:
    #   stack_file - name of stack file to create; list of the lndsr products
    #       to be processed in addition to the date, path/row, sensor, bounding
    #       coords, pixel size, and UTM zone
    #
    # Returns:
    #     ERROR - error generating the annual maximums
    #     SUCCESS - successful processing
    #
    # Notes:
    #   1. The seasons will be ignored.
    #######################################################################
    def generateAnnualMaximums (self, stack_file):
        # make sure the stack file exists
        if not os.path.exists(stack_file):
            msg = 'Could not open stack file: ' + stack_file
            logIt (msg, self.log_handler)
            return ERROR

        # ignore divide by zero and invalid (NaN) values when doing array
        # division.  these will be handled on our own.
        seterr(divide='ignore', invalid='ignore')

        # open and read the stack file
        startTime = time.time()
        f_in = open (stack_file, 'r')
        self.csv_data = recfromcsv (stack_file, delimiter=',', names=True,  \
            dtype="string")
        f_in = None
        if self.csv_data == None:
            msg = 'Error reading the stack file: ' + stack_file
            logIt (msg, self.log_handler)
            return ERROR

        # get the sorted, unique years in the stack; grab the first and last
        # year and use as the range of years to be processed.
        years = unique (self.csv_data['year'])
        start_year = years[0]
        end_year = years[len(years)-1]
        msg = '\nProcessing stack for %d - %d' % (start_year, end_year)
        logIt (msg, self.log_handler)

        # determine the mask file for the first scene listed in the stack
        self.hdf_dir_name = os.path.dirname(self.csv_data['file_'][0])
        first_file = self.csv_data['file_'][0]
        base_file = os.path.basename(first_file.replace('.hdf', '.tif'))
        first_file = '%s/mask/%s' % (self.hdf_dir_name, base_file)

        # open the first file in the stack (GeoTIFF) to get ncols and nrows
        # and other associated info for the stack of scenes
        tifBand1 = TIF_Scene_1_band(first_file, 1, self.log_handler)
        if tifBand1 == None:
            msg = 'Error reading the GeoTIFF file: ' + first_file
            logIt (msg, self.log_handler)
            return ERROR

        self.ncol = tifBand1.NCol
        self.nrow = tifBand1.NRow
        self.geotrans = tifBand1.dataset.GetGeoTransform()
        self.prj = tifBand1.dataset.GetProjectionRef()
        self.nodata = tifBand1.NoData
        tifBand1 = None

        # load up the work queue for processing annual maximums in parallel
        work_queue = multiprocessing.Queue()
        num_years = end_year - start_year + 1
        for year in range (start_year, end_year+1):
            work_queue.put(year)

        # create a queue to pass to workers to store the processing status
        result_queue = multiprocessing.Queue()
 
        # spawn workers to process each year in the stack - generate the
        # seasonal summaries
        msg = 'Spawning %d years for processing via %d processors ....' %  \
            (num_years, self.num_processors)
        logIt (msg, self.log_handler)
        for i in range(self.num_processors):
            worker = parallelMaxWorker(work_queue, result_queue, self)
            worker.start()
 
        # collect the results off the queue
        for i in range(num_years):
            status = result_queue.get()
            if status != SUCCESS:
                msg = 'Error processing annual maximums for year %d' %  \
                    start_year + i
                logIt (msg, self.log_handler)
                return ERROR

        endTime = time.time()
        msg = 'Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, self.log_handler)
 
        return SUCCESS


    ########################################################################
    # Description: generateYearMaximums will generate the maximums for the
    # current year.  If a log file was specified then the output from each
    # application will be logged to that file.
    #
    # History:
    #
    # Inputs:
    #   year - year to process the maximums
    #
    # Returns:
    #     ERROR - error generating the maximums for this year
    #     SUCCESS - successful processing
    #
    # Notes:
    #   1. Maximums are the max value for each year for ndvi, ndmi, nbr,
    #      and nbr2.
    #######################################################################
    def generateYearMaximums (self, year):
        # determine which files apply to the current year
        year_files = (self.csv_data['year'] == year)
        n_files = sum (year_files)
 
        # if there aren't any files to process then skip to the next year
        msg = '  year = %d,  file count = %d' % (year, n_files)
        logIt (msg, self.log_handler)
        if n_files == 0:
            return SUCCESS
 
        # generate the directory name for the mask stack
        dir_name = '%s/mask/' % self.hdf_dir_name
            
        # pull the files for the current year
        files = self.csv_data['file_'][year_files]
            
        # create the mask datasets -- stack of nrow x ncols
        mask_data = zeros((n_files, self.nrow, self.ncol), dtype=int16)

        # loop through the current set of files, open the mask files,
        # and stack them up in a 3D array
        for i in range(0, n_files):
            temp = files[i]
            base_file = os.path.basename(temp.replace('.hdf', '.tif'))
            mask_file = '%s%s' % (dir_name, base_file)
            mask_dataset = gdal.Open (mask_file, gdalconst.GA_ReadOnly)
            if mask_dataset == None:
                msg = 'Could not open mask file: ' + mask_file
                logIt (msg, self.log_handler)
                return ERROR
            mask_band = mask_dataset.GetRasterBand(1)
            mask_data[i,:,:] = mask_band.ReadAsArray()
            mask_band = None
            mask_dataset = None
        
        # which voxels in the mask have fill values?
        mask_data_bad = mask_data < 0
        mask_data = None
            
        # create the bad data mask that will hold a stack of mask_data_bad
        # for a single row for all the files
        curr_mask_data_bad = zeros((n_files, self.ncol),
            dtype=mask_data_bad.dtype)
            
        # loop through indices for which we want to generate maximums
        for ind in ['ndvi', 'ndmi', 'nbr', 'nbr2']:
            msg = '    Generating %d maximums for %s using %d files ...' % \
                (year, ind, n_files)
            logIt (msg, self.log_handler)
                
            # generate the directory name for the index stack
            dir_name = '%s/%s/' % (self.hdf_dir_name, ind)
    
            # set up the annual maximum GeoTIFF file
            temp_file = dir_name + str(year) + '_maximum_' + ind + '.tif'
            driver = gdal.GetDriverByName("GTiff")
            driver.Create(temp_file, self.ncol, self.nrow, 1,  \
                gdalconst.GDT_Int16)
            temp_out_dataset = gdal.Open(temp_file, gdalconst.GA_Update)
            if temp_out_dataset is None:
                msg = 'Could not create output file: ' + temp_file
                logIt (msg, self.log_handler)
                return ERROR
    
            temp_out_dataset.SetGeoTransform(self.geotrans)
            temp_out_dataset.SetProjection(self.prj)
            temp_out = temp_out_dataset.GetRasterBand(1)
            temp_out.SetNoDataValue(self.nodata)

            # create the index dataset -- stack of ncols
            indx_data = zeros((n_files, self.ncol), dtype=int16)
                
            # loop through the current set of files, open them, and attach
            # to the proper band
            input_ds = {}
            indx_band = {}
            for i in range(0, n_files):
                temp = files[i]
                base_file = os.path.basename(files[i]).replace('.hdf', '.tif')
                temp_file = '%s%s' % (dir_name, base_file)
                my_ds = gdal.Open (temp_file, gdalconst.GA_ReadOnly)
                if my_ds == None:
                    msg = 'Could not open index file: ' + temp_file
                    logIt (msg, self.log_handler)
                    return ERROR
                input_ds[i] = my_ds
                
                # open band 1 of the index product
                my_indx_band = my_ds.GetRasterBand(1)

                # make sure the band is valid
                if my_indx_band == None:
                    msg = 'Could not open raster band for ' + ind
                    logIt (msg, self.log_handler)
                    return ERROR
                indx_band[i] = my_indx_band

            # loop through each line in the image and process
            for y in range (0, self.nrow):
#                print 'Line: ' + str(y)
                # loop through the current set of files and process them
                for i in range(0, n_files):
#                    print '  Stacking file: ' + files[i]
                    # read the current row of data
                    my_indx_band = indx_band[i]
                    indx_data[i,:] = my_indx_band.ReadAsArray(0, y,  \
                        self.ncol, 1)[0,]

                    # stack up the current row of the bad data mask
                    curr_mask_data_bad[i,:] = mask_data_bad[i,y,:]
                
                # determine maximum values in the stack for each line/sample
                if n_files > 0:
                    # calculate maximum values within each voxel
                    max_data = apply_over_axes(amax, indx_data, axes=[0])[0,]
                else:
                   # create a line of nodata -- nrow=1 x ncols
                    max_data = zeros((self.ncol), dtype=uint16) +  \
                        self.nodata
    
                # write the annual maximums to a GeoTIFF file
                max_data_2d = reshape (max_data, (1, len(max_data)))
                temp_out.WriteArray(max_data_2d, 0, y)
            # end for y
    
            # clean up the data for the current index
            temp_out = None
            temp_out_dataset = None
            indx_data = None
            max_data = None
            input_ds = None
            indx_band = None
        # end for ind
 
        # clean up the masked datasets for the current year
        mask_data_bad = None
 
        return SUCCESS


    ########################################################################
    # Description: processStack will process the temporal stack of data
    # needed for burned area processing.  If a log file was specified, then
    # the output from each application will be logged to that file.
    #
    # History:
    #   Updated on 8/15/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to add processing of the annual maximums.
    #
    # Inputs:
    #   input_dir - name of the directory in which to find the lndsr products
    #       to be processed
    #   logfile - name of the logfile for logging information; if None then
    #       the output will be written to stdout
    #   make_histos - if the user wants to generate histograms and overview
    #       pyramids of the output GeoTIFF products, then set this cmd-line
    #       option
    #   usebin - this specifies if the BA exes reside in the $BIN directory;
    #       if None then the BA exes are expected to be in the PATH
    #
    # Returns:
    #     ERROR - error running the BA applications and script
    #     SUCCESS - successful processing
    #
    # Notes:
    #   1. The script obtains the path of the input stack of temporal data
    #      and changes directory to that path for running the burned area
    #      temporal stack code.  If the input directory is not writable, then
    #      this script exits with an error.
    #   2. If the input_dir is not specified and the information is going
    #      to be grabbed from the command line, then it's assumed all the
    #      parameters will be pulled from the command line.
    #######################################################################
    def processStack (self, input_dir=None, logfile=None, make_histos=None,  \
        usebin=None):
        # if no parameters were passed then get the info from the
        # command line
        msg = "Start time:" +  \
            str(datetime.datetime.now().strftime("%b %d %Y %H:%M:%S"))
        logIt (msg, self.log_handler)
        startTime0 = time.time()
        if input_dir == None:
            # get the command line argument for the input parameters
            parser = OptionParser()
            parser.add_option ("-f", "--input_dir", type="string",
                dest="input_dir",
                help="path to the input directory where the temporal stack of data resides and where the data will be processed (i.e. need read and write permissions to this directory)", metavar="FILE")
            parser.add_option ("-l", "--logfile", type="string", dest="logfile",
                help="name of optional log file", metavar="FILE")
            parser.add_option ("--make_histos", dest="make_histos",
                default=False, action="store_true",
                help="process histograms and overviews for each of the GeoTIFFs generated by this application")
            parser.add_option ("-p", "--num_processors", type="int",
                dest="num_processors",
                help="how many processors should be used for parallel processing sections of the application (default = 1, single threaded")
            parser.add_option ("--usebin", dest="usebin", default=False,
                action="store_true",
                help="use BIN environment variable as the location of external burned area apps")
            (options, args) = parser.parse_args()
    
            # validate the command-line options
            logfile = options.logfile
            self.make_histos = options.make_histos
            usebin = options.usebin

            # input directory
            input_dir = options.input_dir
            if input_dir == None:
                parser.error ("missing input directory command-line argument");
                return ERROR

            # number of processors
            if options.num_processors != None:
                self.num_processors = options.num_processors

        # open the log file if it exists; use line buffering for the output
        self.log_handler = None
        if logfile != None:
            self.log_handler = open (logfile, 'w', buffering=1)
        msg = 'Burned area temporal stack processing of directory: %s' % \
            input_dir
        logIt (msg, self.log_handler)
        
        # if the input_dir doesn't end with a closing directory path separator
        # then end it with one so that we don't have to add later when
        # concatinating filenames to this directory name
        if input_dir[len(input_dir)-1] != '/':
            input_dir = input_dir + '/'

        # should we expect the external applications to be in the PATH or in
        # the BIN directory?
        if usebin:
            # get the BIN dir environment variable
            bin_dir = os.environ.get('BIN')
            if bin_dir == None:
                msg = 'ERROR: BIN environment variable not defined'
                logIt (msg, self.log_handler)
                return ERROR
            bin_dir = bin_dir + '/'
            msg = 'BIN environment variable: %s' % bin_dir
            logIt (msg, self.log_handler)
        else:
            # don't use a path to the external applications
            bin_dir = ""
            msg = 'External burned area executables expected to be in the PATH'
            logIt (msg, self.log_handler)
        
        # make sure the input directory exists and is writable
        self.input_dir = input_dir
        if not os.path.exists(input_dir):
            msg = 'Input directory does not exist: ' + input_dir
            logIt (msg, self.log_handler)
            return ERROR

        if not os.access(input_dir, os.W_OK):
            msg = 'Input directory is not writable: %s.  Burned area apps ' \
                'need write access to this directory.' % input_dir
            logIt (msg, self.log_handler)
            return ERROR
        msg = 'Changing directories for burned area stack processing: %s' % \
            input_dir
        logIt (msg, self.log_handler)

        # save the current working directory for return to upon error or when
        # processing is complete
        mydir = os.getcwd()
        os.chdir (input_dir)

        # generate the stack and list files for processing to identify the
        # list of reflectance files to be processed
        msg = 'Calling stack_generate'
        logIt (msg, self.log_handler)
        stack_file = "hdf_stack.csv"
        list_file = "hdf_list.txt"
        status = self.generate_stack (stack_file, list_file)
        if status != SUCCESS:
            msg = 'Error generating the list of files to be processed. ' \
                'Processing will terminate.'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # run the executable to determine the maximum bounding extent of
        # the temporal stack of products.  exit if any errors occur.
        bounding_box_file = "bounding_box_coordinates.csv"
        cmdstr = "%sdetermine_max_extent --list_file=%s --extent_file=%s " \
            "--verbose" % (bin_dir, list_file, bounding_box_file)
        (status, output) = commands.getstatusoutput (cmdstr)
        logIt (output, self.log_handler)
        exit_code = status >> 8
        if exit_code != 0:
            msg = 'Error running determine_max_extent. Processing will ' \
                'terminate.'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # convert the HDF files to GeoTIFF and then resample the files to
        # the maximum bounding extent of the stack
        status = self.stackHDFToTiff (bounding_box_file, stack_file)
        if status != SUCCESS:
            msg = 'Error converting the list of files from HDF to GeoTIFF. ' \
                'Processing will terminate.'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # generate the seasonal summaries for each year in the stack
        status = self.generateSeasonalSummaries (stack_file)
        if status != SUCCESS:
            msg = 'Error generating the seasonal summaries. Processing will ' \
                'terminate.'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # generate the annual maximums for each year in the stack
        status = self.generateAnnualMaximums (stack_file)
        if status != SUCCESS:
            msg = 'Error generating the annual maximums. Processing will ' \
                'terminate.'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        endTime0 = time.time()
        msg = '***Total stack processing time = ' +   \
            str (endTime0 - startTime0) + ' seconds'
        logIt (msg, self.log_handler)

        msg = "End time:" + \
            str(datetime.datetime.now().strftime("%b %d %Y %H:%M:%S"))
        logIt (msg, self.log_handler)

######end of temporalBAStack class######

if __name__ == "__main__":
    sys.exit (temporalBAStack().processStack())
