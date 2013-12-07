#! /usr/bin/env python
#############################################################################
# Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
#       Geographic Science Center
# Created Python script to loop through all the individual burn probability
# (BP) images and then create a burned/unburned classified (BC) image.  The
# BP images are first filtered so only pixels with probabilities greater than
# or equal to the seed probability threshold are returned (as a binary result).
# Then the algorithm finds patches in the binary image and calculates the
# size of the burned areas.  The seed size threshold is used to eliminate
# small patches.  The remaining patches are grown by adding adjacent pixels
# with probabilities greater than or equal to the flood fill probability
# threshold.
#
# History:
#   Updated on 11/26/2013 by Gail Schmidt, USGS/EROS
#       Modified to incorporate into the ESPA environment
#############################################################################

import sys
import os
import time
import getopt

import numpy
import scipy.ndimage
import skimage.measure

from argparse import ArgumentParser
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst

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
# Created on November 29, 2013 by Gail Schmidt, USGS/EROS
# Turned into a class to run the overall burn thresholds on the burn
#     probabilities.
#
# History:
#   Updated on 12/4/2013 by Gail Schmidt, USGS/EROS
#       Modified output classification names to have a .tif extension since
#       they are GeoTiff files.
#
# Usage: do_threshold_stack.py --help prints the help message
############################################################################
class BurnAreaThreshold():
    """Class for handling the burned area thresholding functions.
    """

    def __init__(self):
        pass


    def writeResults(self, outputData, outputFilename, geotrans, prj, nodata, \
        outputRAT=None, makeHistos=False):
        """Writes an array of data to an output file.
        Description: simple function to write an array of data to an output
            Geotiff file
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args:
          outputData - output data structure to be written
          outputFilename - name of GeoTiff file to write the array of data
          geotrans - affine transform for mapping pixel coordinates into
              projection space (returned from GDAL GetGeoTransform())
          prj - projection coordinate system (returned from GDAL
              GetProjectionRef())
          nodata - fill or nodata data value for the output image
          outputRAT - name of output raster attribute table (RAT)
          makeHistos - should histograms and overview pyramids be generated
              for the output GeoTIFF products?  Default is false.
        
        Returns:
            Nothing
        """

        # create the TIF driver for output data
        driver = gdal.GetDriverByName("GTiff")
        
        # create the output dataset
        bp_dataset = driver.Create(outputFilename, outputData.shape[1],  \
            outputData.shape[0], 1, gdal.GDT_Int16)
        bp_dataset.SetGeoTransform(geotrans)
        bp_dataset.SetProjection(prj)
        
        # get the output band
        bp_band = bp_dataset.GetRasterBand(1)
        bp_band.SetNoDataValue(nodata)
        bp_band.WriteArray(outputData)
        
        if outputRAT <> None:
            bp_band.SetDefaultRAT(outputRAT)
        
        # generate the histograms and overviews if specified
        if makeHistos:
            histogram = bp_band.GetDefaultHistogram()
            bp_band.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
            bp_band.FlushCache()
    
            bp_dataset.BuildOverviews(overviewlist=[3,9,27,81,243,729])
            bp_dataset.FlushCache()
    
    
    def floodFill(self, input_image, row, col, output_image, output_label=1,
        local_threshold=75, nodata=-9999):
        """Implements the flood fill algorithm for the identified burned areas.
        Description: routine to implement the flood fill of the burned areas
            using the lower threshold test.  Adjacent pixels are added to the
            burn patch if their thresholds are high enough.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
   
        Args:
          input_image - input image of burn probabilities
          row, col - row and column (line and sample) to start at for the flood
              filling
          output_image - output image of burn/unburned pixels
          output_label - output value to be used to identify the burn pixels as
              part of the flood filling; default is 1
          local_threshold - threshold to be used to add burn pixels from the
              burn probability image to the burn classification; default is 75%
          nodata - pixel value used to identify nodata pixels in the input image
   
        Returns:
            nFill - number of pixels that were flood filled
   
        Notes:
          1. The default lower threshold for flood filling is 75% burn
             probability.
        """
    
        # returns number of pixels that were flood filled
        nFill = 0
        toFill = list()
        toFill.append((row,col))
    
        while len(toFill) > 0:
            (row,col) = toFill.pop()
            x = input_image[row,col]
            lab = output_image[row,col]
                    
            # flood fill pixels that are greater than the defined threshold
            if (x <> nodata) & (lab <> output_label) & (x > local_threshold):
                output_image[row,col] = output_label
                nFill += 1
    
                if row > 1:
                    if output_image[row-1,col] == 0:
                        toFill.append((row-1,col))
    
                if (row+1) < input_image.shape[0]:
                    if output_image[row+1,col] == 0:
                        toFill.append((row+1,col))
    
                if col > 1:
                    if output_image[row,col-1] == 0:
                        toFill.append((row,col-1))
    
                if (col+1) < input_image.shape[1]:
                    if output_image[row,col+1] == 0:
                        toFill.append((row,col+1))
    
        return nFill
        
        
    def findBurnScars(self, bp_image, seed_prob_thresh=97.5,
        seed_size_thresh=5, flood_fill_prob_thresh=75, log_handler=None):
        """Identify the seeds for burn scars from the input burn probabilities.
        Description: routine to find burn scars using the flood-fill approach.
          Seed pixels are found by using the seed threshold.  Any pixels with
          a probability higher than the seed threshold are identified as seed
          pixels.  Any area with more than the seed size threshold is used as
          a seed area for growing the burn extent using the flood fill
          process.  Areas with fewer than the seed size are ignored and not
          flagged as burn areas.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on Nov. 26, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use int32 arrays vs. int64 arrays for the burn
              classification images
        
        Args:
          bp_image - input image of burn probabilities
          seed_prob_thresh - threshold to be used to identify burn pixels
              in the burn probability image as seed pixels for the burn area;
              default is 97.5%
          seed_size_thresh - threshold to be used to identify burn areas in the
              probability image. If the count of burn probability seed pixels is
              greater than this threshold for a certain area then the burn area
              is left and flood-filled.  If the count is less than the seed
              threshold, then this is a false positive and the area is not used
              in the burn classification.  Default is 5 pixels.
          flood_fill_prob_thresh - threshold to be used to add burn pixels
              from the burn probability image to the burn classification via
              flood filling; default is 75%
          log_handler - file handler for the log file; if this is None then
              informational/error messages will be written to stdout
        
        Returns:
          nFill - number of pixels that were flood filled
        
        Notes:
          1. The default lower threshold for flood filling is 75% burn
             probability.
        """
    
        # set up array to hold filled region labels, initialize to 0s which
        # means unburned
        bp_regions = numpy.zeros_like(bp_image, dtype=numpy.int32)
        
        # find regions to start the flood fill from; these regions are seed
        # pixels that are greater than the seed probability threshold
        bp_seeds = bp_image >= seed_prob_thresh    
        
        # group the seed pixels into regions of connected components; these
        # regions will be the start of the flood-fill algorithm
        bp_seed_regions = numpy.zeros_like(bp_seeds, dtype=numpy.int32)
        n_seed_labels = scipy.ndimage.label(bp_seeds, output=bp_seed_regions)
        msg = 'Found %d seeds to use for flood fill' % n_seed_labels
        logIt (msg, log_handler)
        
        # get list of region pixel coordinates, use the first pixel from each
        # as the seed for the region
        bp_region_coords = skimage.measure.regionprops(  \
            label_image=bp_seed_regions, properties=['Area','Coordinates'])
        print 'DEBUG: There are', len(bp_region_coords),   \
            'coordinates identified.'
        
        # loop through regions and flood fill to expand them where they are of
        # an appropriate size
        for i in range(0, len(bp_region_coords)):
            temp_label = bp_region_coords[i]['Label'] 
            temp_area = bp_region_coords[i]['Area']
            temp_coords = bp_region_coords[i]['Coordinates'][0]
            
            # if the number of pixels in this region exceeds the seed size
            # threshold then process the region by flood-filling to grow the
            # burn area
            if temp_area >= seed_size_thresh:
                col = temp_coords[1]
                row = temp_coords[0]
                
                nFilled = self.floodFill(input_image=bp_image, row=row,  \
                    col=col, output_image=bp_regions, output_label=temp_label, \
                    local_threshold=flood_fill_prob_thresh, nodata=-9999)
                
                if False:
                    print '#############################################'  \
                          '###############'
                    print 'Seed region label:', temp_label
                    print 'Number of pixels in region:', temp_area
                    print 'First coordinate:', temp_coords
                    print 'Filled pixels:', nFilled
        
        
        # find region properties for the flood filled burn areas
        bc2 = bp_regions > 0
        bp_regions2 = numpy.zeros_like(bc2, dtype=numpy.int32)
        n_labels = scipy.ndimage.label(bc2, output=bp_regions2)
        
        prop_names = ['Area','FilledArea','MaxIntensity','MeanIntensity',  \
            'MinIntensity']       # ,'Solidity','ConvexArea']
        bp_region2_props = skimage.measure.regionprops(  \
            label_image=bp_regions2, properties=prop_names,  \
            intensity_image=bp_image)
        
        # define the RAT (raster attribute table)
        #print 'Creating raster attribute table...'
        label_rat = gdal.RasterAttributeTable()
        label_rat.CreateColumn("Value", gdalconst.GFT_Integer,  \
            gdalconst.GFU_MinMax)
        
        for prop in prop_names:
            label_rat.CreateColumn(prop, gdalconst.GFT_Real,  \
                gdalconst.GFU_MinMax)
            
        # resize the RAT
        label_rat.SetRowCount(n_labels)
        
        # set values in the RAT
        #print 'Populating raster attribute table...'
        for i in range(0, n_labels):
            # label id
            label_rat.SetValueAsInt(i, 0, bp_region2_props[i]['Label'])
            
            for j in range(0, len(prop_names)):
                temp_prop = prop_names[j]
                label_rat.SetValueAsDouble(i, j+1,  \
                    float(bp_region2_props[i][temp_prop]))
        
        return ([bp_regions2, label_rat])
    
    
    def runBurnThreshold(self, stack_file=None, input_dir=None,
        output_dir=None, start_year=None, end_year=None, seed_prob_thresh=97.5,
        seed_size_thresh=5, flood_fill_prob_thresh=75, make_histos=False,
        logfile=None):
        """Runs the burn thresholding algorithm to find the burn scars from the
           input burn probabilities.
        Description: routine to find the burn scars using the flood-fill
            approach.  Seed pixels are found by using the seed threshold.  Any
            pixels with a probability higher than the seed threshold are
            identified as seed pixels.  Any area with more than the seed size
            threshold is used as a seed area for growing the burn extent using
            the flood fill process.  Areas with fewer than the seed size are
            ignored and not flagged as burn areas.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on Nov. 26, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use int32 arrays vs. int64 arrays for the burn
              classification images
          Updated on Dec. 2, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use argparser vs. optionparser, since optionparser
              is deprecated.
        
        Args:
          stack_file - input CSV file with information about the files to be
              processed.  this is generated as part of the seasonal summaries
              application.
          input_dir - location of the burn probability files
          output_dir - location to write the output burn classifications
          start_year - starting year of the stack_file to process; default is
              to start with the lowest year + 1
          end_year - ending year of the stack_file to process; default is to
              end with the highest year
          seed_prob_thresh - threshold to be used to identify burn pixels
              in the burn probability image as seed pixels for the burn area;
              default is 97.5%
          seed_size_thresh - threshold to be used to identify burn areas in the
              probability image. If the count of burn probability seed pixels
              is greater than this threshold for a certain area then the burn
              area is left and flood-filled.  If the count is less than the
              seed threshold, then this is a false positive and the area is
              not used in the burn classification; default is 5 pixels
          flood_fill_prob_thresh - threshold to be used to add burn pixels
              from the burn probability image to the burn classification via
              flood filling; default is 75%
          make_histos - process histograms and overviews for each of the output
              GeoTiffs generated by this application; default is False
          logfile - name of the logfile for logging information; if None then
              the output will be written to stdout
        
        Returns:
            ERROR - error running the burn threshold application
            SUCCESS - successful processing
        """

        # if no parameters were passed then get the info from the command line
        if stack_file is None:
            # get the command line argument for the input parameters
            parser = ArgumentParser(  \
                description='Process burn scars using thresholds')
            parser.add_argument ('-c', '--stack_file', type=str,
                dest='stack_file',
                help='input file, csv delimited, each row contains '  \
                     'information about a landsat image',
                metavar='FILE', required=True)
            parser.add_argument ('-i', '--input_dir', type=str,
                dest='input_dir',
                help='input directory, location to find input burn '  \
                     'probability files',
                metavar='DIR', required=True)
            parser.add_argument ('-o', '--output_dir', type=str,
                dest='output_dir',
                help='output directory, location to write burn '  \
                     'classification files',
                metavar='DIR', required=True)
            parser.add_argument ('-b', '--start_year', type=int,
                dest='start_year',
                help='starting year for processing the stack of burn '  \
                     'probabilities; default is to use the minimum year '  \
                     'of the files in the stack_file',
                metavar='YEAR')
            parser.add_argument ('-e', '--end_year', type=int, dest='end_year',
                help='ending year for processing the stack of burn '  \
                    'probabilities; default is to use the maximum year '  \
                    'of the files in the stack_file',
                metavar='YEAR')
            parser.add_argument ('-d', '--seed_prob_thresh', type=float,
                dest='seed_prob_thresh',
                help='probability threshold used to seed patch locations; '  \
                    'default is 97.5%%',
                metavar='THRESHOLD')
            parser.add_argument ('-s', '--seed_size_thresh', type=int,
                dest='seed_size_thresh',
                help='minimum size of seed patches, in pixels; default is 5',
                metavar='THRESHOLD')
            parser.add_argument ('-f', '--flood_fill_prob_thresh', type=float,
                dest='flood_fill_prob_thresh',
                help='probability threshold used to grow seeds, pixels '  \
                    'with probabilities above this value are added to '  \
                    'the patch; default is 75%%',
                metavar='THRESHOLD')
            parser.add_argument ('--make_histos', dest='make_histos',
                default=False, action='store_true',
                help='process histograms and overviews for each of '  \
                    'the GeoTiffs generated by this application; default '  \
                    'is False')
            parser.add_argument ('-l', '--logfile', type=str, dest='logfile',
                help='name of optional log file', metavar='FILE')

            options = parser.parse_args()

            # validate command-line options and arguments
            stack_file = options.stack_file
            if stack_file is None:
                parser.error ("missing CSV stack file cmd-line argument")
                return ERROR

            input_dir = options.input_dir
            if input_dir is None:
                parser.error ("missing input directory cmd-line argument")
                return ERROR

            output_dir = options.output_dir
            if output_dir is None:
                parser.error ("missing output directory cmd-line argument")
                return ERROR

            if options.start_year is not None:
                start_year = options.start_year

            if options.end_year is not None:
                end_year = options.end_year

            if options.seed_prob_thresh is not None:
                seed_prob_thresh = options.seed_prob_thresh
    
            if options.seed_size_thresh is not None:
                seed_size_thresh = options.seed_size_thresh
    
            if options.flood_fill_prob_thresh is not None:
                flood_fill_prob_thresh = options.flood_fill_prob_thresh

            if options.make_histos is not None:
                make_histos = options.make_histos

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile is not None:
            log_handler = open (logfile, 'w', buffering=1)

        # validate options and arguments
        if start_year is not None:
            if (start_year < 1984) | (start_year > 2011):
                msg = 'start_year falls outside 1984-2011: %d' % start_year
                logIt (msg, log_handler)
                return ERROR
    
        if end_year is not None:
            if (end_year < 1984) | (end_year > 2011):
                msg = 'end_year falls outside 1984-2011: %d' % end_year
                logIt (msg, log_handler)
                return ERROR
    
        if (end_year is not None) & (start_year is not None):
            if end_year < start_year:
                msg = 'end_year (%d) is less than start_year (%d)' %  \
                    (end_year, start_year)
                logIt (msg, log_handler)
                return ERROR
            
        if not os.path.exists(stack_file):
            msg = 'CSV stack file does not exist: ' + stack_file
            logIt (msg, log_handler)
            return ERROR
    
        if not os.path.exists(input_dir):
            msg = 'Input directory does not exist: ' + input_dir
            logIt (msg, log_handler)
            return ERROR
    
        if not os.path.exists(output_dir):
            msg = 'Output directory does not exist: %s. Creating ...' % \
                output_dir
            logIt (msg, log_handler)
            os.makedirs(output_dir, 0755)

        # save the current working directory for return to upon error or when
        # processing is complete
        mydir = os.getcwd()
        msg = 'Changing directories for burn threshold processing: ' +  \
            output_dir
        logIt (msg, log_handler)
        os.chdir (output_dir)

        # open the stack file
        stack = numpy.recfromcsv(stack_file, delimiter=",", names=True,  \
            dtype="string")
        
        # use the minimum and maximum years in the stack if the start year and
        # end year were not specified on the command line.  start year needs
        # to be one more than the actual starting year in the stack since the
        # burn products require a previous year to process.  so the start year
        # for the burn products is one year after the actual starting year in
        # the stack.
        if start_year is None:
            start_year = numpy.min(stack['year']) + 1
        
        if end_year is None:
            end_year = numpy.max(stack['year'])
        
        stack_mask = (stack['year'] >= start_year) & (stack['year'] <= end_year)
        stack2 = stack[ stack_mask, :]
        
        # read the input data from the stack, for the years specified
        msg = 'Processing burn probabilities for %d-%d' % (start_year, end_year)
        logIt (msg, log_handler)
        msg = '  seed probability threshold: %f' % seed_prob_thresh
        logIt (msg, log_handler)
        msg = '  seed size threshold: %d' % seed_size_thresh
        logIt (msg, log_handler)
        msg = '  flood fill probability threshold: %f' %  \
            flood_fill_prob_thresh
        logIt (msg, log_handler)

        for i in range(0, stack2.shape[0]):
            msg = '############################################################'
            logIt (msg, log_handler)
            file_name = stack2['file_'][i]
            
            # use the lndsr filename in the CSV file to obtain the burn
            # probability filename
            fname = os.path.basename(file_name).replace('lndsr.','')
            fname = fname.replace('.hdf','_burn_probability.hdf')
            bp_file_name = input_dir + "/" + fname
            if not os.path.exists(bp_file_name):
                msg = 'burn probability file does not exist: ' +  bp_file_name
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR

            
            # determine the output classification name
            fname = os.path.basename(bp_file_name).replace(  \
                'burn_probability.hdf', 'burn_class.tif')
            bc_file_name = output_dir + "/" + fname
            
            # process the current burn probability file
            msg = 'Reading file ... ' + bp_file_name
            logIt (msg, log_handler)
            bp_dataset = gdal.Open(bp_file_name)
            if bp_dataset is None:
                msg = 'Failed to open bp file: ' + bp_file_name
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR
            
            # read the only band in the file, band 1
            bp_band = bp_dataset.GetRasterBand(1)
            if bp_band is None:
                msg = 'Failed to open bp band 1 from ' + bp_file_name
                logIt (msg, log_handler)
                os.chdir (mydir)
                return ERROR
            
            # given that all burn probabilities in this temporal stack have the
            # same scene extents and projection information, just obtain that
            # information from the first file and use it for all of the files
            if i == 0:
                geotrans = bp_dataset.GetGeoTransform()
                prj = bp_dataset.GetProjectionRef()
                nrow = bp_dataset.RasterYSize
                ncol = bp_dataset.RasterXSize
                nodata = bp_band.GetNoDataValue()
                
                # array to hold burn scars
                bp_scars = numpy.zeros((nrow, ncol))
                bp_rats = []
            
            # read the probabilities for the current scene
            bp_data = bp_band.ReadAsArray()
            
            # find the final burn scars from the burn probabilities
            bp_scar_results = self.findBurnScars(bp_data, seed_prob_thresh,
                seed_size_thresh, flood_fill_prob_thresh, log_handler)
            bp_scars = bp_scar_results[0]
            bp_scars[ bp_data < 0 ] = bp_data[ bp_data < 0 ]
            bp_rats.append(bp_scar_results[1])
                
            # output the burn classifications for this scene
            msg = 'Writing output to %s ... ' % bc_file_name
            logIt (msg, log_handler)
            self.writeResults(outputData=bp_scar_results[0],
                outputFilename=bc_file_name, geotrans=geotrans, prj=prj,
                nodata=nodata, outputRAT=bp_scar_results[1],
                makeHistos=make_histos)

        # successful completion.  return to the original directory.
        msg = 'Completion of burn threshold.'
        logIt (msg, log_handler)
        if logfile is not None:
            log_handler.close()
        os.chdir (mydir)
        return SUCCESS

######end of BurnAreaThreshold class######

if __name__ == "__main__":
    sys.exit (BurnAreaThreshold().runBurnThreshold())
