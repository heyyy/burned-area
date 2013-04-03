# from sys import path
from numpy import *
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst
import os
import time


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
	subdataset_DDV_QA = None
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
	band_DDV_QA = None
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
	key_DDV_QA = None
	key_cloud_QA = None
	key_shadow_QA = None
	key_snow_QA = None
	key_land_water_QA = None
	key_adjacent_cloud_QA = None

	# delete class
	def __del__(self):
		self.dataset = None
		self.subdataset1 = None
		self.subdataset2 = None
		self.subdataset3 = None
		self.subdataset4 = None
		self.subdataset5 = None
		self.subdataset6 = None
		self.subdataset7 = None
		self.subdataset_fill_QA = None
		self.subdataset_DDV_QA = None
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
		self.band_DDV_QA = None
		self.band_cloud_QA = None
		self.band_shadow_QA = None
		self.band_snow_QA = None
		self.band_land_water_QA = None
		self.band_adjacent_cloud_QA = None


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
	
		self.mdata = self.dataset.GetMetadata()					# extract the metadata
		self.month = int(self.mdata['AcquisitionDate'][5:7])
		self.day = int(self.mdata['AcquisitionDate'][8:10])
		self.year = int(self.mdata['AcquisitionDate'][0:4])
		self.subdatasets = self.dataset.GetMetadata('SUBDATASETS')		# extract the dictionary of subdatasets

		# find the keys for each subdataset
		sd_pairs = self.subdatasets.items()
		
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

			if sd[1].find('Grid:band6') > 0:
				self.key6 = sd[0]

			if sd[1].find('Grid:band7') > 0:
				self.key7 = sd[0]

			if sd[1].find('Grid:fill_QA') > 0:
				self.key_fill_QA = sd[0]

			if sd[1].find('Grid:DDV_QA') > 0:
				self.key_DDV_QA = sd[0]

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
	
		# open connections to the individual bands
		self.subdataset1 = gdal.Open( self.subdatasets[self.key1] )
		self.subdataset2 = gdal.Open( self.subdatasets[self.key2] )
		self.subdataset3 = gdal.Open( self.subdatasets[self.key3] )
		self.subdataset4 = gdal.Open( self.subdatasets[self.key4] )
		self.subdataset5 = gdal.Open( self.subdatasets[self.key5] )
		self.subdataset6 = gdal.Open( self.subdatasets[self.key6] )
		self.subdataset7 = gdal.Open( self.subdatasets[self.key7] )
		self.subdataset_fill_QA = gdal.Open( self.subdatasets[self.key_fill_QA] )
		self.subdataset_DDV_QA = gdal.Open( self.subdatasets[self.key_DDV_QA] )
		self.subdataset_cloud_QA = gdal.Open( self.subdatasets[self.key_cloud_QA] )
		self.subdataset_shadow_QA = gdal.Open( self.subdatasets[self.key_shadow_QA] )
		self.subdataset_snow_QA = gdal.Open( self.subdatasets[self.key_snow_QA] )
		self.subdataset_land_water_QA = gdal.Open( self.subdatasets[self.key_land_water_QA] )
		self.subdataset_adjacent_cloud_QA = gdal.Open( self.subdatasets[self.key_adjacent_cloud_QA] )
		
		# create connections to the bands
		self.band1 = self.subdataset1.GetRasterBand(1)
		self.band2 = self.subdataset2.GetRasterBand(1)
		self.band3 = self.subdataset3.GetRasterBand(1)
		self.band4 = self.subdataset4.GetRasterBand(1)
		self.band5 = self.subdataset5.GetRasterBand(1)
		self.band6 = self.subdataset6.GetRasterBand(1)
		self.band7 = self.subdataset7.GetRasterBand(1)
		self.band_fill_QA = self.subdataset_fill_QA.GetRasterBand(1)
		self.band_DDV_QA = self.subdataset_DDV_QA.GetRasterBand(1)
		self.band_cloud_QA = self.subdataset_cloud_QA.GetRasterBand(1)
		self.band_shadow_QA = self.subdataset_shadow_QA.GetRasterBand(1)
		self.band_snow_QA = self.subdataset_snow_QA.GetRasterBand(1)
		self.band_land_water_QA = self.subdataset_land_water_QA.GetRasterBand(1)
		self.band_adjacent_cloud_QA = self.subdataset_adjacent_cloud_QA.GetRasterBand(1)

		# get coordinate information from band 1
		gt = self.subdataset1.GetGeoTransform()

		self.NCol = self.subdataset1.RasterXSize
                self.dX = gt[1]
		self.WestBoundingCoordinate = gt[0]
		self.EastBoundingCoordinate = self.WestBoundingCoordinate + (self.NCol * self.dX)

		self.NRow = self.subdataset1.RasterYSize		
		self.dY = gt[5]
		self.NorthBoundingCoordinate = gt[3]
		self.SouthBoundingCoordinate = self.NorthBoundingCoordinate + (self.NRow * self.dY)

		# update the other variables
		self.WRS_Path = int(self.mdata['WRS_Path'])
		self.WRS_Row = int(self.mdata['WRS_Row'])
		self.Satellite = self.mdata['Satellite']
		self.SolarAzimuth = float(self.mdata['SolarAzimuth'])
		self.SolarZenith = float(self.mdata['SolarZenith'])
		self.AcquisitionDate = time.strptime(self.mdata['AcquisitionDate'][0:10], "%Y-%m-%d")

		

	# convert from coordinate space to pixel space
	def xy2ij(self, x, y):
		col = int((x - self.WestBoundingCoordinate) / self.dX)
		row = int((y - self.NorthBoundingCoordinate) / self.dY)
	
		return([col, row])

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
			fill_QA = self.band_fill_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			DDV_QA = self.band_DDV_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			cloud_QA = self.band_cloud_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0] 
			shadow_QA = self.band_shadow_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			snow_QA = self.band_snow_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			land_water_QA = self.band_land_water_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]
			adjacent_cloud_QA = self.band_adjacent_cloud_QA.ReadAsArray(ij[0], ij[1], 1, 1)[0,0]

			QA = 0
			if fill_QA > 0:
				QA = -1
			if DDV_QA > 0:
				QA = -2
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
		
		fill_QA = self.band_fill_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		DDV_QA = self.band_DDV_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		snow_QA = self.band_snow_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		land_water_QA = self.band_land_water_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		adjacent_cloud_QA = self.band_adjacent_cloud_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		shadow_QA = self.band_shadow_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		cloud_QA = self.band_cloud_QA.ReadAsArray(0, ij[1], self.NCol, 1)[0,]
		
		#QA = array(land_water_QA, copy=True)
		#QA.dtype = int16
		QA = zeros( shape(land_water_QA), dtype=int16)
		QA[fill_QA > 0] = -1
		QA[DDV_QA > 0] = -2
		QA[land_water_QA > 0] = -3
		QA[snow_QA > 0] = -4
		QA[shadow_QA > 0] = -5
		QA[adjacent_cloud_QA > 0] = -6	# these show up as big squares of nodata
		QA[cloud_QA > 0] = -7
		#QA[(x1==-9999) | (x2==-9999) | (x3==-9999) | (x4==-9999) | (x5==-9999) | (x6==-9999) | (x7==-9999)] = -9999
		QA[(x1==-9999) | (x2==-9999) | (x3==-9999) | (x4==-9999) | (x5==-9999) | (x7==-9999)] = -9999
		
		return( {'band1':x1, 'band2':x2, 'band3':x3, 'band4':x4, 'band5':x5, 'band6':x6, 'band7':x7, 'QA':QA } )
		#, 'fill_QA':fill_QA, 'DDV_QA':DDV_QA, 'cloud_QA':cloud_QA, 'shawdow_QA':shadow_QA, 'snow_QA':snow_QA, 'land_water_QA':land_water_QA, 'adjacent_cloud_QA':adjacent_cloud_QA} )

		#return_x = transpose( array( [x1, x2, x3, x4, x5, x6, x7, QA] )	)
		# dtype=[('band1', float), ('band2', float), ('band3', float), ('band4', float), ('band5', float), ('band6', float), ('band7', float), ('QA', float)
		#return( return_x )
