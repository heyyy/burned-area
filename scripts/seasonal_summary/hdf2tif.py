from sys import path
from HDF_scene import *
import os, sys, time, datetime

### Error/Success codes ###
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

def hdf2tif(input_file, output_file, make_histos=False, log_handler=None):
    """Converts the HDF file to GeoTiff.
    Description: hdf2tif will convert the HDF file to GeoTIFF.  It sets
        the noData value to -9999.
    
    History:
      Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
          Geographic Science Center
      Updated on 4/30/2013 by Gail Schmidt, USGS/EROS LSRD Project
          Took out the conversion of projection coords (x,y) to
              line, sample (i,j) space and vice versa.  This speeds up the
              processing time a bit.
          Removed some of the redundant tagging of QA as noData.
          Removed the redundant histogram and build pyramids code which is
              also done by polishImage
          Modified to utilize a log file if passed along.
    
    Inputs:
      input_file - name of the input HDF reflectance file to be converted
      output_file - name of the output GeoTIFF file which contains bands 1-7
          and a QA band
      make_histos - should histograms and overview pyramids be generated for
          each of the output GeoTIFF files?
      log_handler - open log file for logging or None for stdout
    
    Returns:
        ERROR - error converting the HDF file to GeoTIFF
        SUCCESS - successful processing
    """

    # test to make sure the input file exists
    if not os.path.exists(input_file):
        msg = 'Input file does not exist: ' + input_file
        logIt (msg, log_handler)
        return ERROR

    # test to make sure the output directory exists, create it if it does not
    output_dirname = os.path.dirname (output_file)
    if output_dirname != "" and not os.path.exists (output_dirname):
        msg = 'Creating directory for output file: ' + output_dirname
        logIt (msg, log_handler)
        os.makedirs (output_dirname)
    
    # open the input file
    hdfAttr = HDF_Scene(input_file, log_handler)
    if hdfAttr is None:
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

    # initialize the noData value for all bands to -9999, including the QA band
    output_band1.SetNoDataValue(-9999)
    output_band2.SetNoDataValue(-9999)
    output_band3.SetNoDataValue(-9999)
    output_band4.SetNoDataValue(-9999)
    output_band5.SetNoDataValue(-9999)
    output_band6.SetNoDataValue(-9999)
    output_band7.SetNoDataValue(-9999)
    output_band_QA.SetNoDataValue(-9999)

    # read and write each band, including the QA band, converting from HDF
    # to GeoTIFF; the QA band is a combination of all the QA values (negative
    # values flag any non-clear pixels and -9999 represents the fill pixels).
    vals = hdfAttr.getBandValues ('band1', log_handler)
    output_band1.WriteArray (vals)
    vals = hdfAttr.getBandValues ('band2', log_handler)
    output_band2.WriteArray (vals)
    vals = hdfAttr.getBandValues ('band3', log_handler)
    output_band3.WriteArray (vals)
    vals = hdfAttr.getBandValues ('band4', log_handler)
    output_band4.WriteArray (vals)
    vals = hdfAttr.getBandValues ('band5', log_handler)
    output_band5.WriteArray (vals)
    vals = hdfAttr.getBandValues ('band6', log_handler)
    output_band6.WriteArray (vals)
    vals = hdfAttr.getBandValues ('band7', log_handler)
    output_band7.WriteArray (vals)
    vals = hdfAttr.getBandValues ('band_qa', log_handler)
    output_band_QA.WriteArray(vals)

    # loop through all the lines in the image, read the line of data from
    # the HDF file, and write back out to GeoTIFF;  the QA band is a
    # combination of all the QA values (negative values flag non-clear
    # pixels and -9999 is the fill pixel).
    for y in range( 0, hdfAttr.NRow):
        # read a row of data
        vals = hdfAttr.getLineOfBandValues(y)    
        output_band1.WriteArray( array([vals['band1']]), 0, y)
        output_band2.WriteArray( array([vals['band2']]), 0, y)
        output_band3.WriteArray( array([vals['band3']]), 0, y)
        output_band4.WriteArray( array([vals['band4']]), 0, y)
        output_band5.WriteArray( array([vals['band5']]), 0, y)
        output_band6.WriteArray( array([vals['band6']]), 0, y)
        output_band7.WriteArray( array([vals['band7']]), 0, y)
        output_band_QA.WriteArray( array([vals['QA']]), 0, y)
      
    # build histograms and pyramid overviews if the user specified
    if make_histos:
        # create histograms
        histogram = output_band1.GetDefaultHistogram()
        if not histogram is None:
            output_band1.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
        histogram = output_band1.GetDefaultHistogram()
        if not histogram is None:
            output_band2.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
        histogram = output_band2.GetDefaultHistogram()
        if not histogram is None:
            output_band3.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
        histogram = output_band3.GetDefaultHistogram()
        if not histogram is None:
            output_band4.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
        histogram = output_band4.GetDefaultHistogram()
        if not histogram is None:
            output_band5.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
        histogram = output_band5.GetDefaultHistogram()
        if not histogram is None:
            output_band6.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
        histogram = output_band6.GetDefaultHistogram()
        if not histogram is None:
            output_band7.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])
        histogram = output_band_QA.GetDefaultHistogram()
        if not histogram is None:
            output_band_QA.SetDefaultHistogram(histogram[0], histogram[1],  \
                histogram[3])  
    
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
    output_band_QA = None
    output_ds = None

    return SUCCESS
