from sys import path
import os, sys, time, datetime
import csv

#path.append("/d/workspace/FireECV/BAMA")
from HDF_scene import *
from TIF_scene import *
from spectral_indices import *
from spectral_index_from_tif import *

##########
# Get spatial extent dictionary from text file
##########
def stackSpatialExtent(input_file):
    ##########
    # Check to make sure the input file exists, open it
    ##########
    if not os.path.exists(input_file):
        print 'Could not find ' + input_file
        return(False)

    reader = csv.reader(open(input_file, "rb"))
    
    header = reader.next()  # read the column headings
    values = reader.next()  # read the values

    # return a dictionary of results
    return_dict = dict([
        [header[0], float(values[0])], \
        [header[1], float(values[1])], \
        [header[2], float(values[2])], \
        [header[3], float(values[3])] \
    ])
    
    return(return_dict)
    


##########
# main processing loop
##########
path = "019"
row = "035"

# get the spatial extent
spatial_extent_file = "/e/workspace/FireECV/p" + path + "r" + row + "/spatial_extent/bounding_box_coordinates.csv"
print "Using spatial extent from ", spatial_extent_file
spatial_extent = stackSpatialExtent(input_file=spatial_extent_file)

# Open the stack file, loop through HDFs, convert them to .tifs, and resize to spatial extent
stack_file = "/e/workspace/FireECV/p" + path + "r" + row + "/hdf_stack.csv"
stack = csv.reader(open(stack_file, "rb"))

# get the header of the stack file
header_row = stack.next()
root_dir = "/e/workspace/FireECV"
#root_dir = "/disks/gec-projects-storage/samfs2/projects_share/ECVs/data/"
 
# set the output directories files
tif_dir = root_dir + "/p" + path + "r" + row + "/tif/"
ndvi_dir = root_dir + "/p" + path + "r" + row + "/ndvi/"
ndmi_dir = root_dir + "/p" + path + "r" + row + "/ndmi/"
nbr_dir = root_dir + "/p" + path + "r" + row + "/nbr/"
nbr2_dir = root_dir + "/p" + path + "r" + row + "/nbr2/"
mask_dir = root_dir + "/p" + path + "r" + row + "/mask/"
i = 0

# make each processing step run multithreaded with parallel python?
for row in enumerate(stack):
    startTime0 = time.time()

    print "################################################################################"
    hdf_file = row[1][header_row.index('file')]
    tif_file = hdf_file.replace('.hdf', '.tif')
    tif_file = tif_dir + os.path.basename(tif_file)
    temp_file = tif_dir + "temp.tif"
    
    # convert .hdf to .tif
    startTime = time.time()
    hdf2tif(hdf_file, temp_file)
    endTime = time.time()
    print 'Processing time = ' + str(endTime-startTime) + ' seconds'
    
    # resize .tif
    startTime = time.time()
    print "Resizing image..."
    cmd = 'gdal_merge.py -o ' + tif_file + ' -co "INTERLEAVE=BAND" -co "TILED=YES" -init -9999 -n -9999 -a_nodata -9999 -ul_lr ' + str(spatial_extent['West']) + ' ' + str(spatial_extent['North']) + ' ' + str(spatial_extent['East']) + ' ' + str(spatial_extent['South']) + ' ' + temp_file
    print cmd
    os.system(cmd)

    # open the tif, set nodata values, build pyramids, calculate histograms for the .tif file
    polishImage(tif_file, nodata=-9999)
    
    endTime = time.time()
    print 'Processing time = ' + str(endTime-startTime) + ' seconds'

    # calculate ndvi, ndmi, nbr, nbr2 from the converted tif file
    print "Calculating spectral indices..."
    ndvi_file = ndvi_dir + os.path.basename(tif_file)
    create_spectral_index(tif_file, ndvi_file, index='ndvi')
    
    ndmi_file = ndmi_dir + os.path.basename(tif_file)
    create_spectral_index(tif_file, ndmi_file, index='ndmi')
    
    nbr_file = nbr_dir + os.path.basename(tif_file)
    create_spectral_index(tif_file, nbr_file, index='nbr')
    
    nbr2_file = nbr2_dir + os.path.basename(tif_file)
    create_spectral_index(tif_file, nbr2_file, index='nbr2')
    
    mask_file = mask_dir + os.path.basename(tif_file)
    create_spectral_index(tif_file, mask_file, index='mask')
    
    i += 1
    
    endTime0 = time.time()
    print 'Total processing time = ' + str(endTime0-startTime0) + ' seconds'
    
    if (i > 2000):
        break
    
    
