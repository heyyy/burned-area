import os, time
from sys import path
path.append("/d/workspace/FireECV/BAMA")
from HDF_scene import *

def generate_stack(input_dir, output_file):
	delim = ','
	
	# check to make sure the input directory exists
	if not os.path.exists(input_dir):
		print 'Could not find ' + input_dir
		return(False)
	else:
		print 'Input directory = ' + input_dir

	# check to make sure the output location exists, create the directory if needed
	if not os.path.exists( os.path.dirname(output_file) ):
		print 'Creating directory for output file'
		osmakedirs( os.path.dirname(output_file) )

	# open the output file
	f_out = open(output_file, 'w')
	if not f_out:
		print 'Could not open output_file: ' + f_out
		return(False)
		
	# loop through .hdf files in input_directory and gather information
	i = 0

	# print the header
	f_out.write( 'file,year,season,month,day,julian,path,row,sensor,west,east,north,south,ncol,nrow,dx,dy,utm_zone\n' )
	
	for f_in in sort(os.listdir(input_dir)):
		if f_in.endswith(".hdf")  and i < 9999:
			input_file = input_dir + f_in
			
			print 'Processing ' + input_file + '...'
			
			x = HDF_Scene(input_file)

			# determine which season the scene was aqcuired
			if x.month == 12 or x.month == 1 or x.month == 2:
				season = 'winter'
			elif x.month >= 3 and x.month <= 5:
				season = 'spring'
			elif x.month >= 6 and x.month <= 8:
				season = 'summer'
			else:
				season = 'fall'
			
			# determine the julian date of the acquisition date
			t = time.mktime( (x.year, x.month, x.day, 0, 0, 0, 0, 0, 0) )
			julian = int(time.strftime("%j", time.gmtime(t)))
			
			# get the projection information, put in quotes so commas in projection string don't confuse .csv readers
			#tProjection = "'" + x.subdataset1.GetProjection() + "'"
			tProjection = x.subdataset1.GetProjection()
			t_osr = osr.SpatialReference()
			t_osr.ImportFromWkt(tProjection)
			tUTM = t_osr.GetUTMZone()
			print "UTM Zone = " + str(tUTM)
		
			# calculate total number of pixels with QA masks set
			qatotal = long(0)
			if False:
				for j in range(0, x.NRow):
					y = x.ij2xy(0,j)[1]

					vals = x.getRowOfBandValues(y)

					qatotal += sum(vals['QA'] < 0)		
			
			f_out.write( input_file + delim + \
				str(x.year) + delim + \
				season + delim + \
				str(x.month) + delim + \
				str(x.day) + delim + \
				str(julian) + delim + \
				str(x.WRS_Path) + delim + \
				str(x.WRS_Row) + delim + \
				x.Satellite + delim + \
				str(x.WestBoundingCoordinate) + delim + \
				str(x.EastBoundingCoordinate) + delim + \
				str(x.NorthBoundingCoordinate) + delim + \
				str(x.SouthBoundingCoordinate) + delim + \
				str(x.NRow) + delim + \
				str(x.NCol) + delim + \
				str(x.dX) + delim + \
				str(x.dY) + delim + \
				str(tUTM) + '\n')
				#tProjection + '\n')

			i += 1

			x = None

	f_out.close()
	
	
if __name__ == '__main__':
	input_dir = "/e/workspace/FireECV/p025r034/unzipped/"
	output_file = "/e/workspace/FireECV/p025r034/hdf_stack.csv"

	generate_stack(input_dir, output_file)
