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
from optparse import OptionParser
from HDF_scene import *
from TIF_scene import *
from spectral_indices import *
from spectral_index_from_tif import *
from log_it import *


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
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     ERROR - error generating the stack and list files
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def generate_stack (self, stack_file, list_file, log_handler=None):
        # define CSV delimiter
        delim = ','
        
        # check to make sure the output location exists, create the directory
        # if needed
        stack_dirname = os.path.dirname(stack_file)
        if stack_dirname != "" and not os.path.exists (stack_dirname):
            msg = 'Creating directory for output file: ' + stack_dirname
            logIt (msg, log_handler)
            os.makedirs (stack_dirname)
    
        list_dirname = os.path.dirname(list_file)
        if list_dirname != "" and not os.path.exists (list_dirname):
            msg = 'Creating directory for output file: ' + list_dirname
            os.makedirs (list_dirname)
    
        # open the output files
        fstack_out = open(stack_file, 'w')
        if not fstack_out:
            msg = 'Could not open stack_file: ' + fstack_out
            logIt (msg, log_handler)
            return ERROR
            
        flist_out = open(list_file, 'w')
        if not flist_out:
            msg = 'Could not open list_file: ' + flist_out
            logIt (msg, log_handler)
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
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     None - error reading the bounding extents
    #     return_dict - dictionary of spatial extents
    #
    # Notes:
    #######################################################################
    def stackSpatialExtent(self, bounding_extents_file, log_handler=None):
        # check to make sure the input file exists before opening
        if not os.path.exists (bounding_extents_file):
            msg = 'Bounding extents file does not exist: ' + \
                bounding_extents_file
            logIt (msg, log_handler)
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
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     ERROR - error converting the HDF file to GeoTIFF
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def hdf2tif(self, input_file, output_file, log_handler=None):
        # test to make sure the input file exists
        if not os.path.exists(input_file):
            msg = 'Input file does not exist: ' + input_file
            logIt (msg, log_handler)
            return ERROR
    
        # make sure the output directory exists, otherwise create it
        output_dirname = os.path.dirname (output_file)
        if output_dirname != "" and not os.path.exists (output_dirname):
            msg = 'Creating directory for output file: ' + output_dirname
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        
        # open the input file
        hdfAttr = HDF_Scene(input_file, log_handler)
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
    
        # read and write each band, including the QA band, converting from HDF
        # to GeoTIFF; the QA band is a combination of all the QA values
        # (negative values flag any non-clear pixels and -9999 represents the
        # fill pixels).
        logIt ('    Band 1...', log_handler)
        vals = hdfAttr.getBandValues ('band1', log_handler)
        if vals == None:
            msg = 'Error reading band1 from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band1.WriteArray (vals)

        logIt ('    Band 2...', log_handler)
        vals = hdfAttr.getBandValues ('band2', log_handler)
        if vals == None:
            msg = 'Error reading band2 from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band2.WriteArray (vals)

        logIt ('    Band 3...', log_handler)
        vals = hdfAttr.getBandValues ('band3', log_handler)
        if vals == None:
            msg = 'Error reading band3 from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band3.WriteArray (vals)

        logIt ('    Band 4...', log_handler)
        vals = hdfAttr.getBandValues ('band4', log_handler)
        if vals == None:
            msg = 'Error reading band4 from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band4.WriteArray (vals)

        logIt ('    Band 5...', log_handler)
        vals = hdfAttr.getBandValues ('band5', log_handler)
        if vals == None:
            msg = 'Error reading band5 from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band5.WriteArray (vals)

        logIt ('    Band 6...', log_handler)
        vals = hdfAttr.getBandValues ('band6', log_handler)
        if vals == None:
            msg = 'Error reading band6 from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band6.WriteArray (vals)

        logIt ('    Band 7...', log_handler)
        vals = hdfAttr.getBandValues ('band7', log_handler)
        if vals == None:
            msg = 'Error reading band7 from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band7.WriteArray (vals)

        logIt ('    QA band ...', log_handler)
        vals = hdfAttr.getBandValues ('band_qa', log_handler)
        if vals == None:
            msg = 'Error reading QA bands from HDF file'
            logIt (msg, log_handler)
            os.makedirs (output_dirname)
        output_band_QA.WriteArray(vals)
    
##        # loop through all the lines in the image, read the line of data from
##        # the HDF file, and write back out to GeoTIFF;  the QA band is a
##        # combination of all the QA values (negative values flag non-clear
##        # pixels and -9999 is the fill pixel).
##        for y in range( 0, hdfAttr.NRow):
##            # read a row of data
##            vals = hdfAttr.getLineOfBandValues(y)    
##            output_band1.WriteArray (array([vals['band1']]), 0, y)
##            output_band2.WriteArray (array([vals['band2']]), 0, y)
##            output_band3.WriteArray (array([vals['band3']]), 0, y)
##            output_band4.WriteArray (array([vals['band4']]), 0, y)
##            output_band5.WriteArray (array([vals['band5']]), 0, y)
##            output_band6.WriteArray (array([vals['band6']]), 0, y)
##            output_band7.WriteArray (array([vals['band7']]), 0, y)
##            output_band_QA.WriteArray (array([vals['QA']]), 0, y)
          
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
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     ERROR - error converting all the HDF files to GeoTIFF and resampling
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def stackHDFToTiff(self, bounding_extents_file, stack_file,  \
        log_handler=None):
        # read the spatial extents
        self.spatial_extent = self.stackSpatialExtent (bounding_extents_file, \
            log_handler)
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
            logIt (msg, log_handler)
            os.makedirs (self.refl_dir)
        if not os.path.exists (self.ndvi_dir):
            msg = 'Creating directory for GeoTIFF NDVI files'
            logIt (msg, log_handler)
            os.makedirs (self.ndvi_dir)
        if not os.path.exists (self.ndmi_dir):
            msg = 'Creating directory for GeoTIFF NDMI files'
            logIt (msg, log_handler)
            os.makedirs (self.ndmi_dir)
        if not os.path.exists (self.nbr_dir):
            msg = 'Creating directory for GeoTIFF NBR files'
            os.makedirs (self.nbr_dir)
        if not os.path.exists (self.nbr2_dir):
            msg = 'Creating directory for GeoTIFF NBR2 files'
            logIt (msg, log_handler)
            os.makedirs (self.nbr2_dir)
        if not os.path.exists (self.mask_dir):
            msg = 'Creating directory for GeoTIFF mask files'
            logIt (msg, log_handler)
            os.makedirs (self.mask_dir)

        # process each scene in the stack - convert from HDF to GeoTIFF,
        # create histograms and pyramids, and calculate the spectral indices
        # GAIL - can we run multiprocessor with parallel python?  Basically
        # the code in the for loop could be another module and that module
        # could be run in parallel for all the scenes in the stack.
        for scene in enumerate (stack):
            hdf_file = scene[1][header_row.index('file')]
            status = self.sceneHDFToTiff (hdf_file, log_handler)
            if status != SUCCESS:
                msg = 'Error converting the HDF file (%s) to GeoTIFF. ' \
                    'Processing will terminate.' % hdf_file
                logIt (msg, log_handler)
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
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     ERROR - error converting all the HDF files to GeoTIFF and resampling
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def sceneHDFToTiff(self, hdf_file, log_handler=None):
        startTime0 = time.time()
   
        # generate the HDF, GeoTIFF, and temporary GeoTIFF filename
        msg = '############################################################'
        logIt (msg, log_handler)
        tif_file = hdf_file.replace('.hdf', '.tif')
        tif_file = self.refl_dir + os.path.basename (tif_file)
        temp_file = self.refl_dir + "temp.tif"
        
        # convert .hdf to .tif
        startTime = time.time()
        msg = '   Converting HDF (%s) to GeoTIFF (%s)' % (hdf_file, \
            temp_file)
        logIt (msg, log_handler)
        status = self.hdf2tif (hdf_file, temp_file, log_handler)
        if status != SUCCESS:
            msg = 'Error converting the HDF file to GeoTIFF. Processing ' \
                'will terminate.'
            logIt (msg, log_handler)
            return ERROR
        endTime = time.time()
        msg = '    Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, log_handler)

        # resample the .tif file to our maximum bounding coords
        startTime = time.time()
        msg = '   Resizing temp (%s) to max bounds (%s)' % (temp_file, \
            tif_file)
        logIt (msg, log_handler)
        cmd = 'gdal_merge.py -o ' + tif_file + ' -co "INTERLEAVE=BAND" -co "TILED=YES" -init -9999 -n -9999 -a_nodata -9999 -ul_lr ' + str(self.spatial_extent['West']) + ' ' + str(self.spatial_extent['North']) + ' ' + str(self.spatial_extent['East']) + ' ' + str(self.spatial_extent['South']) + ' ' + temp_file
        msg = '    ' + cmd
        logIt (msg, log_handler)
        os.system(cmd)

        endTime = time.time()
        msg = '    Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, log_handler)
   
        # remove the temp file since it is no longer needed
        os.remove (temp_file)

        # open the tif, calculate histograms for each band in the .tif
        # file, and build pyramids -- if the user specifies this should
        # be done
        if self.make_histos:
            startTime = time.time()
            msg = '    Polishing image ...'
            logIt (msg, log_handler)
            status = polishImage (tif_file, log_handler)
            if status != SUCCESS:
                msg = 'Error polishing the GeoTIFF file. Processing ' \
                    'will terminate.'
                logIt (msg, log_handler)
                return ERROR
            
            endTime = time.time()
            msg = '    Processing time = ' + str(endTime-startTime) +  \
                ' seconds'
            logIt (msg, log_handler)

        # calculate ndvi, ndmi, nbr, nbr2 from the converted tif file
        startTime = time.time()
        msg = '   Calculating spectral indices...'
        logIt (msg, log_handler)
        specIndx = spectralIndex (tif_file, log_handler)
        if status != SUCCESS:
            # error message already written by class constructor
            return ERROR

        ndvi_file = self.ndvi_dir + os.path.basename(tif_file)
        specIndx.createSpectralIndex (ndvi_file, 'ndvi', self.make_histos,
            log_handler)
        
        ndmi_file = self.ndmi_dir + os.path.basename(tif_file)
        specIndx.createSpectralIndex (ndmi_file, 'ndmi', self.make_histos,
            log_handler)
        
        nbr_file = self.nbr_dir + os.path.basename(tif_file)
        specIndx.createSpectralIndex (nbr_file, 'nbr', self.make_histos,
            log_handler)
        
        nbr2_file = self.nbr2_dir + os.path.basename(tif_file)
        specIndx.createSpectralIndex (nbr2_file, 'nbr2', self.make_histos,
            log_handler)
        
        # create the mask file (basically copy over the QA file)
        mask_file = self.mask_dir + os.path.basename(tif_file)
        specIndx.createSpectralIndex (mask_file, 'mask', self.make_histos,
            log_handler)

        endTime = time.time()
        msg = '    Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, log_handler)

        endTime0 = time.time()
        msg = '***Total scene processing time = %d seconds' % \
            (endTime0 - startTime0)
        logIt (msg, log_handler)
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
    #   log_handler - open log file for logging or None for stdout
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
    def generateSeasonalSummaries (self, stack_file, log_handler=None):
        # make sure the stack file exists
        if not os.path.exists(stack_file):
            msg = 'Could not open stack file: ' + stack_file
            logIt (msg, log_handler)
            return ERROR

        # ignore divide by zero and invalid (NaN) values when doing array
        # division.  these will be handled on our own.
        seterr(divide='ignore', invalid='ignore')

        # open and read the stack file
        f_in = open (stack_file, 'r')
        csv_data = recfromcsv (stack_file, delimiter=',', names=True,  \
            dtype="string")
        f_in = None
        if csv_data == None:
            msg = 'Error reading the stack file: ' + stack_file
            logIt (msg, log_handler)
            return ERROR

        # get the sorted, unique years in the stack; grab the first and last
        # year and use as the range of years to be processed.
        years = unique (csv_data['year'])
        start_year = years[0]
        end_year = years[len(years)-1]
        msg = '\nProcessing stack for %d - %d' % (start_year, end_year)
        logIt (msg, log_handler)

        # determine the mask file for the first scene listed in the stack
        hdf_dir_name = os.path.dirname(csv_data['file_'][0])
        first_file = csv_data['file_'][0]
        base_file = os.path.basename(first_file.replace('.hdf', '.tif'))
        first_file = '%s/mask/%s' % (hdf_dir_name, base_file)

        # open the first file in the stack (GeoTIFF) to get ncols and nrows
        # and other associated info for the stack of scenes
        tifBand1 = TIF_Scene_1_band(first_file, 1, log_handler)
        if tifBand1 == None:
            msg = 'Error reading the GeoTIFF file: ' + first_file
            logIt (msg, log_handler)
            return ERROR

        ncol = tifBand1.NCol
        nrow = tifBand1.NRow
        geotrans = tifBand1.dataset.GetGeoTransform()
        prj = tifBand1.dataset.GetProjectionRef()
        nodata = tifBand1.NoData

        # loop through years
        for year in range (start_year, end_year+1):
            # print some general info for the current year and determine
            # last year
            startTime = time.time()
            n_files = sum(csv_data['year'] == year)
            msg = 'Year: %d  Number of files: %d' % (year, n_files)
            logIt (msg, log_handler)
            last_year = year - 1

            # if there aren't any files to process skip to the next year
            if n_files == 0:
                continue

            # loop through seasons
            # GAIL - this could be parallelized via multiprocessing
            for season in ['winter', 'spring', 'summer', 'fall']:
                # determine which files apply to the current season in the
                # current year
                if season == 'winter':
                    season_files =  \
                        ((csv_data['year'] == last_year) &  \
                         (csv_data['month'] == 12)) |  \
                        ((csv_data['year'] == year) &  \
                        ((csv_data['month'] == 1) | (csv_data['month'] == 2)))
                elif season == 'spring':
                    season_files = (csv_data['year'] == year) &  \
                        ((csv_data['month'] >= 3) & (csv_data['month'] <= 5))
                elif season == 'summer':
                    season_files = (csv_data['year'] == year) &  \
                        ((csv_data['month'] >= 6) & (csv_data['month'] <= 8))
                elif season=='fall':
                    season_files = (csv_data['year'] == year) &  \
                        ((csv_data['month'] >= 9) & (csv_data['month'] <= 11))

                # how many files do we have for the current year and season?
                # if there aren't any files to process then skip to the next
                # season
                n_files = sum(season_files)
                msg = '  season = %s,  file count = %d' % (season, n_files)
                logIt (msg, log_handler)
                if n_files == 0:
                    continue
    
                # generate the directory name for the mask stack
                dir_name = '%s/mask/' % hdf_dir_name
                
                # pull the files for this year and season
                files = csv_data['file_'][season_files]
                
                # create the mask datasets -- stack of nrow x ncols
                mask_data = zeros((n_files, nrow, ncol), dtype=int16)
                
                # loop through the current set of files, open the mask files,
                # and stack them up in a 3D array
                for i in range(0, n_files):
                    temp = files[i]
                    base_file = os.path.basename(temp.replace('.hdf', '.tif'))
                    mask_file = '%s%s' % (dir_name, base_file)
                    mask_dataset = gdal.Open (mask_file, gdalconst.GA_ReadOnly)
                    if mask_dataset == None:
                        msg = 'Could not open mask file: ' + mask_file
                        logIt (msg, log_handler)
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
                logIt (msg, log_handler)
                if n_files > 0:
                    good_looks = apply_over_axes(sum, mask_data_good,  \
                        axes=[0])[0,:,:]
                else:
                    good_looks = zeros((nrow,ncol), dtype=uint8)
                
                # save the good looks output to a GeoTIFF file
                good_looks_file = dir_name + str(year) + '_' + season +  \
                    '_good_count.tif'
                
                # write the good looks count to an output GeoTIFF file as a
                # byte product.  the noData value for this set will be 0 vs.
                # the traditional nodata value of -9999, since we are working
                # with a byte product.
                driver = gdal.GetDriverByName("GTiff")
                driver.Create(good_looks_file, ncol, nrow, 1,  \
                    gdalconst.GDT_Byte)
                good_looks_dataset = gdal.Open(good_looks_file,  \
                    gdalconst.GA_Update)
                if good_looks_dataset is None:
                    msg = 'Could not create output file: ', good_looks_file
                    logIt (msg, log_handler)
                    return ERROR
                
                good_looks_dataset.SetGeoTransform(geotrans)
                good_looks_dataset.SetProjection(prj)
                
                good_looks_band1 = good_looks_dataset.GetRasterBand(1)
                good_looks_band1.SetNoDataValue(0)
                good_looks_band1.WriteArray(good_looks)
                
                good_looks_band1 = None
                good_looks_dataset = None

                # loop through bands and indices for which we want to generate
                # summaries
                for ind in ['band3', 'band4', 'band5', 'band7', 'ndvi', \
                    'ndmi', 'nbr', 'nbr2']:
                    msg = '    Generating %d %s summary for %s using %d '  \
                        'files ...' % (year, season, ind, n_files)
                    logIt (msg, log_handler)
                    
                    # generate the directory name for the index stack
                    if (ind == 'ndvi') or (ind == 'ndmi') or (ind == 'nbr') or \
                        (ind == 'nbr2'):
                        dir_name = '%s/%s/' % (hdf_dir_name, ind)
                    else:   # tif file
                        dir_name = '%s/refl/' % hdf_dir_name
        
                    # create the index/band datasets --stack of nrow x ncols
                    band_data = zeros((n_files, nrow, ncol), dtype=int16)
                    
                    # loop through the current set of files
                    for i in range(0, n_files):
                        temp = files[i]
                        base_file = os.path.basename(files[i]).replace( \
                            '.hdf', '.tif')
                        temp_file = '%s%s' % (dir_name, base_file)
                        temp_dataset = gdal.Open (temp_file,  \
                            gdalconst.GA_ReadOnly )
                        if temp_dataset == None:
                            msg = 'Could not open index/band file: ' + temp_file
                            logIt (msg, log_handler)
                            return ERROR
                        
                        # open the appropriate band in the input image; if
                        # processing an index product, then read band 1
                        if ind == 'band3':
                            temp_band = temp_dataset.GetRasterBand(3)
                        elif ind == 'band4':
                            temp_band = temp_dataset.GetRasterBand(4)
                        elif ind == 'band5':
                            temp_band = temp_dataset.GetRasterBand(5)
                        elif ind == 'band7':
                            temp_band = temp_dataset.GetRasterBand(7)
                        else:  # index product
                            temp_band = temp_dataset.GetRasterBand(1)
                            
                        band_data[i,:,:] = temp_band.ReadAsArray()
                        temp_dataset = None
                        temp_band = None
                    
                    # summarize the good pixels in the stack for each
                    # line/sample
                    if n_files > 0:
                        # replace bad QA values with zeros
                        band_data[mask_data_bad] = 0
                    
                        # calculate totals within each voxel
                        sum_data = apply_over_axes(sum, band_data,  \
                            axes=[0])[0,:,:]
                        
                        # divide by the number of good looks within a voxel
                        mean_data = sum_data / good_looks
                        
                        # fill with nodata values in places where we would
                        # have divide by zero errors
                        mean_data[good_looks == 0] = nodata
                    else:
                        mean_data = zeros((nrow,ncol), dtype=uint16) + nodata
        
                    # save the season summaries to a GeoTIFF file
                    temp_file = dir_name + str(year) + '_' + season + '_' +  \
                        ind + '.tif'
        
                    driver = gdal.GetDriverByName("GTiff")
                    driver.Create(temp_file,ncol,nrow,1,gdalconst.GDT_Int16)
                    temp_dataset = gdal.Open(temp_file, gdalconst.GA_Update)
                    if temp_dataset is None:
                        msg = 'Could not create output file: ' + temp_file
                        logIt (msg, log_handler)
                        return ERROR
        
                    temp_dataset.SetGeoTransform(geotrans)
                    temp_dataset.SetProjection(prj)
        
                    temp_band1 = temp_dataset.GetRasterBand(1)
                    temp_band1.SetNoDataValue(nodata)
                    temp_band1.WriteArray(mean_data)
        
                    # clean up the data for the current index
                    temp_band1 = None
                    temp_dataset = None
                    band_data = None
                    sum_data = None
                    mean_data = None
                # end for ind

                # clean up the masked datasets for the current year and season
                mask_data_good = None
                mask_data_bad = None
                good_looks = None
            # end for season

            endTime = time.time()
            msg = 'Processing time = ' + str(endTime-startTime) + ' seconds'
            logIt (msg, log_handler)
        # end for year

        return SUCCESS


    ########################################################################
    # Description: processStack will process the temporal stack of data
    # needed for burned area processing.  If a log file was specified, then
    # the output from each application will be logged to that file.
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

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile != None:
            log_handler = open (logfile, 'w', buffering=1)
        msg = 'Burned area temporal stack processing of directory: %s' % \
            input_dir
        logIt (msg, log_handler)
        
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
            bin_dir = bin_dir + '/'
            msg = 'BIN environment variable: %s' % bin_dir
            logIt (msg, log_handler)
        else:
            # don't use a path to the external applications
            bin_dir = ""
            msg = 'External burned area executables expected to be in the PATH'
            logIt (msg, log_handler)
        
        # make sure the input directory exists and is writable
        self.input_dir = input_dir
        if not os.path.exists(input_dir):
            msg = 'Input directory does not exist: ' + input_dir
            logIt (msg, log_handler)
            return ERROR

        if not os.access(input_dir, os.W_OK):
            msg = 'Input directory is not writable: %s.  Burned area apps ' \
                'need write access to this directory.' % input_dir
            logIt (msg, log_handler)
            return ERROR
        msg = 'Changing directories for burned area stack processing: %s' % \
            input_dir
        logIt (msg, log_handler)

        # save the current working directory for return to upon error or when
        # processing is complete
        mydir = os.getcwd()
        os.chdir (input_dir)

        # generate the stack and list files for processing to identify the
        # list of reflectance files to be processed
        msg = 'Calling stack_generate'
        logIt (msg, log_handler)
        stack_file = "hdf_stack.csv"
        list_file = "hdf_list.txt"
        status = self.generate_stack (stack_file, list_file, log_handler)
        if status != SUCCESS:
            msg = 'Error generating the list of files to be processed. ' \
                'Processing will terminate.'
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # run the executable to determine the maximum bounding extent of
        # the temporal stack of products.  exit if any errors occur.
        bounding_box_file = "bounding_box_coordinates.csv"
        cmdstr = "%sdetermine_max_extent --list_file=%s --extent_file=%s" % \
            (bin_dir, list_file, bounding_box_file)
        (status, output) = commands.getstatusoutput (cmdstr)
        logIt (output, log_handler)
        exit_code = status >> 8
        if exit_code != 0:
            msg = 'Error running determine_max_extent. Processing will ' \
                'terminate.'
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # convert the HDF files to GeoTIFF and then resample the files to
        # the maximum bounding extent of the stack
        status = self.stackHDFToTiff (bounding_box_file, stack_file, \
            log_handler)
        if status != SUCCESS:
            msg = 'Error converting the list of files from HDF to GeoTIFF. ' \
                'Processing will terminate.'
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        # generate the seasonal summaries for each year in the stack
        status = self.generateSeasonalSummaries (stack_file, log_handler)
        if status != SUCCESS:
            msg = 'Error generating the seasonal summaries. Processing will ' \
                'terminate.'
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR


######end of temporalBAStack class######

if __name__ == "__main__":
    sys.exit (temporalBAStack().processStack())

