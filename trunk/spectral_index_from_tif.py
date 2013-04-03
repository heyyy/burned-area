##########
# Create spectral indices from Landsat data
##########
from sys import path
import os, sys, time, datetime, getopt

from numpy import *
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst

path.append("/d/workspace/FireECV/BAMA")
from spectral_indices import *

def create_spectral_index(input_file, output_file, index='nbr'):
	#input_file = '/d/workspace/FireECV/p028r033/unzipped/lndsr.LT50280332006103PAC01.hdf'
	#output_file = '/d/workspace/FireECV/p028r033/ndvi/ndvi_LT50280332006103PAC01.tif'
	startTime = time.time()

	##########
	# figure out which spectral index to generate
	##########
	if not (index in ['ndvi','nbr','nbr2','ndmi','mask']):
		print 'Algorithm for ' + index + ' is not implemented'
		return(False)
	else:
		print 'Spectral index = ' + index
		
	##########
	# Check to make sure the input file exists, open it
	##########
	if not os.path.exists(input_file):
		print 'Could not find ' + input_file
		return(False)
	else:
		print 'Input file = ' + input_file

	##########
	# define the image to work with, create connections to the various bands in the image
	##########
	input_ds = gdal.Open( input_file, gdalconst.GA_ReadOnly )
	if input_ds is None:
		print "Failed to open input file: " + input_file
		exit(False)
	input_band1 = input_ds.GetRasterBand(1)
	input_band2 = input_ds.GetRasterBand(2)
	input_band3 = input_ds.GetRasterBand(3)
	input_band4 = input_ds.GetRasterBand(4)
	input_band5 = input_ds.GetRasterBand(5)
	input_band6 = input_ds.GetRasterBand(6)
	input_band7 = input_ds.GetRasterBand(7)
	input_band8 = input_ds.GetRasterBand(8)

	##########
	# create the output folder if it does not exist
	##########
	output_dir = os.path.dirname(output_file)
	if not os.path.exists(output_dir):
		print 'Creating output directory ' + output_dir
		os.makedirs(output_dir)
		
	##########
	# Create the output file
	##########
	print 'Output file = ' + output_file
	driver = gdal.GetDriverByName("GTiff")
	output_ds = driver.Create( output_file, input_ds.RasterXSize, input_ds.RasterYSize, 1, gdal.GDT_Int16)
	output_ds.SetGeoTransform( input_ds.GetGeoTransform() )
	output_ds.SetProjection( input_ds.GetProjection() )
	output_band = output_ds.GetRasterBand(1)	

	##########
	# loop over rows and generate spectral index
	##########
	print 'Processing...'
	nodata = input_band1.GetNoDataValue()
	if nodata is None:
		nodata = -9999
		
	print 'NoData value = ', nodata
	
	qa = input_band8.ReadAsArray()
	b1 = input_band1.ReadAsArray()
	
	# calculate the spectral index
	if index == 'nbr':
		b4 = input_band4.ReadAsArray()
		b7 = input_band7.ReadAsArray()
		newVals = 1000.0 * NBR( 1.0 * b4, b7)
		newVals[ qa < 0 ] = nodata
	elif index == 'nbr2':
		b5 = input_band5.ReadAsArray()
		b7 = input_band7.ReadAsArray()
		newVals = 1000.0 * NBR2( 1.0 * b5, b7)
		newVals[ qa < 0 ] = nodata
	elif index == 'ndmi':
		b4 = input_band4.ReadAsArray()
		b5 = input_band5.ReadAsArray()
		newVals = 1000.0 * NDMI( 1.0 * b4, b5)
		newVals[ qa < 0 ] = nodata
	elif index == 'ndvi':
		b4 = input_band4.ReadAsArray()
		b3 = input_band3.ReadAsArray()
		newVals = 1000.0 * NDVI( 1.0 * b3, b4)
		newVals[ qa < 0 ] = nodata
	else:	# save the mask
		newVals = qa
		
	newVals[ b1 == nodata ] = nodata
	
	# write a row of output 
	output_band.WriteArray(newVals)

	##########
	# create histograms and pyramids
	##########
	output_band.SetNoDataValue(nodata)
	
	histogram = output_band.GetDefaultHistogram()
	if not histogram == None:
		output_band.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	# build pyramids
	gdal.SetConfigOption('HFA_USE_RRD', 'YES')
	output_ds.BuildOverviews(overviewlist=[3,9,27,81,243,729])

	##########
	# cleanup
	##########
	output_band = None
	output_ds = None

	input_band1 = None
	input_band2 = None
	input_band3 = None
	input_band4 = None
	input_band5 = None
	input_band6 = None
	input_band7 = None
	input_band8 = None
	
	endTime = time.time()
	print 'Processing time = ' + str(endTime-startTime) + ' seconds'

#input_file = "/d/workspace/FireECV/p028r033/tif/lndsr.LT50280331984107XXX01.tif"
#output_file = "/d/workspace/FireECV/p028r033/nbr/lndsr.LT50280331984107XXX01.tif"
#input_file = "/d/workspace/FireECV/p028r033/tif/lndsr.LT50280331985013XXX02.tif"
#output_file = "/d/workspace/FireECV/p028r033/ndvi/lndsr.LT50280331985013XXX02.tif"
#create_spectral_index(input_file, output_file, index='nbr')
