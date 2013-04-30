from sys import path
from HDF_scene import *
import os, sys, time, datetime

# convert a landsat hdf file to a multi-band tif file
def hdf2tif(input_file, output_file):
    # test to make sure the input file exists
    if not os.path.exists(input_file):
        print 'Input file ' + input_file + ' does not exist!!!'
        return(False)
#    else:
#        print 'Input file = ' + input_file
        
    # test to make sure the output directory exists, create it if it does not
    output_dirname = os.path.dirname (output_file)
    if output_dirname != "" and not os.path.exists (output_dirname):
        print 'Creating directory for output file: ' + output_dirname
        os.makedirs (output_dirname)
    
#    print 'Output file = ' + output_file
    
    # open the input file
    hdfAttr = HDF_Scene(input_file)
    
    # create an output file with 8 bands which are all int16s and get band
    # pointers to each band. the last band is the QA band.
    num_out_bands = 8
    driver = gdal.GetDriverByName("GTiff")
    output_ds = driver.Create( output_file, hdfAttr.NCol, hdfAttr.NRow, \
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
    output_band8 = output_ds.GetRasterBand(8)   # QA band

    output_band1.SetNoDataValue(-9999)
    output_band2.SetNoDataValue(-9999)
    output_band3.SetNoDataValue(-9999)
    output_band4.SetNoDataValue(-9999)
    output_band5.SetNoDataValue(-9999)
    output_band6.SetNoDataValue(-9999)
    output_band7.SetNoDataValue(-9999)
    output_band8.SetNoDataValue(-9999)

    print 'DEBUG: north bounding coord, south bounding coord: %f, %f' % \
        (hdfAttr.NorthBoundingCoordinate, hdfAttr.SouthBoundingCoordinate)
    print 'DEBUG: pixel size: %f' % hdfAttr.dY
    for y in range( int(hdfAttr.NorthBoundingCoordinate),  \
        int(hdfAttr.SouthBoundingCoordinate), int(hdfAttr.dY)):
        # read a row of data
        vals = hdfAttr.getRowOfBandValues(y)    
        yoff = hdfAttr.xy2ij(0,y)[1]
        print 'DEBUG: y = %d, yoff = %d' % (y, yoff)

        # nodata only shows up in band1 for some reason, make sure it is
        # transfered to all the other bands
        nd = vals['band1'] == -9999
        #vals['band2'][nd] = -9999
        #vals['band3'][nd] = -9999
        #vals['band4'][nd] = -9999
        #vals['band5'][nd] = -9999
        #vals['band6'][nd] = -9999
        #vals['band7'][nd] = -9999
        vals['QA'][nd] = -9999
        
        output_band1.WriteArray( array([vals['band1']]), 0, yoff)
        output_band2.WriteArray( array([vals['band2']]), 0, yoff)
        output_band3.WriteArray( array([vals['band3']]), 0, yoff)
        output_band4.WriteArray( array([vals['band4']]), 0, yoff)
        output_band5.WriteArray( array([vals['band5']]), 0, yoff)
        output_band6.WriteArray( array([vals['band6']]), 0, yoff)
        output_band7.WriteArray( array([vals['band7']]), 0, yoff)
        output_band8.WriteArray( array([vals['QA']]), 0, yoff)
    
    # create histograms and pyramids
    histogram = output_band1.GetDefaultHistogram()
    output_band1.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])
    histogram = output_band1.GetDefaultHistogram()
    output_band2.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])
    histogram = output_band2.GetDefaultHistogram()
    output_band3.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])
    histogram = output_band3.GetDefaultHistogram()
    output_band4.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])
    histogram = output_band4.GetDefaultHistogram()
    output_band5.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])
    histogram = output_band5.GetDefaultHistogram()
    output_band6.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])
    histogram = output_band6.GetDefaultHistogram()
    output_band7.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])
    histogram = output_band8.GetDefaultHistogram()
    output_band8.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])  
    
    # build pyramids
    gdal.SetConfigOption('HFA_USE_RRD', 'YES')
    output_ds.BuildOverviews(overviewlist=[3,9,27,81,243,729])  
    
    # cleanup
    output_band1 = None
    output_band2 = None
    output_band3 = None
    output_band4 = None
    output_band5 = None
    output_band6 = None
    output_band7 = None
    output_band8 = None
    
    output_ds = None
