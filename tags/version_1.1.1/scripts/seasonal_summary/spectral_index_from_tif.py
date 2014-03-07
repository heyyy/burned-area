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
#   Updated on 5/21/2013 by Gail Schmidt, USGS/EROS LSRD Project
#       Modified to process all the indices one line  at a time (vs. the
#       entire band) since this is faster.
############################################################################
class spectralIndex:
    """Class for producing the spectral indices.
    """

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

    def __init__ (self, tif_file, log_handler=None):
        """Class constructor which opens the GeoTiff file.
        Description: spectralIndex class constructor opens the input GeoTiff
            file and obtains pointers to each of the desired bands.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 5/2/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize the class constructor to open and set up
              band pointers from the file, so they are available for use by
              other methods within the class.
        
        Args:
          tif_file - GeoTiff file to be opened and processed for this class
          log_handler - open log file for logging or None for stdout
        
        Returns:
            None - error opening the file
            Object - successful processing
        """

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


    def __del__ (self):
        """Class desctructor to clean up band pointers.
        Description: class destructor cleans up all the sub dataset and band
            pointers.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args: None
        
        Returns: Nothing
        """

        # cleanup
        self.input_band1 = None
        self.input_band2 = None
        self.input_band3 = None
        self.input_band4 = None
        self.input_band5 = None
        self.input_band6 = None
        self.input_band7 = None
        self.input_band_qa = None


    def createSpectralIndices (self, index_dict, make_histos=False,  \
        log_handler=None):
        """Generates the specified spectral indices.
        Description: createSpectralIndices creates the desired spectral index
            products.  If mask is specified, then a combined mask file is
            generated using the various input masks.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 5/2/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize a class structure and only read the bands
              if they haven't been read already.  This saves from duplication
              of reading the same band over and over for different indices.
          Updated on 5/21/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to process all the indices one line  at a time (vs. the
              entire band) since this is faster.
        
        Args:
          index_dict - dictionary of index types (ndvi, nbr, nbr2, ndmi, mask)
              and the associated filename for the index file
          make_histos - should histograms and overview pyramids be generated
              for each of the output GeoTIFF files?
          log_handler - open log file for logging or None for stdout
        
        Returns:
            ERROR - error generating the spectral indices or mask
            SUCCESS - successful processing
        """

        num_indices = len(index_dict)
        print '    Processing %d indices: ' % num_indices
        for index in index_dict.keys():
            print '      ' + index
    
        # ignore divide by zero and invalid (NaN) values when doing array
        # division.  these will be handled on our own.
        seterr(divide='ignore', invalid='ignore')

        # grab the noData value from the input GeoTIFF file
        ncol = self.input_ds.RasterXSize
        nrow = self.input_ds.RasterYSize
        nodata = self.input_band1.GetNoDataValue()
        if nodata is None:
            nodata = -9999
            
        # loop through the indices specified to be processed and create the
        # output GeoTIFF files
        driver = {}
        output_ds = {}
        output_band = {}
        for index in index_dict.keys():
            # figure out which spectral index to generate
            if not (index in ['ndvi','nbr','nbr2','ndmi','mask']):
                msg = 'Algorithm for %s is not implemented' % index
                logIt (msg, log_handler)
                return ERROR
    
            # create the output folder if it does not exist
            output_dir = os.path.dirname(index_dict.get(index))
            if not os.path.exists(output_dir):
                msg = 'Creating output directory ' + output_dir
                logIt (msg, log_handler)
                os.makedirs(output_dir)
            
            # create the output file; spectral indices are multiplied by 1000.0
            # and the mask file is as-is.
            mydriver = gdal.GetDriverByName("GTiff")
            driver[index] = mydriver
            my_ds = mydriver.Create(index_dict.get(index),  \
                self.input_ds.RasterXSize, self.input_ds.RasterYSize, 1,  \
                gdal.GDT_Int16)
            my_ds.SetGeoTransform(self.input_ds.GetGeoTransform())
            my_ds.SetProjection(self.input_ds.GetProjection())
            output_ds[index] = my_ds
            my_band = my_ds.GetRasterBand(1)    
            my_band.SetNoDataValue(nodata)
            output_band[index] = my_band
        
        # loop through each line in the image and process
        for y in range (0, nrow):
            # read the QA data
            qa = self.input_band_qa.ReadAsArray(0, y, ncol, 1)
    
            # loop through the indices specified and process each index product
            # reusing the line from each band where possible
            b3 = b4 = b5 = b7 = None
            for index in index_dict.keys():
                # calculate the spectral index
                if index == 'nbr':
                    if b4 is None:
                        b4 = self.input_band4.ReadAsArray(0, y, ncol,1)
                    if b7 is None:
                        b7 = self.input_band7.ReadAsArray(0, y, ncol, 1)
                    newVals = 1000.0 * NBR(b4, b7, nodata)
                    newVals[qa < 0] = nodata

                    # write the output 
                    my_output_band = output_band.get(index)
                    my_output_band.WriteArray(newVals, 0, y)

                elif index == 'nbr2':
                    if b5 is None:
                        b5 = self.input_band5.ReadAsArray(0, y, ncol, 1)
                    if b7 is None:
                        b7 = self.input_band7.ReadAsArray(0, y, ncol, 1)
                    newVals = 1000.0 * NBR2(b5, b7, nodata)
                    newVals[qa < 0] = nodata

                    # write the output 
                    my_output_band = output_band.get(index)
                    my_output_band.WriteArray(newVals, 0, y)

                elif index == 'ndmi':
                    if b4 is None:
                        b4 = self.input_band4.ReadAsArray(0, y, ncol, 1)
                    if b5 is None:
                        b5 = self.input_band5.ReadAsArray(0, y, ncol, 1)
                    newVals = 1000.0 * NDMI(b4, b5, nodata)
                    newVals[qa < 0] = nodata

                    # write the output 
                    my_output_band = output_band.get(index)
                    my_output_band.WriteArray(newVals, 0, y)

                elif index == 'ndvi':
                    if b4 is None:
                        b4 = self.input_band4.ReadAsArray(0, y, ncol, 1)
                    if b3 is None:
                        b3 = self.input_band3.ReadAsArray(0, y, ncol, 1)
                    newVals = 1000.0 * NDVI(b3, b4, nodata)
                    newVals[qa < 0] = nodata

                    # write the output 
                    my_output_band = output_band.get(index)
                    my_output_band.WriteArray(newVals, 0, y)

                else:   # save the mask
                    newVals = qa
                    
                    # write the output 
                    my_output_band = output_band.get(index)
                    my_output_band.WriteArray(newVals, 0, y)
                # end if
            # end for index
        # end for y

        # cleanup the bands
        b3 = b4 = b5 = b7 = None

        # create histograms and pyramids
        if make_histos:
            # loop through the indices specified and process histograms
            for index in index_dict.keys():
                # make histogram
                my_output_band = output_band.get(index)
                histogram = my_output_band.GetDefaultHistogram()
                if not histogram is None:
                    my_output_band.SetDefaultHistogram(histogram[0], \
                        histogram[1], histogram[3])
    
                # build pyramids
                gdal.SetConfigOption('HFA_USE_RRD', 'YES')
                my_output_ds = output_ds.get(index)
                my_output_ds.BuildOverviews(  \
                    overviewlist=[3,9,27,81,243,729])
    
        # cleanup
        del (output_band)
        del (output_ds)
        del (driver)

        return SUCCESS
######end of spectralIndex class######
