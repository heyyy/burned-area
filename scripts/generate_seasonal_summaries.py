import os, time
import csv
from sys import path
path.append("/d/workspace/FireECV/BAMA/")
from TIF_scene import *


##########
# generate seasonal summaries for a stack
# seasons:
#   winter = dec (previous year), jan, and feb
#   spring = mar, apr, may
#   summer = jun, jul, aug
#   fall = sep, oct, nov
#
# summaries:
#   good_count = number of 'lloks' with no QA flag set
#   mean value for season for bands 3,4,5,7, ndvi, ndmi, nbr, and nbr2
##########

#input_file = input_file="/d/workspace/FireECV/p025r034/hdf_stack.csv"
input_file = input_file="/e/workspace/FireECV/p025r034/hdf_stack.csv"

##########
# make sure the input_file exists
##########
if not os.path.exists(input_file):
    print 'Could not open stack file: ' + input_file
    #return(False)
else:
    print 'Stack file = ' + input_file

##########
# open the stack file
##########
f_in = open(input_file, 'r')
csv_data = recfromcsv(input_file, delimiter=',', names=True, dtype="string")
f_in = None

years = unique(csv_data['year'])
hdf_dir_name = os.path.dirname(csv_data['file_'][0])

# open the first file in the stack to get ncols and nrows
first_file = csv_data['file_'][0]
first_file = first_file.replace('unzipped','mask').replace('.hdf','.tif')
x = TIF_Scene_1_band(first_file)
ncol = x.NCol
nrow = x.NRow
#print 'NCol = ', str(ncol), 'NRow = ',str(nrow)
geotrans = x.dataset.GetGeoTransform()
prj = x.dataset.GetProjectionRef()
nodata = -9999

# loop through years
for year in range(1984,2012):
    print 'Number of files for year =', str(year), 'is', sum(csv_data['year'] == year)

    last_year = year - 1

    # loop through seasons
    for season in ['winter', 'spring', 'summer', 'fall']:
        if season == 'winter':
            season_files = ((csv_data['year'] == last_year) & (csv_data['month'] == 12)) | ((csv_data['year'] == year) & ((csv_data['month'] == 1) | (csv_data['month'] == 2)))
        elif season == 'spring':
            season_files = (csv_data['year'] == year) & ((csv_data['month'] >= 3) & (csv_data['month'] <= 5))
        elif season == 'summer':
            season_files = (csv_data['year'] == year) & ((csv_data['month'] >= 6) & (csv_data['month'] <= 8))
        else: # season=='fall'
            season_files = (csv_data['year'] == year) & ((csv_data['month'] >= 9) & (csv_data['month'] <= 11))
    
        print 'season =', season, 'file count = ', str(sum(season_files))
    
        n_files = sum(season_files)
        
        ##########
        # generate the mask stack
        ##########
        dir_name = hdf_dir_name.replace("unzipped","mask") + '/'
        
        files = csv_data['file_'][season_files]
        
        # create the mask datasets
        mask_data = zeros((n_files, nrow, ncol))
        mask_data.fill(nodata)
        
        for i in range(0,n_files):
            temp = files[i]
            mask_file = temp.replace('unzipped', 'mask').replace('.hdf','.tif')
            mask_dataset = gdal.Open( mask_file, gdalconst.GA_ReadOnly )
            mask_band = mask_dataset.GetRasterBand(1)
            mask_data[i,:,:] = mask_band.ReadAsArray()
            mask_band = None
            mask_dataset = None
        
        # which voxels in the mask have good qa values?
        mask_data_good = mask_data >= 0
        mask_data_bad = mask_data < 0
        
        # summarize across n_files
        if n_files > 0:
            good_looks = apply_over_axes(sum, mask_data_good, axes=[0])[0,:,:]
        else:
            good_looks = zeros((nrow,ncol)) + nodata
        
        #print 'Good looks array dimmensions: ', good_looks.shape
        
        # save the output
        good_looks_file = dir_name + str(year) + '_' + season + '_good_count.tif'
        print 'saving good count to', good_looks_file
        
        driver = gdal.GetDriverByName("GTiff")
        driver.Create(good_looks_file,ncol,nrow,1,gdalconst.GDT_Byte)
        
        good_looks_dataset = gdal.Open(good_looks_file, gdalconst.GA_Update)
        if good_looks_dataset is None:
            print 'Could not create output file: ', good_looks_file
        
        good_looks_dataset.SetGeoTransform(geotrans)
        good_looks_dataset.SetProjection(prj)
        
        good_looks_band1 = good_looks_dataset.GetRasterBand(1)
        good_looks_band1.SetNoDataValue(nodata)
        good_looks_band1.WriteArray(good_looks)
        
        good_looks_band1 = None
        good_looks_dataset = None
        
        
        ##########
        # loop through indices
        ##########
        for ind in ['band3', 'band4', 'band5', 'band7', 'ndvi', 'ndmi', 'nbr', 'nbr2']:
            print 'Generating', str(year), season, 'summary for', ind, 'using', n_files, 'files...'
            
            if ind == 'ndvi':
                dir_name = hdf_dir_name.replace("unzipped","ndvi") + '/'
            elif ind == 'ndmi':
                dir_name = hdf_dir_name.replace("unzipped","ndmi") + '/'
            elif ind == 'nbr':
                dir_name = hdf_dir_name.replace("unzipped","nbr") + '/'
            elif ind == 'nbr2':
                dir_name = hdf_dir_name.replace("unzipped","nbr2") + '/'
            else:   # tif file
                dir_name = hdf_dir_name.replace("unzipped","tif") + '/'

            sum_data = zeros((n_files, nrow, ncol))
            
            for i in range(0, n_files):
                temp = files[i]
                temp_file = dir_name + os.path.basename(files[i]).replace('.hdf','.tif')
                print 'Reading', temp_file      
                temp_dataset = gdal.Open( temp_file, gdalconst.GA_ReadOnly )
                
                if ind == 'band3':
                    temp_band = temp_dataset.GetRasterBand(3)
                elif ind == 'band4':
                    temp_band = temp_dataset.GetRasterBand(4)
                elif ind == 'band5':
                    temp_band = temp_dataset.GetRasterBand(5)
                elif ind == 'band7':
                    temp_band = temp_dataset.GetRasterBand(7)
                else:
                    temp_band = temp_dataset.GetRasterBand(1)
                    
                sum_data[i,:,:] = temp_band.ReadAsArray()
                
                temp_dataset = None
                temp_band = None
            
            # summarize across n_files
            if n_files > 0:
                # replace bad values with zeros
                #print 'sum_data dimmensions = ', sum_data.shape
                #print 'mask_data_good dimmensions = ', mask_data_good.shape
                sum_data[mask_data_bad] = 0
            
                # calculate totals within each voxel
                sum_data2 = apply_over_axes(sum, sum_data, axes=[0])[0,:,:]
                
                # divide by the number of good looks within a voxel
                mean_data = sum_data2 / good_looks
                
                # fill with nodata values in places where we would have divide by zero errors
                mean_data[good_looks == 0] = nodata
            else:
                mean_data = zeros((nrow,ncol)) + nodata

            # save the output here
            temp_file = dir_name + str(year) + '_' + season + '_' + ind + '.tif'
            print 'saving', ind, 'to', temp_file

            driver = gdal.GetDriverByName("GTiff")
            driver.Create(temp_file,ncol,nrow,1,gdalconst.GDT_Int16)

            temp_dataset = gdal.Open(temp_file, gdalconst.GA_Update)
            if temp_dataset is None:
                print 'Could not create output file: ', temp_file

            temp_dataset.SetGeoTransform(geotrans)
            temp_dataset.SetProjection(prj)

            temp_band1 = temp_dataset.GetRasterBand(1)
            temp_band1.SetNoDataValue(nodata)
            temp_band1.WriteArray(mean_data)

            temp_band1 = None
            temp_dataset = None

                        
