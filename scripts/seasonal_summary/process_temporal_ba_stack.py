#! /usr/bin/env python
# use true division so we don't have to worry about scalar divided by scalar
# not being a floating point
from __future__ import division
import sys
import glob
import os
import re
import subprocess
import time
import datetime
import csv
import tempfile
import shutil
import multiprocessing, Queue
from argparse import ArgumentParser

from XML_scene import *
from ENVI_scene import *
from spectral_indices import *
from spectral_index_from_espa import *
from log_it import *
from parallel_worker import *

NUM_SR_BANDS = 13

#############################################################################
# Created on April 29, 2013 by Gail Schmidt, USGS/EROS
# Created class to hold the methods which process various aspects of the
# temporal stack for burned area processing.
#
# History:
# Updated on March 11, 2014 by Gail Schmidt, USGS/EROS
# Modified the scripts to process ESPA input raw binary data products vs.
#   the original HDF file format.
#
# Usage: process_temporal_stack.py --help prints the help message
############################################################################
class temporalBAStack():
    """Class for handling the seasonal summary and annual maximum processing
       of a temporal stack of scenes.
    """

    # Data attributes
    input_dir = "None"        # base directory where sr products reside
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
    nrow = 0                  # number of rows in stack for seasonal summaries
    ncol = 0                  # number of cols in stack for seasonal summaries
    geotrans = None           # geographic trans for seasonal summaries
    prj = None                # geographic projection for seasonal summaries
    nodata = None             # noData value of the HDF files for seasonal summ

    def __init__ (self):
        pass


    def is_scene_l1g (self, mtl_file):
        """Looks at the MTL file and determines if the scene is L1G.
        Description: is_scene_l1g will read the DATA_TYPE from the metadata
            file and return True or False, depending on whether the scene
            is L1G.

        History:
          Created on 12/11/2013 by Gail Schmidt, USGS/EROS LSRD Project
        
        Args:
          mtl_file - name of the metadata file to read and parse

        Returns:
            True - scene is L1G
            False - scene is L1T
        """

        # open and read the input metadata file
        metadata_file = open(mtl_file, "r")
        text_list = metadata_file.readlines()
        metadata_file.close()
        num_lines = len(text_list)

        # loop through each of the lines and check for the DATA_TYPE field
        for i in range(num_lines):
            curr_line = text_list[i].rstrip('\n')
            if '=' in curr_line:
                (field, field_value) = curr_line.split('=')
                field = field.strip()
                if field == 'DATA_TYPE':
                    field_value = field_value.replace('"', '').strip()
                    if field_value == 'L1G':
                        return True

        return False


    def exclude_l1g_files (self):
        """Loops through the HDF files in the input directory and excludes
           the L1G scenes, leaving the L1T scenes.
        Description: exclude_l1g_files will loop through the HDF files in the
            input_dir, read the DATA_TYPE from the associated _MTL.txt file,
            and move the sr and _MTL.txt file for that scene to a subdirectory
            called 'exclude_l1g'.

        History:
          Created on 12/11/2013 by Gail Schmidt, USGS/EROS LSRD Project
          Modified on 4/4/2014 by Gail Schmidt, USGS/EROS LSRD Project
            Updated to use the ESPA internal raw binary file format
        
        Args: None

        Returns:
            ERROR - error excluding the L1G files
            SUCCESS - successful processing
        """

        # loop through the HDF files in the input directory
        l1g_dir = self.input_dir + 'exclude_l1g/'
        for f_in in sort(os.listdir(self.input_dir)):
            if f_in.endswith(".xml") and not f_in.endswith(".aux.xml"):
                input_file = self.input_dir + f_in
                scene_file = f_in

                # get the scene name from the current file
                # (Ex. LT50170391984072XXX07.xml)
                base_file = os.path.basename(scene_file)
                scene_name = base_file.replace('.xml', '')

                # determine the _MTL.txt filename
                mtl_name = scene_name + '_MTL.txt'
                input_mtl_file = self.input_dir + mtl_name

                # read the _MTL.txt file and determine if the scene is L1G
                is_l1g = self.is_scene_l1g (input_mtl_file)

                # if the scene is L1G, then move the scene and MTL file to the
                # exclude_l1g subdirectory
                if is_l1g:
                    # create the L1G exclude directory if it doesn't exist
                    if not os.path.exists(l1g_dir):
                        msg = 'L1G exclude directory does not exist: %s. '  \
                            'Creating ...' % l1g_dir
                        logIt (msg, self.log_handler)
                        os.makedirs(l1g_dir, 0755)

                    # move the scene files to the exclude subdirectory
                    all_files = self.input_dir + scene_name + '*'
                    msg = 'Moving %s to %s' % (all_files, l1g_dir)
                    logIt (msg, self.log_handler)
                    for data in glob.glob(self.input_dir + scene_name + '*'):
                        shutil.move (data, l1g_dir)


    def generate_list (self, list_file):
        """Creates the list_file for the input files to be processed
        Description: generate_list will determine XML files residing in the
            input_dir, then write a simple list_file containing the files to
            be processed.
        
        History:
          Created on 3/12/2014 by Gail Schmidt, USGS/EROS LSRD Project

        Args:
          list_file - name of list file to create; simple list of XML
              products to be processed from the current directory
        
        Returns:
            ERROR - error generating the list files
            SUCCESS - successful processing
        """

        # check to make sure the output location exists, create the directory
        # if needed
        list_dirname = os.path.dirname(list_file)
        if list_dirname != "" and not os.path.exists (list_dirname):
            msg = 'Creating directory for output file: ' + list_dirname
            os.makedirs (list_dirname)
    
        # open the output files
        flist_out = open(list_file, 'w')
        if not flist_out:
            msg = 'Could not open list_file: ' + flist_out
            logIt (msg, self.log_handler)
            return ERROR
            
        # loop through *.xml files in input_directory and gather info
        for f_in in sort(os.listdir(self.input_dir)):
            if f_in.endswith(".xml") and not f_in.endswith(".aux.xml"):
                input_file = self.input_dir + f_in

                # write the XML file to the output list file
                flist_out.write(input_file + '\n')
    
            # end if *.xml files
        # end for all files in this directory
    
        # close the output list file
        flist_out.close()
        return SUCCESS


    def stackSpatialExtent(self, bounding_extents_file):
        """Returns the spatial extents of all files in the temporal stack
           from the spatial extents file.
        Description: stackSpatialExtent will open the input CSV file, which
            contains the bounding extents for the current temporal stack,
            and returns a dictionary of east, west, north, south extents.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 4/29/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize a log file if passed along.
        
        Args:
          bounding_extents_file - name of file which contains the bounding
              extents; first line should identify east, west, north, south
              coords; second line should contain the projection coords
              themselves
        
        Returns:
            None - error reading the bounding extents
            return_dict - dictionary of spatial extents
        """

        # check to make sure the input file exists before opening
        if not os.path.exists (bounding_extents_file):
            msg = 'Bounding extents file does not exist: ' + \
                bounding_extents_file
            logIt (msg, self.log_handler)
            return None

        # open the CSV file, read the column headings in the first line, then
        # read the spatial extents in the second line
        reader = csv.reader (open (bounding_extents_file, 'r'))
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


    def resampleStack(self, bounding_extents_file, stack_file):
        """Resamples the ENVI surface reflectance bands in the temporal stack
           using the specified geographic extents.
        Description: resampleStack will resample the surface reflectance
            bands (ENVI bands) in the XML files to the bounding extents.  It
            also computes the spectral indices.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 4/29/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize a log file if passed along.
              Make the histograms and overviews optional.
          Updated on 3/17/2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use the ESPA raw binary internal file format.
        
        Args:
          bounding_extents_file - name of file which contains the bounding
              extents
          stack_file - name of stack file; list of the XML products to be
              processed in addition to the date, path/row, sensor, bounding
              coords, pixel size, and UTM zone
        
        Returns:
            ERROR - error resampling all the surface reflectance bands
            SUCCESS - successful processing
        """

        # read the spatial extents
        self.spatial_extent = self.stackSpatialExtent (bounding_extents_file)
        if self.spatial_extent is None:
            # error message already written
            return ERROR

        # open the stack file and read the header of the stack file
        stack = csv.reader (open (stack_file, 'r'))
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
            msg = 'Creating directory for resampled reflectance files'
            logIt (msg, self.log_handler)
            os.makedirs (self.refl_dir)
        if not os.path.exists (self.ndvi_dir):
            msg = 'Creating directory for resampled NDVI files'
            logIt (msg, self.log_handler)
            os.makedirs (self.ndvi_dir)
        if not os.path.exists (self.ndmi_dir):
            msg = 'Creating directory for resampled NDMI files'
            logIt (msg, self.log_handler)
            os.makedirs (self.ndmi_dir)
        if not os.path.exists (self.nbr_dir):
            msg = 'Creating directory for resampled NBR files'
            os.makedirs (self.nbr_dir)
        if not os.path.exists (self.nbr2_dir):
            msg = 'Creating directory for resampled NBR2 files'
            logIt (msg, self.log_handler)
            os.makedirs (self.nbr2_dir)
        if not os.path.exists (self.mask_dir):
            msg = 'Creating directory for resampled mask files'
            logIt (msg, self.log_handler)
            os.makedirs (self.mask_dir)

        # load up the work queue for processing scenes in parallel
        work_queue = multiprocessing.Queue()
        num_scenes = 0
        for scene in enumerate (stack):
            xml_file = scene[1][header_row.index('file')]
            work_queue.put(xml_file)
            num_scenes += 1

        # make sure we have scenes to be processed
        if num_scenes == 0:
            msg = 'Error resampling bands stack file.  No bands were '  \
                'specified in ' + stack_file
            logIt (msg, self.log_handler)
            return ERROR

        # create a queue to pass to workers to store the processing status
        result_queue = multiprocessing.Queue()
 
        # spawn workers to process each scene in the stack - resample each
        # band, create histograms and pyramids, and calculate the spectral
        # indices
        msg = 'Spawning %d scenes for resampling via %d '  \
            'processors ....' % (num_scenes, self.num_processors)
        logIt (msg, self.log_handler)
        for i in range(self.num_processors):
            worker = parallelSceneWorker(work_queue, result_queue, self)
            worker.start()
 
        # collect the results off the queue
        for i in range(num_scenes):
            status = result_queue.get()
            if status != SUCCESS:
                msg = 'Error resampling bands in XML file (file %d in the ' \
                    'stack).' % i
                logIt (msg, self.log_handler)
                return ERROR

        # close the stack file
        stack = None

        return SUCCESS


    def sceneResample(self, xml_file):
        """Resamples the surface reflectance bands in the XML file to the
           specified geographic extent, creates a single QA band, and computes
           spectral indices.
        Description: sceneResample will resample the suface reflectance bands
            in the XML file to the bounding extents.  It then creates a single
            QA band from the surface reflectance QA bands.  Finally it computes
            the spectral indices for the scene.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 5/7/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to allow for multiprocessing at the scene level.
          Updated on 3/17/2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use the ESPA internal raw binary format
        
        Args:
          xml_file - name of XML file to process
        
        Returns:
            ERROR - error parsing the XML file, resampling each band, or
                determining the spectral indices
            SUCCESS - successful processing
        """
   
        # parse the XML file looking for the surface reflectance bands 1-7
        # and the QA bands.  open each band and store the GDAL band connection.
        startTime0 = time.time()
        xmlAttr = XML_Scene (xml_file)
        if xmlAttr is None:
            msg = 'Error reading the XML and setting up the bands: ' + xml_file
            logIt (msg, self.log_handler)
            return ERROR

        # create a single QA band from the surface reflectance QA bands
        xmlAttr.createQaBand (self.log_handler)

        # resample the .img and single QA bands to our maximum bounding coords
        # and place in the reflectance directory; QA file goes in the mask
        # directory
        resamp_band_dict = {}
        for i in ['band1', 'band2', 'band3', 'band4', 'band5', 'band6',  \
            'band7', 'band_qa']:
            if i == 'band_qa':
                resamp_band_dict[i] = self.mask_dir + \
                    os.path.basename (xmlAttr.band_dict[i])
            else:
                resamp_band_dict[i] = self.refl_dir + \
                    os.path.basename (xmlAttr.band_dict[i])
            msg = '   Resizing file (%s) to max bounds (%s)' %  \
                (xmlAttr.band_dict[i], resamp_band_dict[i])
            logIt (msg, self.log_handler)
            cmd = 'gdal_merge.py -o %s -init -9999 -n -9999 -a_nodata -9999 ' \
                '-ul_lr %d %d %d %d -of ENVI %s' % (resamp_band_dict[i],  \
                self.spatial_extent['West'], self.spatial_extent['North'],  \
                self.spatial_extent['East'], self.spatial_extent['South'],  \
                xmlAttr.band_dict[i])
            msg = '    ' + cmd
            logIt (msg, self.log_handler)
            os.system(cmd)

        # calculate ndvi, ndmi, nbr, nbr2 from the resampled files
        msg = '   Calculating spectral indices...'
        logIt (msg, self.log_handler)
        specIndx = spectralIndex (resamp_band_dict, self.log_handler)
        if specIndx == None:
            msg = 'Error generating the spectral indices for ' + xml_file
            logIt (msg, self.log_handler)
            return ERROR

        idx_dict = {}
        idx_dict['ndvi'] = self.ndvi_dir +  \
            os.path.basename (xml_file.replace ('.xml', '_ndvi.img'))
        idx_dict['ndmi'] = self.ndmi_dir +  \
            os.path.basename (xml_file.replace ('.xml', '_ndmi.img'))
        idx_dict['nbr'] = self.nbr_dir +  \
            os.path.basename (xml_file.replace ('.xml', '_nbr.img'))
        idx_dict['nbr2'] = self.nbr2_dir +  \
            os.path.basename (xml_file.replace ('.xml', '_nbr2.img'))
        status = specIndx.createSpectralIndices (idx_dict, self.make_histos,
            self.log_handler)
        if status != SUCCESS:
            msg = 'Error creating the spectral indices for ' + xml_file
            logIt (msg, self.log_handler)
            return ERROR

        # clean up the classes and dictionaries
        del (idx_dict)
        xmlAttr = None
        specIndx = None

        endTime0 = time.time()
        msg = '***Total scene processing time = %f seconds' %  \
            (endTime0 - startTime0)
        logIt (msg, self.log_handler)
        return SUCCESS


    def generateSeasonalSummaries (self, stack_file):
        """Generates the seasonal summaries for the temporal stack.
        Description: generateSeasonalSummaries will generate the seasonal
        summaries for the temporal stack.  If a log file was specified then the
        output from each application will be logged to that file.
        
        Args:
          stack_file - name of stack file to create; list of the XML products
              to be processed in addition to the date, path/row, sensor,
              bounding coords, pixel size, and UTM zone
        
        Returns:
            ERROR - error generating the seasonal summaries
            SUCCESS - successful processing
        
        Notes:
          1. Seasons are defined as:
             winter = dec (previous year), jan, and feb
             spring = mar, apr, may
             summer = jun, jul, aug
             fall = sep, oct, nov
          2. Seasonal summaries are the mean value for each season for bands
             3, 4, 5, 7, ndvi, ndmi, nbr, and nbr2.
          3. Good count is the number of 'lloks' with no QA flag set
        """

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
        if self.csv_data is None:
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

        # determine band1 file for the first scene listed in the stack
        first_file = self.csv_data['file_'][0]
        base_file = os.path.basename(  \
            first_file.replace('.xml', '_sr_band1.img'))
        first_file = '%s%s' % (self.refl_dir, base_file)

        # open the mask for the first file in the stack to get ncols and nrows
        # and other associated info for the stack of scenes
        enviMask = ENVI_Scene (first_file, self.log_handler)
        if enviMask is None:
             msg = 'Error reading the ENVI file: ' + first_file
             logIt (msg, self.log_handler)
             return ERROR

        self.ncol = enviMask.NCol
        self.nrow = enviMask.NRow
        self.geotrans = enviMask.dataset.GetGeoTransform()
        self.prj = enviMask.dataset.GetProjectionRef()
        self.nodata = enviMask.NoData
        enviMask = None

        # load up the work queue for processing yearly summaries in parallel
        work_queue = multiprocessing.Queue()
        num_years = end_year - start_year + 1
        for year in range (start_year, end_year+1):
            work_queue.put(year)

        # create a queue to pass to workers to store the processing status
        result_queue = multiprocessing.Queue()
 
        # spawn workers to process each year in the stack - generate the
        # seasonal summaries
        msg = 'Spawning %d years for processing seasonal summaries via %d '  \
            'processors ....' % (num_years, self.num_processors)
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
        msg = 'Processing time = %f seconds' % (endTime-startTime)
        logIt (msg, self.log_handler)
 
        return SUCCESS


    def generateYearSeasonalSummaries (self, year):
        """Generates the seasonal summaries for the specified year.
        Description: generateYearSeasonalSummaries will generate the seasonal
        summaries for the current year.  If a log file was specified then the
        output from each application will be logged to that file.
        
        History:
          Updated on 5/22/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to process all the indices one line  at a time (vs. the
              entire band) since this is faster.
          Updated on 9/20/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to process a summary product with fill if there aren't
              any valid inputs for the current season/year.
          Updated on 3/24/2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize the ESPA internal raw binary format

        Args:
          year - year to process the seasonal summaries
        
        Returns:
            ERROR - error generating the seasonal summaries for this year
            SUCCESS - successful processing
        
        Notes:
          1. Seasons are defined as:
             winter = dec (previous year), jan, and feb
             spring = mar, apr, may
             summer = jun, jul, aug
             fall = sep, oct, nov
          2. Seasonal summaries are the mean value for each season for bands
             3, 4, 5, 7, ndvi, ndmi, nbr, and nbr2.
          3. Good count is the number of 'lloks' with no QA flag set
        """

        # loop through seasons
        last_year = year - 1
        for season in ['winter', 'spring', 'summer', 'fall']:
            # determine which scenes apply to the current season in the
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
 
            # how many scenes do we have for the current year and season?
            # if there aren't any files to process then write out a
            # product with fill
            n_files = sum (season_files)
            msg = '  season = %s,  file count = %d' % (season, n_files)
            logIt (msg, self.log_handler)
 
            # pull the files for this year and season
            files = self.csv_data['file_'][season_files]
            
            # create the mask datasets -- stack of nrow x ncols
            mask_data = zeros((n_files, self.nrow, self.ncol), dtype=int16)

            # loop through the current set of files, open the mask files,
            # and stack them up in a 3D array
            for i in range(0, n_files):
                temp = files[i]
                base_file = os.path.basename(temp.replace('.xml', '_mask.img'))
                mask_file = '%s%s' % (self.mask_dir, base_file)
                mask_dataset = gdal.Open (mask_file, gdalconst.GA_ReadOnly)
                if mask_dataset is None:
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
            
            # save the good looks output to an ENVI file
            good_looks_file = self.mask_dir + str(year) + '_' + season +  \
                '_good_count.img'
            
            # write the good looks count to an output ENVI file as a
            # byte product.  the noData value for this set will be 0 vs.
            # the traditional nodata value of -9999, since we are working
            # with a byte product.
            driver = gdal.GetDriverByName('ENVI')
            driver.Create (good_looks_file, self.ncol, self.nrow, 1,  \
                gdalconst.GDT_Byte)
            good_looks_dataset = gdal.Open (good_looks_file,  \
                gdalconst.GA_Update)
            if good_looks_dataset is None:
                msg = 'Could not create output file: ' + good_looks_file
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
                if (ind == 'ndvi'):
                    dir_name = self.ndvi_dir
                    ext = '_%s.img' % ind
                elif (ind == 'ndmi'):
                    dir_name = self.ndmi_dir
                    ext = '_%s.img' % ind
                elif (ind == 'nbr'):
                    dir_name = self.nbr_dir
                    ext = '_%s.img' % ind
                elif (ind == 'nbr2'):
                    dir_name = self.nbr2_dir
                    ext = '_%s.img' % ind
                else:   # refl file
                    dir_name = self.refl_dir
                    ext = '_sr_%s.img' % ind
    
                # set up the season summaries file
                temp_file = dir_name + str(year) + '_' + season + '_' +  \
                    ind + '.img'
                driver = gdal.GetDriverByName('ENVI')
                driver.Create (temp_file, self.ncol, self.nrow, 1,  \
                    gdalconst.GDT_Int16)
                temp_out_dataset = gdal.Open (temp_file, gdalconst.GA_Update)
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
                    base_file = os.path.basename(files[i]).replace('.xml', ext)
                    temp_file = '%s%s' % (dir_name, base_file)
                    my_ds = gdal.Open (temp_file, gdalconst.GA_ReadOnly)
                    if my_ds is None:
                        msg = 'Could not open index/band file: ' + temp_file
                        logIt (msg, self.log_handler)
                        return ERROR
                    input_ds[i] = my_ds
                    
                    # open the appropriate band in the input image
                    my_temp_band = my_ds.GetRasterBand(1)
                    if my_temp_band is None:
                        msg = 'Could not open raster band for ' + ind
                        logIt (msg, self.log_handler)
                        return ERROR
                    temp_band[i] = my_temp_band

                # loop through each line in the image and process
                for y in range (0, self.nrow):
#                    print 'Line: %d' % y
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
    
                    # write the season summaries to a file
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


    def generateAnnualMaximums (self, stack_file):
        """Generates the annual maximums for the temporal stack.
        Description: generateAnnualMaximums will generate the maximum values
        for each year in the temporal stack.  If a log file was specified then
        the output from each application will be logged to that file.
        
        Args:
          stack_file - name of stack file to create; list of the XML products
              to be processed in addition to the date, path/row, sensor,
              bounding coords, pixel size, and UTM zone
        
        Returns:
            ERROR - error generating the annual maximums
            SUCCESS - successful processing
        
        Notes:
          1. The seasons will be ignored.
        """

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
        if self.csv_data is None:
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

        # determine band1 file for the first scene listed in the stack
        first_file = self.csv_data['file_'][0]
        base_file = os.path.basename(  \
            first_file.replace('.xml', '_sr_band1.img'))
        first_file = '%s%s' % (self.refl_dir, base_file)

        # open the mask for the first file in the stack to get ncols and nrows
        # and other associated info for the stack of scenes
        enviMask = ENVI_Scene (first_file, self.log_handler)
        if enviMask is None:
             msg = 'Error reading the ENVI file: ' + first_file
             logIt (msg, self.log_handler)
             return ERROR

        self.ncol = enviMask.NCol
        self.nrow = enviMask.NRow
        self.geotrans = enviMask.dataset.GetGeoTransform()
        self.prj = enviMask.dataset.GetProjectionRef()
        self.nodata = enviMask.NoData
        enviMask = None

        # load up the work queue for processing annual maximums in parallel
        work_queue = multiprocessing.Queue()
        num_years = end_year - start_year + 1
        for year in range (start_year, end_year+1):
            work_queue.put(year)

        # create a queue to pass to workers to store the processing status
        result_queue = multiprocessing.Queue()
 
        # spawn workers to process each year in the stack - generate the
        # seasonal summaries
        msg = 'Spawning %d years for processing annual maximums via %d '  \
            'processors ....' % (num_years, self.num_processors)
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
        msg = 'Processing time = %f seconds' % (endTime-startTime)
        logIt (msg, self.log_handler)
 
        return SUCCESS


    def generateYearMaximums (self, year):
        """Generates the annual maximums for the specified year.
        Description: generateYearMaximums will generate the maximums for the
        current year.  If a log file was specified then the output from each
        application will be logged to that file.
        
        History:
          Updated on 3/24/2014 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize the ESPA internal raw binary format
        
        Args:
          year - year to process the maximums
        
        Returns:
            ERROR - error generating the maximums for this year
            SUCCESS - successful processing
        
        Notes:
          1. Maximums are the max value for each year for ndvi, ndmi, nbr,
             and nbr2.
        """

        # determine which files apply to the current year
        year_files = (self.csv_data['year'] == year)
        n_files = sum (year_files)
 
        # if there aren't any files to process then skip to the next year
        msg = '  year = %d,  file count = %d' % (year, n_files)
        logIt (msg, self.log_handler)
        if n_files == 0:
            return SUCCESS
 
        # pull the files for the current year
        files = self.csv_data['file_'][year_files]
            
        # create the mask datasets -- stack of nrow x ncols
        mask_data = zeros((n_files, self.nrow, self.ncol), dtype=int16)

        # loop through the current set of files, open the mask files,
        # and stack them up in a 3D array
        for i in range(0, n_files):
            temp = files[i]
            base_file = os.path.basename(temp.replace('.xml', '_mask.img'))
            mask_file = '%s%s' % (self.mask_dir, base_file)
            mask_dataset = gdal.Open (mask_file, gdalconst.GA_ReadOnly)
            if mask_dataset is None:
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
            ext = '_%s.img' % ind
            if (ind == 'ndvi'):
                dir_name = self.ndvi_dir
            elif (ind == 'ndmi'):
                dir_name = self.ndmi_dir
            elif (ind == 'nbr'):
                dir_name = self.nbr_dir
            elif (ind == 'nbr2'):
                dir_name = self.nbr2_dir
    
            # set up the annual maximum ENVI file
            temp_file = dir_name + str(year) + '_maximum_' + ind + '.img'
            driver = gdal.GetDriverByName('ENVI')
            driver.Create (temp_file, self.ncol, self.nrow, 1,  \
                gdalconst.GDT_Int16)
            temp_out_dataset = gdal.Open (temp_file, gdalconst.GA_Update)
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
                base_file = os.path.basename(files[i]).replace('.xml', ext)
                temp_file = '%s%s' % (dir_name, base_file)
                my_ds = gdal.Open (temp_file, gdalconst.GA_ReadOnly)
                if my_ds is None:
                    msg = 'Could not open index file: ' + temp_file
                    logIt (msg, self.log_handler)
                    return ERROR
                input_ds[i] = my_ds
                
                # open band 1 of the index product
                my_indx_band = my_ds.GetRasterBand(1)
                if my_indx_band is None:
                    msg = 'Could not open raster band for ' + ind
                    logIt (msg, self.log_handler)
                    return ERROR
                indx_band[i] = my_indx_band

            # loop through each line in the image and process
            for y in range (0, self.nrow):
#                print 'Line: %d' % y
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
    
                # write the annual maximums to an output file
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


    def processStack (self, input_dir=None, exclude_l1g=None, logfile=None,  \
        make_histos=None, num_processors=1, usebin=None):
        """Processes the temporal stack of data to generate seasonal summaries
           and annual maximums for each year in the stack.
        Description: processStack will process the temporal stack of data
        needed for burned area processing.  If a log file was specified, then
        the output from each application will be logged to that file.
        
        History:
          Updated on 8/15/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to add processing of the annual maximums.
          Updated on 12/2/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use argparser vs. optionparser, since optionparser
              is deprecated.
          Updated on 12/16/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Cleaned up the temporary reflectance and mask files in the
              subdirectories.  These are the files that were reprojected to
              the common geographic extents.
        
        Args:
          input_dir - name of the directory in which to find the surface
              reflectance products to be processed; input_list.txt and
              input_stack.csv are both created in this directory.
          exclude_l1g - if True, then the L1G-based files are excluded from the
              processing stack and only the L1T-based files are processed.
              These L1G files are also moved to a directory called exclude_l1g
              in the input directory.
          logfile - name of the logfile for logging information; if None then
              the output will be written to stdout
          make_histos - if the user wants to generate histograms and overview
              pyramids of the output GeoTIFF products, then set this cmd-line
              option
          usebin - this specifies if the BA exes reside in the $BIN directory;
              if None then the BA exes are expected to be in the PATH
        
        Returns:
            ERROR - error running the BA applications and script
            SUCCESS - successful processing
        
        Notes:
          1. The script obtains the path of the input stack of temporal data
             and changes directory to that path for running the burned area
             temporal stack code.  If the input directory is not writable, then
             this script exits with an error.
          2. If the input_dir is not specified and the information is going
             to be grabbed from the command line, then it's assumed all the
             parameters will be pulled from the command line.
        """

        # if no parameters were passed then get the info from the
        # command line
        msg = 'Start time:' +  \
            str(datetime.datetime.now().strftime("%b %d %Y %H:%M:%S"))
        logIt (msg, self.log_handler)
        startTime0 = time.time()
        if input_dir is None:
            # get the command line argument for the input parameters
            parser = ArgumentParser(  \
                description='Generate the seasonal summaries for the ' \
                'specified stack of data')
            parser.add_argument ('-f', '--input_dir', type=str,
                dest='input_dir',
                help='path to the input directory where the temporal stack '  \
                    'of data resides and where the data will be processed '  \
                    '(i.e. need read and write permissions to this '  \
                    'directory)', metavar='DIR')
            parser.add_argument ('-l', '--logfile', type=str, dest='logfile',
                help='name of optional log file', metavar='FILE')
            parser.add_argument ('--make_histos', dest='make_histos',
                default=False, action='store_true',
                help='process histograms and overviews for each of the '  \
                    'GeoTiffs generated by this application')
            parser.add_argument ('-p', '--num_processors', type=int,
                dest='num_processors',
                help='how many processors should be used for parallel '  \
                    'processing sections of the application '  \
                    '(default = 1, single threaded)')
            parser.add_argument ('--usebin', dest='usebin', default=False,
                action='store_true',
                help='use BIN environment variable as the location of '  \
                    'external burned area apps')
            parser.add_argument ('--exclude_l1g', dest='exclude_l1g',
                default=False, action='store_true',
                help='if True, then the L1G files are excluded from the '  \
                     'temporal stack and only the L1T files are processed. ' \
                     'These L1G files are also moved to a directory called ' \
                     'exclude_l1g in the input directory.')

            options = parser.parse_args()
    
            # validate the command-line options
            logfile = options.logfile
            self.make_histos = options.make_histos
            usebin = options.usebin
            exclude_l1g = options.exclude_l1g

            # input directory
            input_dir = options.input_dir
            if input_dir is None:
                parser.error ("missing input directory command-line argument");
                return ERROR

            # number of processors
            if options.num_processors is not None:
                self.num_processors = options.num_processors
        else:
            self.num_processors = num_processors

        # open the log file if it exists; use line buffering for the output
        self.log_handler = None
        if logfile is not None:
            self.log_handler = open (logfile, 'w', buffering=1)
        msg = 'Burned area temporal stack processing of directory: ' +  \
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
            if bin_dir is None:
                msg = 'ERROR: BIN environment variable not defined'
                logIt (msg, self.log_handler)
                return ERROR
            bin_dir = bin_dir + '/'
            msg = 'BIN environment variable: ' + bin_dir
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
        msg = 'Changing directories for burned area stack processing: ' + \
            input_dir
        logIt (msg, self.log_handler)

        # save the current working directory for return to upon error or when
        # processing is complete
        mydir = os.getcwd()
        os.chdir (input_dir)

        # go to the input_directory and exclude the L1G files, if specified
        if exclude_l1g:
            self.exclude_l1g_files()

        # generate the list of XML files that will be processed from the
        # current directory
        list_file = "input_list.txt"
        status = self.generate_list (list_file)
        if status != SUCCESS:
            msg = 'Error creating the list of files to be processed. ' \
                'Processing will terminate.'
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # run the executable to generate the stack of metadata for the input
        # files.  exit if any errors occur.
        logIt (msg, self.log_handler)
        stack_file = "input_stack.csv"
        cmdstr = '%sgenerate_stack --list_file=%s --stack_file=%s ' \
            '--verbose' % (bin_dir, list_file, stack_file)
        cmdlist = cmdstr.split(' ')
        try:
            output = subprocess.check_output (cmdlist, stderr=None)
            logIt (output, self.log_handler)
        except subprocess.CalledProcessError, e:
            msg = 'Error running generate_stack. Processing will ' \
                'terminate.\n ' + e.output
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # run the executable to determine the maximum bounding extent of
        # the temporal stack of products.  exit if any errors occur.
        bounding_box_file = 'bounding_box_coordinates.csv'
        cmdstr = '%sdetermine_max_extent --list_file=%s --extent_file=%s ' \
            '--verbose' % (bin_dir, list_file, bounding_box_file)
        cmdlist = cmdstr.split(' ')
        try:
            output = subprocess.check_output (cmdlist, stderr=None)
            logIt (output, self.log_handler)
        except subprocess.CalledProcessError, e:
            msg = 'Error running determine_max_extent. Processing will ' \
                'terminate.\n ' + e.output
            logIt (msg, self.log_handler)
            os.chdir (mydir)
            return ERROR

        # resample the files to the maximum bounding extent of the stack
        # and calculate the spectral indices
        status = self.resampleStack (bounding_box_file, stack_file)
        if status != SUCCESS:
            msg = 'Error resampling the list of files to the max bounding ' \
                'extents. Processing will terminate.'
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

        # open the stack file and read the header of the stack file
        stack = csv.reader (open (stack_file, 'r'))
        header_row = stack.next()
        for elem in range (0, len(header_row)):
            header_row[elem] = header_row[elem].strip()

        # clean up the temporary files that were created as part of this
        # processing
#        cleanup_dirs = [self.refl_dir, self.ndvi_dir, self.ndmi_dir,
#            self.nbr_dir, self.nbr2_dir, self.mask_dir]
#        for scene in enumerate (stack):
#            for mydir in cleanup_dirs:
#                full_xml_file = scene[1][header_row.index('file')]
#                xml_file = os.path.basename (full_xml_file.replace ('.xml', ''))
#                rm_files = glob.glob (mydir + xml_file + '*')
#                for file in rm_files:
#                    print 'Remove: ' + file
#                    os.remove (os.path.join (file))

        # close the stack file
        stack = None

        # dump out the processing time, convert seconds to hours
        endTime0 = time.time()
        msg = '***Total stack processing time = %f hours' % \
            ((endTime0 - startTime0) / 3600.0)
        logIt (msg, self.log_handler)

        msg = 'End time:' + \
            str(datetime.datetime.now().strftime("%b %d %Y %H:%M:%S"))
        logIt (msg, self.log_handler)
        return SUCCESS

######end of temporalBAStack class######

if __name__ == "__main__":
    sys.exit (temporalBAStack().processStack())
