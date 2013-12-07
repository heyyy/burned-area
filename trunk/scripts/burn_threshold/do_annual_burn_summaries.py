#! /usr/bin/env python
#############################################################################
# Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
#       Geographic Science Center
# Created Python script to generate a single image of burned/unburned,
# maximum burned probability, first julian day a pixel was labeled as burned,
# and the number of good looks for each pixel (non cloud, non water, etc.) on
# an annual basis.
#
# History:
#   Updated on 12/2/2013 by Gail Schmidt, USGS/EROS
#       Modified to incorporate into the ESPA environment
#############################################################################

import sys
import os
import time
import getopt
import csv

import numpy

from argparse import ArgumentParser
from osgeo import gdal
from osgeo import ogr
from osgeo import osr
from osgeo import gdal_array
from osgeo import gdalconst

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


#############################################################################
# Created on December 2, 2013 by Gail Schmidt, USGS/EROS
# Turned into a class to run the overall annual burn summaries.
#
# History:
#
# Usage: do_annual_burn_summaries.py --help prints the help message
############################################################################
class AnnualBurnSummary():
    """Class for processing the annual burn summaries.
    Usage: do_annual_burn_summaries.py --help prints the help message
    """

    def __init__(self):
        pass


    def readSpatialExtent(self, bounding_extents_file, log_handler):
        """Reads the spatial extents from the bounding extents file.
        Description: readSpatialExtent will open the input CSV file, which
            contains the bounding extents for the current temporal stack,
            and returns a dictionary of east, west, north, south extents.
        
        History:
          Created by Gail Schmidt, USGS/EROS LSRD Project
        
        Args:
          bounding_extents_file - name of file which contains the bounding
              extents; first line should identify east, west, north, south
              coords; second line should contain the projection coords
              themselves
        
        Returns:
            None - error reading the bounding extents
            return_dict - dictionary of spatial extents
        """

        # check to make sure the input file exists before opening
        if not os.path.exists (bounding_extents_file):
            msg = 'Bounding extents file does not exist: ' + \
                bounding_extents_file
            logIt (msg, log_handler)
            return None

        # open the CSV file, read the column headings in the first line, then
        # read the spatial extents in the second line
        reader = csv.reader (open (bounding_extents_file, 'rb'))
        header = reader.next()  # read the column headings
        values = reader.next()  # read the values

        # return a dictionary of results. strip the leading and ending
        # white space from the header values since they may have white space
        # between the comma separators in the CSV file
        return_dict = dict([
            [header[0].strip(), float(values[0])], \
            [header[1].strip(), float(values[1])], \
            [header[2].strip(), float(values[2])], \
            [header[3].strip(), float(values[3])] \
        ])

        return return_dict


    def runAnnualBurnSummaries(self, stack_file=None,
        bounding_extents_file=None, bp_dir=None, bc_dir=None, output_dir=None,
        start_year=None, end_year=None, block_size=1024, make_histos=False,
        logfile=None):
        """Processes the annual burn summaries for each year in the stack.
        Description: routine to process the annual burn summaries for each
            pixel.
        
        History:
          Created in 2013 by Jodi Riegle and Todd Hawbaker, USGS Rocky Mountain
              Geographic Science Center
          Updated on Dec. 2, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to use argparser vs. optionparser, since optionparser
              is deprecated.
          Updated on Dec. 4, 2013 by Gail Schmidt, USGS/EROS LSRD Project
              Modified to resize the scene-based burn probabilities and
              classifications to the maximum geographic extent for stacking
              annual summaries.
        
        Args:
          stack_file - input CSV file with information about the files to be
              processed.  this is generated as part of the seasonal summaries
              application.
          bounding_extents_file - name of file which contains the bounding
              extents; first line should identify east, west, north, south
              coords; second line should contain the projection coords
              themselves
          bp_dir - location of the burn probability files
          bc_dir - location of the burn classification files
          output_dir - location to write the output burn classifications
          start_year - starting year of the stack_file to process; default is
              to start with the lowest year + 1
          end_year - ending year of the stack_file to process; default is to end
              with the highest year
          block_size - size of a block of data in pixels; default is
              512 x 2 = 1024 bytes
          make_histos - process histograms and overviews for each of the output
              GeoTiffs generated by this application; default is False
          logfile - name of the logfile for logging information; if None then
              the output will be written to stdout
   
        Returns:
            ERROR - error running the annual burn summary application
            SUCCESS - successful processing
        """

        # if no parameters were passed then get the info from the command line
        if stack_file is None:
            # get the command line argument for the input parameters
            parser = ArgumentParser(description='Run the annual burn summaries')
            parser.add_argument ('-f', '--stack_file', type=str,
                dest='stack_file',
                help='input file, csv delimited, each row contains '  \
                     'information about a landsat image',
                metavar='FILE', required=True)
            parser.add_argument ('-b', '--bounding_extents_file', type=str,
                dest='bounding_extents_file',
                help='input file, csv delimited, containing the bounding '  \
                     'geographic extents of the stack',
                metavar='FILE', required=True)
            parser.add_argument ('-p', '--bp_dir', type=str, dest='bp_dir',
                help='input directory, location to find input burn '  \
                     'probability files',
                metavar='DIR', required=True)
            parser.add_argument ('-c', '--bc_dir', type=str, dest='bc_dir',
                help='input directory, location to find input burn '  \
                     'classification files',
                metavar='DIR', required=True)
            parser.add_argument ('-o', '--output_dir', type=str,
                dest='output_dir',
                help='output directory, location to write burn '  \
                     'classification files',
                metavar='DIR', required=True)
            parser.add_argument ('-s', '--start_year', type=int,
                dest='start_year',
                help='starting year for processing the stack of burn '  \
                     'probabilities; default is to use the minimum year '  \
                     'of the files in the stack_file',
                metavar='YEAR')
            parser.add_argument ('-e', '--end_year', type=int, dest='end_year',
                help='ending year for processing the stack of burn '  \
                    'probabilities; default is to use the maximum year '  \
                    'of the files in the stack_file',
                metavar='YEAR')
            parser.add_argument ('--make_histos', dest='make_histos',
                default=False, action='store_true',
                help='process histograms and overviews for each of '  \
                    'the GeoTiffs generated by this application; default '  \
                    'is False')
            parser.add_argument ('-l', '--logfile', type=str, dest='logfile',
                help='name of optional log file', metavar='FILE')

            options = parser.parse_args()

            # validate command-line options and arguments
            stack_file = options.stack_file
            if stack_file is None:
                parser.error ("missing CSV stack file cmd-line argument")
                return ERROR

            bounding_extents_file = options.bounding_extents_file
            if bounding_extents_file is None:
                parser.error ("missing CSV bounding extensts file "  \
                    "cmd-line argument")
                return ERROR

            bp_dir = options.bp_dir
            if input_dir is None:
                parser.error ("missing input directory for the burn "  \
                    "probabilities cmd-line argument")
                return ERROR

            bc_dir = options.bc_dir
            if input_dir is None:
                parser.error ("missing input directory for the burn "  \
                    "classifications cmd-line argument")
                return ERROR

            output_dir = options.output_dir
            if output_dir is None:
                parser.error ("missing output directory cmd-line argument")
                return ERROR

            if options.start_year is not None:
                start_year = options.start_year

            if options.end_year is not None:
                end_year = options.end_year

            if options.block_size is not None:
                block_size = options.block_size

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile is not None:
            log_handler = open (logfile, 'w', buffering=1)

        # validate options and arguments
        if start_year is not None:
            if (start_year < 1984) | (start_year > 2011):
                msg = 'start_year falls outside 1984-2011: %d' % start_year
                logIt (msg, log_handler)
                return ERROR
    
        if end_year is not None:
            if (end_year < 1984) | (end_year > 2011):
                msg = 'end_year falls outside 1984-2011: %d' % end_year
                logIt (msg, log_handler)
                return ERROR
    
        if (end_year is not None) & (start_year is not None):
            if end_year < start_year:
                msg = 'end_year (%d) is less than start_year (%d)' %  \
                    (end_year, start_year)
                logIt (msg, log_handler)
                return ERROR
            
        if not os.path.exists(stack_file):
            msg = 'CSV stack file does not exist: ' + stack_file
            logIt (msg, log_handler)
            return ERROR
    
        if not os.path.exists(bounding_extents_file):
            msg = 'CSV bounding extents file does not exist: ' +  \
                bounding_extents_file
            logIt (msg, log_handler)
            return ERROR
    
        if not os.path.exists(bp_dir):
            msg = 'Burn probability directory does not exist: ' + bp_dir
            logIt (msg, log_handler)
            return ERROR
    
        if not os.path.exists(bc_dir):
            msg = 'Burn classificaton directory does not exist: ' + bc_dir
            logIt (msg, log_handler)
            return ERROR
    
        if not os.path.exists(output_dir):
            msg = 'Output directory does not exist: %s. Creating ...' %  \
                output_dir
            logIt (msg, log_handler)
            os.makedirs(output_dir, 0755)

        # save the current working directory for return to upon error or when
        # processing is complete
        mydir = os.getcwd()
        msg = 'Changing directories for burn threshold processing: ' +  \
            output_dir
        logIt (msg, log_handler)
        os.chdir (output_dir)

        # start of threshold processing
        start_time0 = time.time()
    
        # open the stack file
        stack = numpy.recfromcsv(stack_file, delimiter=",", names=True,  \
            dtype="string")
        
        # use the minimum and maximum years in the stack if the start year and
        # end year were not specified on the command line.  start year needs
        # to be one more than the actual starting year in the stack since the
        # burn products require a previous year to process.  so the start year
        # for the burn products is one year after the actual starting year in
        # the stack.
        if start_year is None:
            start_year = numpy.min(stack['year']) + 1
        
        if end_year is None:
            end_year = numpy.max(stack['year'])
        
        stack_mask = (stack['year'] >= start_year) & (stack['year'] <= end_year)
        stack2 = stack[ stack_mask, :]

        # read the spatial extents from the input file
        spatial_extent = self.readSpatialExtent (bounding_extents_file,  \
            log_handler)
        if spatial_extent is None:
            # error message already written
            os.chdir (mydir)
            return ERROR
        print 'DEBUG: spatial_extents: ' + str(spatial_extent)

        # resize the individual scenes to the maximum geographic extent for
        # all the scenes in the stack, similar to what was done for the 
        # seasonal summaries and annual maximums and using the same maximum
        # bounding coordinates. gdal_merge outputs by default to GeoTiff.
        for year in range(end_year,start_year-1,-1):
            stack_mask = stack2['year'] == year
            stack3 = stack2[ stack_mask, :]    

            # loop through the files in the stack for the specified year
            for i in range(0, stack3.shape[0]):
                file_name = stack3['file_'][i]
                
                # create the burn probability and classification filenames
                # from the lndsr filenames in the CSV
                fname = os.path.basename(file_name).replace('lndsr.','')
                fname = fname.replace('.hdf','_burn_probability.hdf')
                bp_file_name = bp_dir + "/" + fname
                fname = fname.replace('.hdf','.tif')
                bp_resize_name = bp_dir + "/resize_" + fname
                if not os.path.exists(bp_file_name):
                    msg = 'burn probability file does not exist: ' +  \
                        bp_file_name
                    logIt (msg, log_handler)
                    os.chdir (mydir)
                    return ERROR

                fname = os.path.basename(file_name).replace('lndsr.','')
                fname = fname.replace('.hdf','_burn_class.tif')
                bc_file_name = bc_dir + "/" + fname
                bc_resize_name = bc_dir + "/resize_" + fname
                if not os.path.exists(bc_file_name):
                    msg = 'burn classification file does not exist: ' +  \
                        bc_file_name
                    logIt (msg, log_handler)
                    os.chdir (mydir)
                    return ERROR

                msg = '   Resizing temp (%s) to max bounds (%s)' %  \
                    (bp_file_name, bp_resize_name)
                logIt (msg, log_handler)
                cmd = 'gdal_merge.py -o %s -co "INTERLEAVE=BAND" '  \
                    '-co "TILED=YES" -init -9999 -n -9999 -a_nodata -9999 ' \
                    '-ul_lr %d %d %d %d %s' % (bp_resize_name, \
                    spatial_extent['West'], spatial_extent['North'], \
                    spatial_extent['East'], spatial_extent['South'], \
                    bp_file_name)
                print 'DEBUG: ' + cmd
                os.system(cmd)

                msg = '   Resizing temp (%s) to max bounds (%s)' %  \
                    (bc_file_name, bc_resize_name)
                logIt (msg, log_handler)
                cmd = 'gdal_merge.py -o %s -co "INTERLEAVE=BAND" '  \
                    '-co "TILED=YES" -init -9999 -n -9999 -a_nodata -9999 ' \
                    '-ul_lr %d %d %d %d %s' % (bc_resize_name, \
                    spatial_extent['West'], spatial_extent['North'], \
                    spatial_extent['East'], spatial_extent['South'], \
                    bc_file_name)
                print 'DEBUG: ' + cmd
                os.system(cmd)

        # given that all burn products in this temporal stack have the same
        # scene extents and projection information, just obtain that
        # information from the first file and use it for all of the files
        # use the lndsr filename in the CSV file to obtain the resized burn
        # probability filename
        file_name = stack2['file_'][0]
        fname = os.path.basename(file_name).replace('lndsr.','')
        fname = fname.replace('.hdf','_burn_probability.tif')
        bp_resize_name = bp_dir + "/resize_" + fname
        if not os.path.exists(bp_resize_name):
            msg = 'resized burn probability file does not exist: ' +  \
                bp_resize_name
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR

        bp_dataset = gdal.Open(bp_resize_name)
        if bp_dataset is None:
            msg = 'Failed to open bp file: ' + bp_resize_name
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR
        
        bp_band = bp_dataset.GetRasterBand(1)
        if bp_band is None:
            msg = 'Failed to open bp band 1 from ' + bp_resize_name
            logIt (msg, log_handler)
            os.chdir (mydir)
            return ERROR
        
        geotrans = bp_dataset.GetGeoTransform()
        prj = bp_dataset.GetProjectionRef()
        nrow = bp_dataset.RasterYSize
        ncol = bp_dataset.RasterXSize
        nodata = bp_band.GetNoDataValue()
    
        # generate the blocks to analyze
        blocks = []
        for startRow in range(0, nrow, block_size):
            endRow = startRow + block_size
            if endRow > nrow:
                endRow = nrow
            
            for startCol in range(0, ncol, block_size):
                endCol = startCol + block_size
                if endCol > ncol:
                    endCol = ncol
            
                blocks.append( [startRow, endRow, startCol, endCol] )
    
        # create the TIF driver for output data
        driver = gdal.GetDriverByName("GTiff")
    
        # process the data for the years specified
        # create images for:
        #    1. first date a burn scar was observed (burn_scar)
        #    2. number of times burn scar was observed (burn_count)
        #    3. number of good looks (good_looks_count)
        #    4. maximum probability for burn scar (max_burn_prob)
    
        # loop through the years in the stack
        msg = 'Processing burn files for %d-%d' % (end_year, start_year)
        logIt (msg, log_handler)
        for year in range(start_year,end_year+1):
            msg = '########################################################'
            logIt (msg, log_handler)
            msg = 'Processing %d ...' % year
            logIt (msg, log_handler)
                
            stack_mask = stack2['year'] == year
            stack3 = stack2[ stack_mask, :]    

            # initialize the input and output datasets
            input_datasets = numpy.empty( (stack2.shape[0],2), dtype=object )
            input_bands = numpy.empty( (stack2.shape[0],2), dtype=object )
            
            output_datasets = numpy.empty((4), dtype=object)
            output_bands = numpy.empty((4), dtype=object)
        
            # open the input datasets - 1st band is burn probability,
            # 2nd band is burn classification
            for i in range(0, stack3.shape[0]):
                file_name = stack3['file_'][i]
                
                # construct the resized burn probability and classification
                # filenames from the lndsr filenames in the CSV
                fname = os.path.basename(file_name).replace('lndsr.','')
                fname = fname.replace('.hdf','_burn_probability.tif')
                bp_resize_name = bp_dir + "/" + "resize_" + fname
                if not os.path.exists(bp_resize_name):
                    msg = 'resized burn probability file does not exist: ' +  \
                        bp_resize_name
                    logIt (msg, log_handler)
                    os.chdir (mydir)
                    return ERROR

                msg = 'Reading %s ...' % bp_resize_name
                logIt (msg, log_handler)
                input_datasets[i,0] = gdal.Open(bp_resize_name)
                input_bands[i,0] = input_datasets[i,0].GetRasterBand(1)
                
                fname = os.path.basename(file_name).replace('lndsr.','')
                fname = fname.replace('.hdf','_burn_class.tif')
                bc_resize_name = bc_dir + "/resize_" + fname
                if not os.path.exists(bc_resize_name):
                    msg = 'resized burn classification file does not '  \
                        'exist: ' + bc_resize_name
                    logIt (msg, log_handler)
                    os.chdir (mydir)
                    return ERROR

                msg = 'Reading %s ...' % bc_resize_name
                logIt (msg, log_handler)
                input_datasets[i,1] = gdal.Open(bc_resize_name)
                input_bands[i,1] = input_datasets[i,1].GetRasterBand(1)

            # open the output datasets
            # first date of burn scar (burn_scar)
            fname = output_dir + "/burn_scar_" + str(year) + ".tif"
            output_datasets[0] = driver.Create(fname, ncol, nrow, 1, \
                gdal.GDT_Int16)
            output_datasets[0].SetGeoTransform(geotrans)
            output_datasets[0].SetProjection(prj)
            output_bands[0] = output_datasets[0].GetRasterBand(1)
            output_bands[0].SetNoDataValue(nodata)
            
            # count of times a pixel was burned (burn_count)
            fname = output_dir + "/burn_count_" + str(year) + ".tif"
            output_datasets[1] = driver.Create(fname, ncol, nrow, 1,  \
                gdal.GDT_Int16)
            output_datasets[1].SetGeoTransform(geotrans)
            output_datasets[1].SetProjection(prj)
            output_bands[1] = output_datasets[1].GetRasterBand(1)
            output_bands[1].SetNoDataValue(nodata)
            
            # count of good looks (good_looks_count)
            fname = output_dir + "/good_looks_count_" + str(year) + ".tif"
            output_datasets[2] = driver.Create(fname, ncol, nrow, 1,  \
                gdal.GDT_Int16)
            output_datasets[2].SetGeoTransform(geotrans)
            output_datasets[2].SetProjection(prj)
            output_bands[2] = output_datasets[2].GetRasterBand(1)
            output_bands[2].SetNoDataValue(nodata)
            
            # maximum burn probability (max_burn_prob)
            fname = output_dir + "/max_burn_prob_" + str(year) + ".tif"
            output_datasets[3] = driver.Create(fname, ncol, nrow, 1,  \
                gdal.GDT_Int16)
            output_datasets[3].SetGeoTransform(geotrans)
            output_datasets[3].SetProjection(prj)
            output_bands[3] = output_datasets[3].GetRasterBand(1)
            output_bands[3].SetNoDataValue(nodata)

            # loop through the blocks 
            for j in range(0, len(blocks)):
                block = blocks[j]
                startRow = block[0]
                endRow = block[1]
                startCol = block[2]
                endCol = block[3]
                
                this_block_cols = endCol - startCol
                this_block_rows = endRow - startRow
                
                # create the arrays to hold input and output data
                input_data = numpy.empty((stack3.shape[0], 2, this_block_rows, \
                    this_block_cols), dtype=numpy.int16)
                input_data.fill(nodata)
    
                output_data = numpy.empty((1, 4, this_block_rows,  \
                    this_block_cols), dtype=numpy.int16)
                output_data.fill(nodata)
    
                # read input data for burn probs and burn classes
                for i in range(0, stack3.shape[0]):
                    input_data[i,0,:,:] = input_bands[i,0].ReadAsArray(  \
                        startCol, startRow, endCol-startCol, endRow-startRow)
                    input_data[i,1,:,:] = input_bands[i,1].ReadAsArray(  \
                        startCol, startRow, endCol-startCol, endRow-startRow)
                
                # find the maximum burn probability (using burn prob)
                bp_max = numpy.apply_over_axes(numpy.max, input_data[:,0,:,:], \
                    axes=[0])[0,:,:]
                
                # find the count of burns - how many times a pixel burned
                # (using burn class)
                bc = numpy.apply_over_axes(numpy.sum,  \
                    input_data[:,1,:,:] >= 1, axes=[0])[0,:,:]
                bc[bp_max == nodata] = nodata
    
                # find the first date of burn scar (using burn class)
                bdi = numpy.apply_over_axes(numpy.argmax,  \
                    input_data[:,1,:,:] >= 1, axes=[0])[0,:,:]
                
                # convert bdi to julian date
                bd = stack3['julian'][bdi]
                bd[bc == 0] = 0
                bd[bp_max == nodata] = nodata
                
                # find the number of good looks (using burn class)
                gc = numpy.apply_over_axes(numpy.sum,  \
                    input_data[:,1,:,:] >= 0, axes=[0])[0,:,:]
                gc[bp_max == nodata] = nodata
            
                # write output data
                #print "Writing output data..."
                output_bands[0].WriteArray(bd, xoff=startCol, yoff=startRow)
                output_bands[1].WriteArray(bc, xoff=startCol, yoff=startRow)
                output_bands[2].WriteArray(gc, xoff=startCol, yoff=startRow)
                output_bands[3].WriteArray(bp_max, xoff=startCol, yoff=startRow)

            # generate the histograms and overviews if specified
            if make_histos:
                for i in range(0, 4):
                    histogram = output_bands[i].GetDefaultHistogram()
                    if histogram is not None:
                        output_bands[i].SetDefaultHistogram(histogram[0],  \
                            histogram[1], histogram[3])
                    output_bands[i].FlushCache()
                    
                    output_datasets[i].BuildOverviews(  \
                        overviewlist=[3,9,27,81,243,729])
                    output_datasets[i].FlushCache()

            # cleanup the resized images
            for i in range(0, stack3.shape[0]):
                file_name = stack3['file_'][i]

                # construct the resized burn probability and classification
                # filenames from the lndsr filenames in the CSV
                fname = os.path.basename(file_name).replace('lndsr.','')
                fname = fname.replace('.hdf','_burn_probability.tif')
                bp_resize_name = bp_dir + "/" + "resize_" + fname
                os.remove(bp_resize_name)

                fname = os.path.basename(file_name).replace('lndsr.','')
                fname = fname.replace('.hdf','_burn_class.tif')
                bc_resize_name = bc_dir + "/resize_" + fname
                os.remove(bc_resize_name)

        # successful completion.  return to the original directory.
        msg = 'Completion of annual burn summaries.'
        logIt (msg, log_handler)
        if logfile is not None:
            log_handler.close()
        os.chdir (mydir)
        return SUCCESS

######end of AnnualBurnSummary class######

if __name__ == "__main__":
    sys.exit (AnnualBurnSummary().runAnnualBurnSummaries())
