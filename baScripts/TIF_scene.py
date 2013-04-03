# from sys import path
from numpy import *
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst
import os
import time


class TIF_Scene_1_band:
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

	# delete class
	def __del__(self):
		self.dataset = None
		self.band1 = None

	# create from filename
	def __init__(self, fname, band=1):
		if not os.path.exists(fname):
			print fname + " does not exist!"
			return
		
		self.filename=fname		# store the filename
		
		self.dataset=gdal.Open(self.filename)	# open the file with GDAL
		if self.dataset is None:
			print "GDAL could not open file " + self.filename + "!"
			return

		self.NBands = self.dataset.RasterCount
		
		# create connections to the bands
		self.band1 = self.dataset.GetRasterBand(band)

		# get coordinate information
		gt = self.dataset.GetGeoTransform()

		self.NCol = self.dataset.RasterXSize
		self.dX = gt[1]
		self.WestBoundingCoordinate = gt[0]
		self.EastBoundingCoordinate = self.WestBoundingCoordinate + (self.NCol * self.dX)

		self.NRow = self.dataset.RasterYSize		
		self.dY = gt[5]
		self.NorthBoundingCoordinate = gt[3]
		self.SouthBoundingCoordinate = self.NorthBoundingCoordinate + (self.NRow * self.dY)

	
	# convert from coordinate space to pixel space
	def xy2ij(self, x, y):
		if (x < self.EastBoundingCoordinate) and (x > self.WestBoundingCoordinate) and (y < self.NorthBoundingCoordinate) and (y > self.SouthBoundingCoordinate):
			col = int((x - self.WestBoundingCoordinate) / self.dX)
			row = int((y - self.NorthBoundingCoordinate) / self.dY)
	
			return([col, row])
		else:
			return(None)

	# convert from pixel space to coordinate space
	def ij2xy(self, col, row):
		x = self.WestBoundingCoordinate + (col * self.dX)
		y = self.NorthBoundingCoordinate + (row * self.dY)

		return([x,y])

	# get a stack of pixel values at a location
	def getBandValues(self, x, y):
		if (x < self.EastBoundingCoordinate) and (x > self.WestBoundingCoordinate) and (y < self.NorthBoundingCoordinate) and (y > self.SouthBoundingCoordinate):
			ij = self.xy2ij(x,y)

			x1 = self.band1.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]

			return( x1 )
		else:
			return(None)

		
	# get a row of pixel values
	def getRowOfBandValues(self, y):
		ij = self.xy2ij(0, y)

		x1 = self.band1.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		
		QA = self.band_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		
		return( x1 )


	# get array of pixel values
	def getArrayOfBandValues(self):
		x = zeros((self.NRow,self.NCol))
		
		return( self.band1.ReadAsArray() )
		
		
class TIF_Scene_8_band:
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

	# delete class
	def __del__(self):
		self.dataset = None

		self.band1 = None
		self.band2 = None
		self.band3 = None
		self.band4 = None
		self.band5 = None
		self.band6 = None
		self.band7 = None
		self.band_QA = None

	# create from filename
	def __init__(self, fname):
		if not os.path.exists(fname):
			print fname + " does not exist!"
			return
		
		self.filename=fname							# store the filename
		
		self.dataset=gdal.Open(self.filename)					# open the file with GDAL
		if self.dataset is None:
			print "GDAL could not open file " + self.filename + "!"
			return

		self.NBands = self.dataset.RasterCount
		
		# create connections to the bands
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
		self.EastBoundingCoordinate = self.WestBoundingCoordinate + (self.NCol * self.dX)

		self.NRow = self.dataset.RasterYSize		
		self.dY = gt[5]
		self.NorthBoundingCoordinate = gt[3]
		self.SouthBoundingCoordinate = self.NorthBoundingCoordinate + (self.NRow * self.dY)

	
	# convert from coordinate space to pixel space
	def xy2ij(self, x, y):
		if (x < self.EastBoundingCoordinate) and (x > self.WestBoundingCoordinate) and (y < self.NorthBoundingCoordinate) and (y > self.SouthBoundingCoordinate):
			col = int((x - self.WestBoundingCoordinate) / self.dX)
			row = int((y - self.NorthBoundingCoordinate) / self.dY)
		
			return([col, row])
		else:
			return(None)

	# convert from pixel space to coordinate space
	def ij2xy(self, col, row):
		x = self.WestBoundingCoordinate + (col * self.dX)
		y = self.NorthBoundingCoordinate + (row * self.dY)

		return([x,y])

	# get a stack of pixel values at a location
	def getBandValues(self, x, y):
		if (x < self.EastBoundingCoordinate) and (x > self.WestBoundingCoordinate) and (y < self.NorthBoundingCoordinate) and (y > self.SouthBoundingCoordinate):
			ij = self.xy2ij(x,y)

			x1 = self.band1.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			x2 = self.band2.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			x3 = self.band3.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			x4 = self.band4.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			x5 = self.band5.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			x6 = self.band6.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			x7 = self.band7.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			QA = self.band_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			if x1 == -9999:
				QA = -9999
				
			return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4, 'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA} )
		else:
			return(None)

		
	# get a row of pixel values
	def getRowOfBandValues(self, y):
		ij = self.xy2ij(0, y)

		x1 = self.band1.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		x2 = self.band2.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		x3 = self.band3.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		x4 = self.band4.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		x5 = self.band5.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		x6 = self.band6.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		x7 = self.band7.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		
		QA = self.band_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		
		return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4, 'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA } )

	# get array of pixel values
	def getArrayOfBandValues(self):
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

		
# open the tif, set nodata values, build pyramids, calculate histograms for the .tif file
def polishImage(input_file, nodata=-9999):
	# test to make sure the input file exists
	if not os.path.exists(input_file):
		print 'Input file ' + input_file + ' does not exist!!!'
		return(False)
	else:
		print 'Setting nodata, building histograms and pyramids for ' + input_file + '...'
	
	input_scene = TIF_Scene_8_band(input_file)
	
	input_scene.band1.SetNoDataValue(nodata)	
	input_scene.band2.SetNoDataValue(nodata)
	input_scene.band3.SetNoDataValue(nodata)
	input_scene.band4.SetNoDataValue(nodata)
	input_scene.band5.SetNoDataValue(nodata)
	input_scene.band6.SetNoDataValue(nodata)
	input_scene.band7.SetNoDataValue(nodata)
	input_scene.band_QA.SetNoDataValue(nodata)

	histogram = input_scene.band1.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band1.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	histogram = input_scene.band2.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band2.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	histogram = input_scene.band3.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band3.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	histogram = input_scene.band4.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band4.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	histogram = input_scene.band5.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band5.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	histogram = input_scene.band6.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band6.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	histogram = input_scene.band7.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band7.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	histogram = input_scene.band_QA.GetDefaultHistogram()
	if not histogram == None:
		input_scene.band_QA.SetDefaultHistogram(histogram[0], histogram[1], histogram[3])

	# build pyramids
	gdal.SetConfigOption('HFA_USE_RRD', 'YES')
	input_scene.dataset.BuildOverviews(overviewlist=[3,9,27,81,243,729])
	
