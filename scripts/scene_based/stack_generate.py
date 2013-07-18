#! /usr/bin/env python
import os, time
from sys import path
from HDF_scene import *
from log_it import *

########################################################################
# Description: generate_stack will determine lndsr files residing in the
#     input_dir, then write a simple list_file and a more involved
#     stack_file outlining the files to be processed and some of their
#     data attributes.
#
# History:
#   Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
#       Geographic Science Center
#   Updated on 4/26/2013 by Gail Schmidt, USGS/EROS LSRD Project
#       Modified to write a list file as well as the stack file.  Also
#       added headers for the python functions and support for a log file.
#
# Inputs:
#   input_dir - name of the directory in which to find the lndsr products
#       to be processed
#   stack_file - name of stack file to create; list of the lndsr products
#       to be processed in addition to the date, path/row, sensor, bounding
#       coords, pixel size, and UTM zone
#   list_file - name of list file to create; simple list of lndsr products
#       to be processed from the current directory
#   log_handler - open log file for logging or None for stdout
#
# Returns: Nothing
#
# Notes:
#######################################################################
def generate_stack (input_dir, stack_file, list_file, log_handler):
    # define CSV delimiter
    delim = ','
    
    # check to make sure the input directory exists
    if not os.path.exists(input_dir):
        print 'Could not find ' + input_dir
        return(False)
    else:
        print 'Input directory = ' + input_dir

    # check to make sure the output location exists, create the directory if
    # needed
    if not os.path.exists( os.path.dirname(stack_file) ):
        print 'Creating directory for output file'
        osmakedirs( os.path.dirname(stack_file) )

    if not os.path.exists( os.path.dirname(list_file) ):
        print 'Creating directory for output file'
        osmakedirs( os.path.dirname(list_file) )

    # open the output files
    fstack_out = open(stack_file, 'w')
    if not fstack_out:
        print 'Could not open stack_file: ' + fstack_out
        return(False)
        
    flist_out = open(list_file, 'w')
    if not flist_out:
        print 'Could not open list_file: ' + flist_out
        return(False)
        
    # print the header for the stack file
    fstack_out.write( 'file,year,season,month,day,julian,path,row,sensor,' \
        'west,east,north,south,ncol,nrow,dx,dy,utm_zone\n' )
    
    # loop through lndsr*.hdf files in input_directory and gather information
    for f_in in sort(os.listdir(input_dir)):
        if f_in.endswith(".hdf") and (f_in.find("lndsr") == 0):
            input_file = input_dir + f_in
            
            # get the attributes from the HDF file
            print 'Processing ' + input_file + '...'
            hdfAttr = HDF_Scene(input_file)

            # determine which season the scene was acquired
            if hdfAttr.month == 12 or hdfAttr.month == 1 or hdfAttr.month == 2:
                season = 'winter'
            elif hdfAttr.month >= 3 and hdfAttr.month <= 5:
                season = 'spring'
            elif hdfAttr.month >= 6 and hdfAttr.month <= 8:
                season = 'summer'
            else:
                season = 'fall'
            
            # determine the julian date of the acquisition date
            t = time.mktime( (hdfAttr.year, hdfAttr.month, hdfAttr.day, 0, \
                0, 0, 0, 0, 0) )
            julian = int(time.strftime("%j", time.gmtime(t)))
            
            # get the projection information, put in quotes so commas in
            # projection string don't confuse .csv readers
            tProjection = hdfAttr.subdataset1.GetProjection()
            t_osr = osr.SpatialReference()
            t_osr.ImportFromWkt(tProjection)
            tUTM = t_osr.GetUTMZone()
            print "UTM Zone = " + str(tUTM)
        
            # calculate total number of pixels with QA masks set
            qatotal = long(0)
            if False:
                for j in range(0, hdfAttr.NRow):
                    y = hdfAttr.ij2xy(0,j)[1]

                    vals = hdfAttr.getRowOfBandValues(y)

                    qatotal += sum(vals['QA'] < 0)      
            
            # write the stack information for the current lndsr file
            fstack_out.write( input_file + delim + \
                str(hdfAttr.year) + delim + \
                season + delim + \
                str(hdfAttr.month) + delim + \
                str(hdfAttr.day) + delim + \
                str(julian) + delim + \
                str(hdfAttr.WRS_Path) + delim + \
                str(hdfAttr.WRS_Row) + delim + \
                hdfAttr.Satellite + delim + \
                str(hdfAttr.WestBoundingCoordinate) + delim + \
                str(hdfAttr.EastBoundingCoordinate) + delim + \
                str(hdfAttr.NorthBoundingCoordinate) + delim + \
                str(hdfAttr.SouthBoundingCoordinate) + delim + \
                str(hdfAttr.NRow) + delim + \
                str(hdfAttr.NCol) + delim + \
                str(hdfAttr.dX) + delim + \
                str(hdfAttr.dY) + delim + \
                str(tUTM) + '\n')
                #tProjection + '\n')

            # write the lndsr file to the output list file
            flist_out.write(input_file + '\n')

            # clear the HDF attributes for the next file
            hdfAttr = None

    # close the output list and stack files
    fstack_out.close()
    flist_out.close()
    
    
if __name__ == '__main__':
    input_dir = "/media/sf_Software_SandBox/Landsat/p035r032/"
    stack_file = "/media/sf_Software_SandBox/Landsat/p035r032/hdf_stack.csv"
    list_file = "/media/sf_Software_SandBox/Landsat/p035r032/hdf_list.txt"

    print 'Calling stack_generate'
    generate_stack(input_dir, stack_file, list_file, None)
