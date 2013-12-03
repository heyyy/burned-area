#! /usr/bin/env python
# from sys import path
from numpy import *
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst
import os
import time
from log_it import *


def polishImage (input_file, log_handler=None):
    """Calculates histograms for each band and builds the pyramids.
    Description: polishImage will open the input GeoTiff file, calculate the
        histogram for each band, and build the pyramids.
    
    History:
      Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
          Geographic Science Center
      Updated on 4/30/2013 by Gail Schmidt, USGS/EROS LSRD Project
          Modified to utilize a log file if passed along.
    
    Args:
      input_file - name of the input HDF reflectance file to be converted
      output_file - name of the output GeoTiff file which contains bands 1-7
          and a QA band
      log_handler - open log file for logging or None for stdout
    
    Returns:
        ERROR - error converting the HDF file to GeoTiff
        SUCCESS - successful processing
    """

    # test to make sure the input file exists
    if not os.path.exists(input_file):
        msg = 'Input file does not exist: ' + input_file
        logIt (msg, log_handler)
        return ERROR
    
    # open the input GeoTiff file
    input_scene = TIF_Scene_8_band(input_file, log_handler)
    if input_scene is None:
        msg = 'Error reading the GeoTiff file: ' + input_file
        logIt (msg, log_handler)
        return ERROR
    
    # create the histograms for each band
    histogram = input_scene.band1.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band1.SetDefaultHistogram(histogram[0], histogram[1],  \
            histogram[3])

    histogram = input_scene.band2.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band2.SetDefaultHistogram(histogram[0], histogram[1],  \
            histogram[3])

    histogram = input_scene.band3.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band3.SetDefaultHistogram(histogram[0], histogram[1],  \
            histogram[3])

    histogram = input_scene.band4.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band4.SetDefaultHistogram(histogram[0], histogram[1],  \
            histogram[3])

    histogram = input_scene.band5.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band5.SetDefaultHistogram(histogram[0], histogram[1],  \
            histogram[3])

    histogram = input_scene.band6.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band6.SetDefaultHistogram(histogram[0], histogram[1],  \
            histogram[3])

    histogram = input_scene.band7.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band7.SetDefaultHistogram(histogram[0], histogram[1],  \
            histogram[3])

    histogram = input_scene.band_QA.GetDefaultHistogram()
    if not histogram is None:
        input_scene.band_QA.SetDefaultHistogram(histogram[0], histogram[1], \
            histogram[3])

    # build pyramids
    gdal.SetConfigOption('HFA_USE_RRD', 'YES')
    input_scene.dataset.BuildOverviews(overviewlist=[3,9,27,81,243,729])

    return SUCCESS


#############################################################################
# Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
#       Geographic Science Center
# Created Python script to open and read the input single band GeoTiff file
# to obtain particular attributes and data from the file.
#
# History:
#   Updated on 4/26/2013 by Gail Schmidt, USGS/EROS
#       Modified to ....
############################################################################
class TIF_Scene_1_band:
    """Class to work with the single band GeoTiff files.
    """

    filename = ""
    NorthBoundingCoordinate = 0
    SouthBoundingCoordinate = 0
    WestBoundingCoordinate = 0
    EastBoundingCoordinate = 0
    dX = 0
    dY = 0
    NRow = 0
    NCol = 0
    NBands = 0
    NoData = 0
    
    # not filled at the moment
    WRS_Path = 0
    WRS_Row = 0
    Satellite = ""
    SolarAzimuth = 0
    SolarZenith = 0
    AcquisitionDate = ""
    month = 0
    day = 0
    year = 0

    # datasets created by gdal.Open
    dataset = None

    # bands
    band1 = None

    def __init__(self, fname, band=1, log_handler=None):
        """Class constructor to open and read metadata of the GeoTiff file.
        Description: class constructor verifies the input file exists, then
            opens it, reads the metadata, and establishes pointers to the
            desired band.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 4/30/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize a log file if passed along.
        
        Args:
          fname - name of the input GeoTiff file to be processed
          log_handler - open log file for logging or None for stdout
        
        Returns:
            None - error opening or reading the file via GDAL
            Object - successful processing
        """

        if not os.path.exists(fname):
            msg = 'Input file does not exist: ' + fname
            logIt (msg, log_handler)
            return None

        self.filename=fname                      # store the filename
        self.dataset=gdal.Open(self.filename)    # open the file with GDAL
        if self.dataset is None:
            msg = 'GDAL could not open input file: ' + self.filename
            logIt (msg, log_handler)
            return None

        # create connections to the bands
        self.NBands = self.dataset.RasterCount
        self.band1 = self.dataset.GetRasterBand(band)

        # get coordinate information
        gt = self.dataset.GetGeoTransform()
        self.NCol = self.dataset.RasterXSize
        self.dX = gt[1]
        self.WestBoundingCoordinate = gt[0]
        self.EastBoundingCoordinate = self.WestBoundingCoordinate +  \
            (self.NCol * self.dX)

        self.NRow = self.dataset.RasterYSize        
        self.dY = gt[5]
        self.NorthBoundingCoordinate = gt[3]
        self.SouthBoundingCoordinate = self.NorthBoundingCoordinate +  \
            (self.NRow * self.dY)

        self.NoData = self.band1.GetNoDataValue()


    def __del__(self):
        """Class constructor to clear the bands.
        Description: class destructor cleans up all the sub dataset and band
            pointers.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args: None
        
        Returns: Nothing
        """

        self.dataset = None
        self.band1 = None

    
    def xy2ij(self, x, y):
        """Converts projection space to pixel space.
        Description: xy2ij converts projection coordinates (x,y) to pixel
            space (i,j)
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args:
          x - x projection coordinate
          y - y projection coordinate
        
        Returns:
            (col, row) - i,j pixel referring to x,y coordinate
        """

        col = int((x - self.WestBoundingCoordinate) / self.dX)
        row = int((y - self.NorthBoundingCoordinate) / self.dY)
        return([col, row])


    def ij2xy(self, col, row):
        """Converts pixel space to projection space.
        Description: ij2xy converts pixel space values (i,j) to projection
            coordinates (x,y)
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args:
          row - row (line) pixel space coordinate
          col - column (sample) pixel space coordinate
        
        Returns:
            (x, y) - x,y projection coord referring to pixel i,j
        """

        x = self.WestBoundingCoordinate + (col * self.dX)
        y = self.NorthBoundingCoordinate + (row * self.dY)
        return([x,y])


    def getRowOfBandValues(self, y):
        """Reads the specified line at specified projection y coordinate.
        Description: getRowOfBandValues reads a row of band values from band1
            at the associated row for the projection y coordinate.  The
            projection y coordinate is converted to the actual row/line in the
            current scene, and then that row is read (reading all the samples
            in the line) and returned.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 5/1/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to validate the y projection coordinate falls within
              the current scene.
        
        Args:
          y - y projection coordinate
        
        Returns:
            None - if pixel location does not fall within the coordinates of
                this scene
            Array - associated line of data y location as band1
        """

        if (y < self.NorthBoundingCoordinate) and  \
           (y > self.SouthBoundingCoordinate):
            # get the i,j pixel (line) for the y projection coordinates
            ij = self.xy2ij(0, y)

            # read the line from band1 in the the scene
            x1 = self.band1.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
        
            return (x1)
        else:
            return (None)


    def getBandValues(self):
        """Reads the specified band of data.
        Description: getBandValues reads an entire band of data for the
            band 1.
        
        History:
          Created on 5/1/2013 by Gail Schmidt, USGS/EROS LSRD Project
        
        Args: None
        
        Returns:
            Array - associated data for band 1
        """

        x = zeros ((self.NRow, self.NCol))
        return (self.band1.ReadAsArray())
        
#### end TIF_Scene_1_band class ####

        
#############################################################################
# Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
#       Geographic Science Center
# Created Python script to open and read the input eight-band GeoTiff file
# to obtain particular attributes and data from the file.
#
# History:
#   Updated on 4/26/2013 by Gail Schmidt, USGS/EROS
#       Modified to ....
############################################################################
class TIF_Scene_8_band:
    """Class to work with the eight-band GeoTiff files.
    """

    filename = ""
    NorthBoundingCoordinate = 0
    SouthBoundingCoordinate = 0
    WestBoundingCoordinate = 0
    EastBoundingCoordinate = 0
    dX = 0
    dY = 0
    NRow = 0
    NCol = 0
    NBands = 0
    
    # not filled at the moment
    WRS_Path = 0
    WRS_Row = 0
    Satellite = ""
    SolarAzimuth = 0
    SolarZenith = 0
    AcquisitionDate = ""
    month = 0
    day = 0
    year = 0

    # datasets created by gdal.Open
    dataset = None

    # bands
    band1 = None
    band2 = None
    band3 = None
    band4 = None
    band5 = None
    band6 = None
    band7 = None
    band_QA = None

    def __init__(self, fname, log_handler=None):
        """Class constructor to open and read metadata of the GeoTiff file.
        Description: class constructor verifies the input file exists, then
            opens it, reads the metadata, and establishes pointers to the
            desired band.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 4/30/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to utilize a log file if passed along.
        
        Args:
          fname - name of the input GeoTiff file to be processed
          log_handler - open log file for logging or None for stdout
        
        Returns:
            None - error opening or reading the file via GDAL
            Object - successful processing
        """

        if not os.path.exists(fname):
            msg = 'Input file does not exist: ' + fname
            logIt (msg, log_handler)
            return None

        self.filename=fname                      # store the filename
        self.dataset=gdal.Open(self.filename)    # open the file with GDAL
        if self.dataset is None:
            msg = 'GDAL could not open input file: ' + self.filename
            logIt (msg, log_handler)
            return None

        # create connections to the bands
        self.NBands = self.dataset.RasterCount
        self.band1 = self.dataset.GetRasterBand(1)
        self.band2 = self.dataset.GetRasterBand(2)
        self.band3 = self.dataset.GetRasterBand(3)
        self.band4 = self.dataset.GetRasterBand(4)
        self.band5 = self.dataset.GetRasterBand(5)
        self.band6 = self.dataset.GetRasterBand(6)
        self.band7 = self.dataset.GetRasterBand(7)
        self.band_QA = self.dataset.GetRasterBand(8)

        # get coordinate information from band 1
        gt = self.dataset.GetGeoTransform()
        self.NCol = self.dataset.RasterXSize
        self.dX = gt[1]
        self.WestBoundingCoordinate = gt[0]
        self.EastBoundingCoordinate = self.WestBoundingCoordinate +  \
            (self.NCol * self.dX)

        self.NRow = self.dataset.RasterYSize        
        self.dY = gt[5]
        self.NorthBoundingCoordinate = gt[3]
        self.SouthBoundingCoordinate = self.NorthBoundingCoordinate +  \
            (self.NRow * self.dY)

    
    def __del__(self):
        """Class constructor to clear the bands.
        Description: class destructor cleans up all the sub dataset and band
            pointers.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args: None
        
        Returns: Nothing
        """

        self.dataset = None
        self.band1 = None
        self.band2 = None
        self.band3 = None
        self.band4 = None
        self.band5 = None
        self.band6 = None
        self.band7 = None
        self.band_QA = None


    def xy2ij(self, x, y):
        """Converts projection space to pixel space.
        Description: xy2ij converts projection coordinates (x,y) to pixel
            space (i,j)
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args:
          x - x projection coordinate
          y - y projection coordinate
        
        Returns:
            (col, row) - i,j pixel referring to x,y coordinate
        """

        col = int((x - self.WestBoundingCoordinate) / self.dX)
        row = int((y - self.NorthBoundingCoordinate) / self.dY)
        return([col, row])


    def ij2xy(self, col, row):
        """Converts pixel space to projection space.
        Description: ij2xy converts pixel space values (i,j) to projection
            coordinates (x,y)
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args:
          row - row (line) pixel space coordinate
          col - column (sample) pixel space coordinate
        
        Returns:
            (x, y) - x,y projection coord referring to pixel i,j
        """

        x = self.WestBoundingCoordinate + (col * self.dX)
        y = self.NorthBoundingCoordinate + (row * self.dY)
        return([x,y])


    def getStackBandValues(self, x, y):
        """Reads the stack of pixel values at the specified location.
        Description: getStackBandValues reads a stack of pixel values (in the Z
            direction) at the specified x,y location
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
        
        Args:
          x - x projection coordinate
          y - y projection coordinate
        
        Returns:
            None - if pixel location does not fall within the coordinates of
                this scene
            pixel stack - stack of values for the x,y location as band1, band2,
                band3, band4, band5, band6, band7, and QA band
        """

        if (x < self.EastBoundingCoordinate) and  \
           (x > self.WestBoundingCoordinate) and  \
           (y < self.NorthBoundingCoordinate) and \
           (y > self.SouthBoundingCoordinate):

            ij = self.xy2ij(x,y)

            x1 = self.band1.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x2 = self.band2.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x3 = self.band3.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x4 = self.band4.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x5 = self.band5.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x6 = self.band6.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x7 = self.band7.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            QA = self.band_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
                
            return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4, \
                'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA} )
        else:
            return (None)

        
    def getRowOfBandValues(self, y):
        """Reads a line of pixel values for each band at the specified y
           coordinate.
        Description: getRowOfBandValues reads a row of band values for each
            of the bands specified at the associated row for the projection
            y coordinate.  The projection y coordinate is converted to the
            actual row/line in the current scene, and then that row is read
            (reading all the samples in the line) and returned.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on 5/1/2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to validate the y projection coordinate falls within
              the current scene.
        
        Args:
          y - y projection coordinate
        
        Returns:
            None - if pixel location does not fall within the coordinates of
                this scene
            Array - associated line of data y location as band1
        """

        if (y < self.NorthBoundingCoordinate) and  \
           (y > self.SouthBoundingCoordinate):

            # get the i,j pixel (line) for the y projection coordinates
            ij = self.xy2ij(0, y)
    
            # read the line from each of the bands in the scene
            x1 = self.band1.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x2 = self.band2.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x3 = self.band3.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x4 = self.band4.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x5 = self.band5.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x6 = self.band6.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x7 = self.band7.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            QA = self.band_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            
            return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4,  \
                'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA } )
        else:
            return (None)


    def getBandValues(self):
        """Read all 8 bands from the GeoTiff file.
        Description: getBandValues reads an entire band of data for all the
            bands in the GeoTiff file.
        
        History:
          Created on 5/1/2013 by Gail Schmidt, USGS/EROS LSRD Project
        
        Args: None
        
        Returns:
            Array - associated data for band 1
        """

        x = zeros((8,self.NRow,self.NCol))
        
        x[0,:,:] = self.band1.ReadAsArray()
        x[1,:,:] = self.band2.ReadAsArray()
        x[2,:,:] = self.band3.ReadAsArray()
        x[3,:,:] = self.band4.ReadAsArray()
        x[4,:,:] = self.band5.ReadAsArray()
        x[5,:,:] = self.band6.ReadAsArray()
        x[6,:,:] = self.band7.ReadAsArray()
        x[7,:,:] = self.band_QA.ReadAsArray()
        
        return(x)

#### end TIF_Scene_8_band class ####
