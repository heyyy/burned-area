#! /usr/bin/env python
from numpy import *
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst
import os
import time
from log_it import *


#############################################################################
# Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
#       Geographic Science Center
# Created Python script to open and read the input HDF file to obtain
# particular attributes from the file.
#
# History:
#   Updated on 4/26/2013 by Gail Schmidt, USGS/EROS
#       Modified to remove the DDV QA from the overall mask.  There is no
#       need to mask out pixels simply due to the fact they are dark dense
#       vegetation.
############################################################################
class HDF_Scene:
    filename = ""
    NorthBoundingCoordinate = 0
    SouthBoundingCoordinate = 0
    WestBoundingCoordinate = 0
    EastBoundingCoordinate = 0
    dX = 0
    dY = 0
    NRow = 0
    NCol = 0
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
    mdata = None
    subsdatasets = None
    subdataset1 = None
    subdataset2 = None
    subdataset3 = None
    subdataset4 = None
    subdataset5 = None
    subdataset6 = None
    subdataset7 = None 
    subdataset_fill_QA = None
    subdataset_cloud_QA = None
    subdataset_shadow_QA = None
    subdataset_snow_QA = None
    subdataset_land_water_QA = None
    subdataset_adjacent_cloud_QA = None

    # bands
    band1 = None
    band2 = None
    band3 = None
    band4 = None
    band5 = None
    band6 = None
    band7 = None
    band_fill_QA = None
    band_cloud_QA = None
    band_shadow_QA = None
    band_snow_QA = None
    band_land_water_QA = None
    band_adjacent_cloud_QA = None

    # keys used to find bands in the HDF files
    key1 = None
    key2 = None
    key3 = None
    key4 = None
    key5 = None
    key6 = None
    key7 = None
    key_fill_QA = None
    key_cloud_QA = None
    key_shadow_QA = None
    key_snow_QA = None
    key_land_water_QA = None
    key_adjacent_cloud_QA = None

    ########################################################################
    # Description: class constructor verifies the input file exists, then
    #     opens it, reads the metadata, and establishes pointers to the
    #     various SDSs.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 4/30/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to utilize a log file if passed along.
    #
    # Inputs:
    #   fname - name of the input HDF reflectance file to be processed
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     None - error opening or reading the file via GDAL
    #     Object - successful processing
    #
    # Notes:
    #######################################################################
    def __init__ (self, fname, log_handler=None):
        # make sure the file exists then open it with GDAL
        if not os.path.exists(fname):
            msg = 'Input file does not exist: ' + fname
            logIt (msg, log_handler)
            return None
        
        self.filename=fname                       # store the filename
        self.dataset=gdal.Open(self.filename)     # open the file with GDAL
        if self.dataset is None:
            msg = 'GDAL could not open input file: ' + self.filename
            logIt (msg, log_handler)
            return None

        # extract the metadata for this file
        self.mdata = self.dataset.GetMetadata()
        self.month = int(self.mdata['AcquisitionDate'][5:7])
        self.day = int(self.mdata['AcquisitionDate'][8:10])
        self.year = int(self.mdata['AcquisitionDate'][0:4])

        # extract the dictionary of subdatasets
        self.subdatasets = self.dataset.GetMetadata('SUBDATASETS')

        # find the keys for each subdataset
        sd_pairs = self.subdatasets.items()
        
        # loop through each SD dataset and obtain a key to the subdataset in
        # the HDF file
        for i in range(0, len(sd_pairs)):
            sd = sd_pairs[i]

            if sd[1].find('Grid:band1') > 0:
                self.key1 = sd[0]

            if sd[1].find('Grid:band2') > 0:
                self.key2 = sd[0]

            if sd[1].find('Grid:band3') > 0:
                self.key3 = sd[0]

            if sd[1].find('Grid:band4') > 0:
                self.key4 = sd[0]

            if sd[1].find('Grid:band5') > 0:
                self.key5 = sd[0]

            # find band6 but don't grab band6_fill_QA
            if sd[1].find('Grid:band6_fill_QA') > 0 :
                next
            elif sd[1].find('Grid:band6') > 0:
                self.key6 = sd[0]

            if sd[1].find('Grid:band7') > 0:
                self.key7 = sd[0]

            if sd[1].find('Grid:fill_QA') > 0:
                self.key_fill_QA = sd[0]

            if sd[1].find('Grid:cloud_QA') > 0:
                self.key_cloud_QA = sd[0]

            if sd[1].find('Grid:cloud_shadow_QA') > 0:
                self.key_shadow_QA = sd[0]

            if sd[1].find('Grid:snow_QA') > 0:
                self.key_snow_QA = sd[0]

            if sd[1].find('Grid:land_water_QA') > 0:
                self.key_land_water_QA = sd[0]

            if sd[1].find('Grid:adjacent_cloud_QA') > 0:
                self.key_adjacent_cloud_QA = sd[0]  

        # close the main dataset
        self.dataset = None

        # validate all the SDSs were found
        if self.key1 == None:
            msg = 'Input band1 does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key2 == None:
            msg = 'Input band2 does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key3 == None:
            msg = 'Input band3 does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key4 == None:
            msg = 'Input band4 does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key5 == None:
            msg = 'Input band5 does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key6 == None:
            msg = 'Input band6 does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key7 == None:
            msg = 'Input band7 does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key_fill_QA == None:
            msg = 'Input fill_QA band does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key_cloud_QA == None:
            msg = 'Input cloud_QA band does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key_shadow_QA == None:
            msg = 'Input cloud_shadow_QA band does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key_snow_QA == None:
            msg = 'Input snow_QA band does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key_land_water_QA == None:
            msg = 'Input land_water_QAband does not exist in ' + fname
            logIt (msg, log_handler)
            return None
        if self.key_adjacent_cloud_QA == None:
            msg = 'Input adjacent_cloud_QA band does not exist in ' + fname
            logIt (msg, log_handler)
            return None
    
        # open connections to the individual bands
        self.subdataset1 = gdal.Open(self.subdatasets[self.key1])
        self.subdataset2 = gdal.Open(self.subdatasets[self.key2])
        self.subdataset3 = gdal.Open(self.subdatasets[self.key3])
        self.subdataset4 = gdal.Open(self.subdatasets[self.key4])
        self.subdataset5 = gdal.Open(self.subdatasets[self.key5])
        self.subdataset6 = gdal.Open(self.subdatasets[self.key6])
        self.subdataset7 = gdal.Open(self.subdatasets[self.key7])
        self.subdataset_fill_QA = gdal.Open(self.subdatasets[self.key_fill_QA])
        self.subdataset_cloud_QA =  \
            gdal.Open(self.subdatasets[self.key_cloud_QA])
        self.subdataset_shadow_QA =  \
            gdal.Open(self.subdatasets[self.key_shadow_QA])
        self.subdataset_snow_QA = gdal.Open(self.subdatasets[self.key_snow_QA])
        self.subdataset_land_water_QA =  \
            gdal.Open(self.subdatasets[self.key_land_water_QA])
        self.subdataset_adjacent_cloud_QA =  \
            gdal.Open(self.subdatasets[self.key_adjacent_cloud_QA])
        
        # create connections to the bands
        self.band1 = self.subdataset1.GetRasterBand(1)
        self.band2 = self.subdataset2.GetRasterBand(1)
        self.band3 = self.subdataset3.GetRasterBand(1)
        self.band4 = self.subdataset4.GetRasterBand(1)
        self.band5 = self.subdataset5.GetRasterBand(1)
        self.band6 = self.subdataset6.GetRasterBand(1)
        self.band7 = self.subdataset7.GetRasterBand(1)
        self.band_fill_QA = self.subdataset_fill_QA.GetRasterBand(1)
        self.band_cloud_QA = self.subdataset_cloud_QA.GetRasterBand(1)
        self.band_shadow_QA = self.subdataset_shadow_QA.GetRasterBand(1)
        self.band_snow_QA = self.subdataset_snow_QA.GetRasterBand(1)
        self.band_land_water_QA = self.subdataset_land_water_QA.GetRasterBand(1)
        self.band_adjacent_cloud_QA =  \
            self.subdataset_adjacent_cloud_QA.GetRasterBand(1)

        # Verify the bands were actually accessed successfully
        if self.band1 == None:
            msg = 'Input band1 connection failed'
            logIt (msg, log_handler)
            return None
        if self.band2 == None:
            msg = 'Input band2 connection failed'
            logIt (msg, log_handler)
            return None
        if self.band3 == None:
            msg = 'Input band3 connection failed'
            logIt (msg, log_handler)
            return None
        if self.band4 == None:
            msg = 'Input band4 connection failed'
            logIt (msg, log_handler)
            return None
        if self.band5 == None:
            msg = 'Input band5 connection failed'
            logIt (msg, log_handler)
            return None
        if self.band6 == None:
            msg = 'Input band6 connection failed'
            logIt (msg, log_handler)
            return None
        if self.band7 == None:
            msg = 'Input band7 connection failed'
            logIt (msg, log_handler)
            return None
        if self.band_fill_QA == None:
            msg = 'Input band_fill_QA connection failed'
            logIt (msg, log_handler)
            return None
        if self.band_cloud_QA == None:
            msg = 'Input band_cloud_QA connection failed'
            logIt (msg, log_handler)
            return None
        if self.band_shadow_QA == None:
            msg = 'Input band_shadow_QA connection failed'
            logIt (msg, log_handler)
            return None
        if self.band_snow_QA == None:
            msg = 'Input band_snow_QA connection failed'
            logIt (msg, log_handler)
            return None
        if self.band_land_water_QA == None:
            msg = 'Input band_land_water_QA connection failed'
            logIt (msg, log_handler)
            return None
        if self.band_adjacent_cloud_QA == None:
            msg = 'Input band_adjacent_cloud_QA connection failed'
            logIt (msg, log_handler)
            return None

        # get coordinate information from band 1, get the number of rows
        # and columns, then determine the actual bounding coordinates and
        # the delta X and Y values (from west to east and north to south)
        gt = self.subdataset1.GetGeoTransform()

        self.NCol = self.subdataset1.RasterXSize
        self.dX = gt[1]
        self.WestBoundingCoordinate = gt[0]
        self.EastBoundingCoordinate = self.WestBoundingCoordinate +  \
            (self.NCol * self.dX)

        self.NRow = self.subdataset1.RasterYSize        
        self.dY = gt[5]
        self.NorthBoundingCoordinate = gt[3]
        self.SouthBoundingCoordinate = self.NorthBoundingCoordinate +  \
            (self.NRow * self.dY)

        # update the other variables
        self.WRS_Path = int(self.mdata['WRS_Path'])
        self.WRS_Row = int(self.mdata['WRS_Row'])
        self.Satellite = self.mdata['Satellite']
        self.SolarAzimuth = float(self.mdata['SolarAzimuth'])
        self.SolarZenith = float(self.mdata['SolarZenith'])
        self.AcquisitionDate =  \
            time.strptime(self.mdata['AcquisitionDate'][0:10], "%Y-%m-%d")


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
        self.dataset = None
        self.subdataset1 = None
        self.subdataset2 = None
        self.subdataset3 = None
        self.subdataset4 = None
        self.subdataset5 = None
        self.subdataset6 = None
        self.subdataset7 = None
        self.subdataset_fill_QA = None
        self.subdataset_cloud_QA = None
        self.subdataset_shadow_QA = None
        self.subdataset_snow_QA = None
        self.subdataset_land_water_QA = None
        self.subdataset_adjacent_cloud_QA = None

        self.band1 = None
        self.band2 = None
        self.band3 = None
        self.band4 = None
        self.band5 = None
        self.band6 = None
        self.band7 = None
        self.band_fill_QA = None
        self.band_cloud_QA = None
        self.band_shadow_QA = None
        self.band_snow_QA = None
        self.band_land_water_QA = None
        self.band_adjacent_cloud_QA = None


    ########################################################################
    # Description: xy2ij converts projection coordinates (x,y) to pixel
    #     space (i,j)
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #
    # Inputs:
    #   x - x projection coordinate
    #   y - y projection coordinate
    #
    # Returns:
    #     (col, row) - i,j pixel referring to x,y coordinate
    #
    # Notes:
    #######################################################################
    def xy2ij(self, x, y):
        col = int((x - self.WestBoundingCoordinate) / self.dX)
        row = int((y - self.NorthBoundingCoordinate) / self.dY)
        return([col, row])


    ########################################################################
    # Description: ij2xy converts pixel space values (i,j) to projection
    #     coordinates (x,y)
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #
    # Inputs:
    #   row - row (line) pixel space coordinate
    #   col - column (sample) pixel space coordinate
    #
    # Returns:
    #     (x, y) - x,y projection coord referring to pixel i,j
    #
    # Notes:
    #######################################################################
    def ij2xy(self, col, row):
        x = self.WestBoundingCoordinate + (col * self.dX)
        y = self.NorthBoundingCoordinate + (row * self.dY)
        return([x,y])


    ########################################################################
    # Description: getStackBandValues reads a stack of pixel values (in the Z
    #     direction) at the specified x,y location
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #
    # Inputs:
    #   x - x projection coordinate
    #   y - y projection coordinate
    #
    # Returns:
    #     None - if pixel location does not fall within the coordinates of
    #         this scene
    #     pixel stack - stack of values for the x,y location as band1, band2,
    #         band3, band4, band5, band6, band7, and QA band
    #
    # Notes:
    #######################################################################
    def getStackBandValues(self, x, y):
        if (x < self.EastBoundingCoordinate) and  \
           (x > self.WestBoundingCoordinate) and  \
           (y < self.NorthBoundingCoordinate) and  \
           (y > self.SouthBoundingCoordinate):

            # get the i,j pixel for the x,y projection coordinates
            ij = self.xy2ij(x,y)

            # read the pixel from each of the bands in the scene, including
            # the QA bands
            x1 = self.band1.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x2 = self.band2.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x3 = self.band3.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x4 = self.band4.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x5 = self.band5.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x6 = self.band6.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            x7 = self.band7.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]

            fill_QA = self.band_fill_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            cloud_QA = self.band_cloud_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0] 
            shadow_QA = self.band_shadow_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            snow_QA = self.band_snow_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            land_water_QA =  \
                self.band_land_water_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
            adjacent_cloud_QA =  \
                self.band_adjacent_cloud_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]

            # turn the QA values into one overall value with -9999 representing
            # the noData value; don't mask the DDV values
            QA = 0
            if land_water_QA > 0:
                QA = -3
            if snow_QA > 0:
                QA = -4
            if adjacent_cloud_QA > 0:
                QA = -5
            if shadow_QA > 0:
                QA = -6
            if cloud_QA > 0:
                QA = -7
            if fill_QA > 0:
                QA = -9999
                
            return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4, \
                'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA} )
        else:
            return (None)


    ########################################################################
    # Description: getRowOfBandValues reads a row of band values for each
    #     of the bands specified at the associated row for the projection
    #     y coordinate.  The projection y coordinate is converted to the
    #     actual row/line in the current scene, and then that row is read
    #     (reading all the samples in the line) and returned.
    #
    # History:
    #   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
    #       Geographic Science Center
    #   Updated on 5/1/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Modified to validate the y projection coordinate falls within
    #       the current scene.
    #
    # Inputs:
    #   y - y projection coordinate
    #
    # Returns:
    #     None - if pixel location does not fall within the coordinates of
    #         this scene
    #     Array - associated line of data y location as band1, band2,
    #         band3, band4, band5, band6, band7, and QA band
    #
    # Notes:
    #######################################################################
    def getRowOfBandValues(self, y):
        if (y < self.NorthBoundingCoordinate) and  \
           (y > self.SouthBoundingCoordinate):

            # get the i,j pixel (line) for the y projection coordinates
            ij = self.xy2ij(0, y)

            # read the line from each of the bands in the scene, including
            # the QA bands
            x1 = self.band1.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x2 = self.band2.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x3 = self.band3.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x4 = self.band4.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x5 = self.band5.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x6 = self.band6.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            x7 = self.band7.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            
            fill_QA = self.band_fill_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            snow_QA = self.band_snow_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
            land_water_QA = self.band_land_water_QA.ReadAsArray(  \
                0, ij[1], self.NCol, 1)[0,]
            adjacent_cloud_QA = self.band_adjacent_cloud_QA.ReadAsArray(  \
                0, ij[1], self.NCol, 1)[0,]
            shadow_QA = self.band_shadow_QA.ReadAsArray(  \
                0, ij[1], self.NCol, 1)[0,]
            cloud_QA = self.band_cloud_QA.ReadAsArray(  \
                0, ij[1], self.NCol, 1)[0,]
            
            # turn the QA values into one overall value with -9999 representing
            # the noData value
            QA = zeros( shape(land_water_QA), dtype=int16)
            QA[land_water_QA > 0] = -3
            QA[snow_QA > 0] = -4
            QA[shadow_QA > 0] = -5
            QA[adjacent_cloud_QA > 0] = -6
            QA[cloud_QA > 0] = -7
            QA[fill_QA > 0] = -9999

            return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4,  \
                'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA } )
        else:
            return (None)


    ########################################################################
    # Description: getLineOfBandValues reads a row of band values for each
    #     of the bands at the specified row.
    #
    # History:
    #   Created on 5/1/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #       Based on the getRowOfBandValues except it passes in the actual
    #           line to be read vs. the projection coordinates and therefore
    #           needing to convert proj x,y to line,sample (i,j).
    #
    # Inputs:
    #   j - row (line) pixel space coordinate
    #
    # Returns:
    #     None - if pixel location does not fall within the coordinates of
    #         this scene
    #     Array - associated line of data y location as band1, band2,
    #         band3, band4, band5, band6, band7, and QA band
    #
    # Notes:
    #######################################################################
    def getLineOfBandValues(self, j):
        # read the line from each of the bands in the scene, including
        # the QA bands
        x1 = self.band1.ReadAsArray(0, j, self.NCol, 1)[0,]
        x2 = self.band2.ReadAsArray(0, j, self.NCol, 1)[0,]
        x3 = self.band3.ReadAsArray(0, j, self.NCol, 1)[0,]
        x4 = self.band4.ReadAsArray(0, j, self.NCol, 1)[0,]
        x5 = self.band5.ReadAsArray(0, j, self.NCol, 1)[0,]
        x6 = self.band6.ReadAsArray(0, j, self.NCol, 1)[0,]
        x7 = self.band7.ReadAsArray(0, j, self.NCol, 1)[0,]
        
        fill_QA = self.band_fill_QA.ReadAsArray(0, j, self.NCol, 1)[0,]
        snow_QA = self.band_snow_QA.ReadAsArray(0, j, self.NCol, 1)[0,]
        land_water_QA =   \
            self.band_land_water_QA.ReadAsArray(0, j, self.NCol, 1)[0,]
        adjacent_cloud_QA =   \
            self.band_adjacent_cloud_QA.ReadAsArray(0, j, self.NCol, 1)[0,]
        shadow_QA = self.band_shadow_QA.ReadAsArray(0, j, self.NCol, 1)[0,]
        cloud_QA = self.band_cloud_QA.ReadAsArray(0, j, self.NCol, 1)[0,]
        
        # combine all the QA bands to one output with negative values to
        # indicate the various types of QA values, with -9999 representing
        # the noData value
        QA = zeros (shape(land_water_QA), dtype=int16)
        QA[land_water_QA > 0] = -3
        QA[snow_QA > 0] = -4
        QA[shadow_QA > 0] = -5
        QA[adjacent_cloud_QA > 0] = -6
        QA[cloud_QA > 0] = -7
        QA[fill_QA > 0] = -9999

        return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4,  \
            'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA } )


    ########################################################################
    # Description: getBandValues reads an entire band of data for the
    #     specified band.  If it's the QA band, then special processing will
    #     occur to combine the various QA band values into one representative
    #     band.
    #
    # History:
    #   Created on 5/1/2013 by Gail Schmidt, USGS/EROS LSRD Project
    #
    # Inputs:
    #   band - string representing which band to read (band1, band2, band3,
    #          band4, band5, band6, band7, band_qa)
    #   log_handler - open log file for logging or None for stdout
    #
    # Returns:
    #     None - if error occurred trying to read the band or combine the
    #         QA bands
    #     Array - associated band of data
    #
    # Notes:
    #######################################################################
    def getBandValues(self, band, log_handler=None):
        if not (band in ['band1', 'band2', 'band3', 'band4', 'band5', \
            'band6', 'band7', 'band_qa']):
            print 'Band ' + band + 'is not supported. Needs to be one of ' \
                'band1, band2, band3, band4, band5, band6, band7, or band_qa.'
            return None

        # read and return the specified band
        if band == 'band1':
            return (self.band1.ReadAsArray())

        elif band == 'band2':
            return (self.band2.ReadAsArray())

        elif band == 'band3':
            return (self.band3.ReadAsArray())

        elif band == 'band4':
            return (self.band4.ReadAsArray())

        elif band == 'band5':
            return (self.band5.ReadAsArray())

        elif band == 'band6':
            return (self.band6.ReadAsArray())

        elif band == 'band7':
            return (self.band7.ReadAsArray())

        elif band == 'band_qa':        
            # read all the QA-related bands
            fill_QA = self.band_fill_QA.ReadAsArray()
            snow_QA = self.band_snow_QA.ReadAsArray()
            land_water_QA = self.band_land_water_QA.ReadAsArray()
            adjacent_cloud_QA = self.band_adjacent_cloud_QA.ReadAsArray()
            shadow_QA = self.band_shadow_QA.ReadAsArray()
            cloud_QA = self.band_cloud_QA.ReadAsArray()
        
            # combine all the QA bands to one output with negative values to
            # indicate the various types of QA values, with -9999 representing
            # the noData value; don't mask DDV (dark dense vegetation) pixels
            QA = zeros (shape(land_water_QA), dtype=int16)
            QA[land_water_QA > 0] = -3
            QA[snow_QA > 0] = -4
            QA[shadow_QA > 0] = -5
            QA[adjacent_cloud_QA > 0] = -6
            QA[cloud_QA > 0] = -7
            QA[fill_QA > 0] = -9999
            return QA
