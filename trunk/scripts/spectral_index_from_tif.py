from sys import path
import os, sys, time, datetime, getopt

from numpy import *
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst
from spectral_indices import *
from log_it import *


#############################################################################
# Created on May 1, 2013 by Gail Schmidt, USGS/EROS
# Created class to hold the methods and attributes for reading the GeoTIFF
# file and generating spectral indices.
#
# History:
#
############################################################################
class spectralIndex:
    # Data attributes
    tif_file = "None"    # base GeoTIFF file to be processed for
                         # spectral indices
    input_ds = None      # pointer to input GeoTIFF file
    input_band1 = None   # pointer to GeoTIFF band
    input_band2 = None   # pointer to GeoTIFF band
    input_band3 = None   # pointer to GeoTIFF band
    input_band4 = None   # pointer to GeoTIFF band
    input_band5 = None   # pointer to GeoTIFF band
    input_band6 = None   # pointer to GeoTIFF band
    input_band7 = None   # pointer to GeoTIFF band
    input_band_qa = None  # pointer to GeoTIFF QA
    b1 = None            # band1 data from GeoTIFF file
    b2 = None            # band2 data from GeoTIFF file
    b3 = None            # band3 data from GeoTIFF file
    b4 = None            # band4 data from GeoTIFF file
    b5 = None            # band5 data from GeoTIFF file
    b6 = None            # band6 data from GeoTIFF file
    b7 = None            # band7 data from GeoTIFF file
    qa = None            # QA data from GeoTIFF file

    ########################################################################
    # Description: spectralIndex class constructor opens the input GeoTIFF
    #     file and obtains pointers to each of the desired bands.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 5/2/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to utilize the class constructor to open and set up
    #       band pointers from the file, so they are available for use by
    #       other methods within the class.
    #
    # Inputs:
    #   tif_file - GeoTIFF file to be opened and processed for this class
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     None - error opening the file
    #     Object - successful processing
    #
    # Notes:
    #######################################################################
    def __init__ (self, tif_file, log_handler=None):
        # Check to make sure the input file exists
        if not os.path.exists (tif_file):
            msg = 'GeoTIFF file does not exist: ' + tif_file
            logIt (msg, log_handler)
            return None
        self.tif_file = tif_file
    
        # Define the image to work with, create connections to the various
        # bands in the image
        self.input_ds = gdal.Open( tif_file, gdalconst.GA_ReadOnly )
        if self.input_ds is None:
            msg = 'Failed to open input file: ' + tif_file
            logIt (msg, log_handler)
            return None

        self.input_band1 = self.input_ds.GetRasterBand(1)
        self.input_band2 = self.input_ds.GetRasterBand(2)
        self.input_band3 = self.input_ds.GetRasterBand(3)
        self.input_band4 = self.input_ds.GetRasterBand(4)
        self.input_band5 = self.input_ds.GetRasterBand(5)
        self.input_band6 = self.input_ds.GetRasterBand(6)
        self.input_band7 = self.input_ds.GetRasterBand(7)
        self.input_band_qa = self.input_ds.GetRasterBand(8)


    ########################################################################
    # Description: class destructor cleans up all the sub dataset and band
    #     pointers.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #
    # Inputs: None
    #
    # Returns: Nothing
    #######################################################################
    def __del__ (self):
        # cleanup
        self.input_band1 = None
        self.input_band2 = None
        self.input_band3 = None
        self.input_band4 = None
        self.input_band5 = None
        self.input_band6 = None
        self.input_band7 = None
        self.input_band_qa = None
        self.b1 = None
        self.b2 = None
        self.b3 = None
        self.b4 = None
        self.b5 = None
        self.b6 = None
        self.b7 = None


    ########################################################################
    # Description: createSpectralIndex creates the desired spectral index
    #     product.  If mask is specified, then a combined mask file is
    #     generated using the various input masks.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 5/2/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to utilize a class structure and only read the bands
    #       if they haven't been read already.  This saves from duplication
    #       of reading the same band over and over for different indices.
    #
    # Inputs:
    #   output_file - name of spectral index file to create
    #   index - name of the index product to be generated (ndvi, nbr, nbr2,
    #       ndmi, mask)
    #   make_histos - should histograms and overview pyramids be generated
    #       for each of the output GeoTIFF files?
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     ERROR - error generating the spectral index or mask
    #     SUCCESS - successful processing
    #
    # Notes:
    #######################################################################
    def createSpectralIndex (self, output_file, index, make_histos=False, \
        log_handler=None):
        startTime = time.time()
        print '    Processing index: ' + index
    
        # ignore divide by zero and invalid (NaN) values when doing array
        # division.  these will be handled on our own.
        seterr(divide='ignore', invalid='ignore')

        # figure out which spectral index to generate
        if not (index in ['ndvi','nbr','nbr2','ndmi','mask']):
            msg = 'Algorithm for ' + index + ' is not implemented'
            logIt (msg, log_handler)
            return ERROR

        # create the output folder if it does not exist
        output_dir = os.path.dirname(output_file)
        if not os.path.exists(output_dir):
            msg = 'Creating output directory ' + output_dir
            logIt (msg, log_handler)
            os.makedirs(output_dir)
            
        # create the output file; spectral indices are multiplied by 1000.0
        # and the mask file is as-is.
        driver = gdal.GetDriverByName("GTiff")
        output_ds = driver.Create( output_file, self.input_ds.RasterXSize, \
            self.input_ds.RasterYSize, 1, gdal.GDT_Int16)
        output_ds.SetGeoTransform( self.input_ds.GetGeoTransform() )
        output_ds.SetProjection( self.input_ds.GetProjection() )
        output_band = output_ds.GetRasterBand(1)    
    
        # grab the noData value from the input GeoTIFF file
        nodata = self.input_band1.GetNoDataValue()
        if nodata is None:
            nodata = -9999
            
        # read the QA data if it hasn't been read already
        if self.qa == None:
            self.qa = self.input_band_qa.ReadAsArray()

        # calculate the spectral index
        if index == 'nbr':
            if self.b4 == None:
                self.b4 = self.input_band4.ReadAsArray()
            if self.b7 == None:
                self.b7 = self.input_band7.ReadAsArray()
            newVals = 1000.0 * NBR(self.b4, self.b7, nodata)
            newVals[ self.qa < 0 ] = nodata
        elif index == 'nbr2':
            if self.b5 == None:
                self.b5 = self.input_band5.ReadAsArray()
            if self.b7 == None:
                self.b7 = self.input_band7.ReadAsArray()
            newVals = 1000.0 * NBR2(self.b5, self.b7, nodata)
            newVals[ self.qa < 0 ] = nodata
        elif index == 'ndmi':
            if self.b4 == None:
                self.b4 = self.input_band4.ReadAsArray()
            if self.b5 == None:
                self.b5 = self.input_band5.ReadAsArray()
            newVals = 1000.0 * NDMI(self.b4, self.b5, nodata)
            newVals[ self.qa < 0 ] = nodata
        elif index == 'ndvi':
            if self.b4 == None:
                self.b4 = self.input_band4.ReadAsArray()
            if self.b3 == None:
                self.b3 = self.input_band3.ReadAsArray()
            newVals = 1000.0 * NDVI(self.b3, self.b4, nodata)
            newVals[ self.qa < 0 ] = nodata
        else:   # save the mask
            newVals = self.qa
            
        # write the output 
        output_band.WriteArray(newVals)
        output_band.SetNoDataValue(nodata)

        # create histograms and pyramids
        if make_histos:
            # make histogram
            histogram = output_band.GetDefaultHistogram()
            if not histogram == None:
                output_band.SetDefaultHistogram(histogram[0], histogram[1], \
                histogram[3])
    
            # build pyramids
            gdal.SetConfigOption('HFA_USE_RRD', 'YES')
            output_ds.BuildOverviews(overviewlist=[3,9,27,81,243,729])
    
        # cleanup
        output_band = None
        output_ds = None

        endTime = time.time()
        msg = '    Processing time = ' + str(endTime-startTime) + ' seconds'
        logIt (msg, log_handler)
