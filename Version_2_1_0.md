## Burned-Area Version 2.1.0 Release Notes ##
Release Date: May 27, 2015

### Downloads ###

**Burned area source code - available via the [burned-area Google Projects Source](https://code.google.com/p/burned-area/source/checkout) link**

  * Non-members may check out a read-only working copy anonymously over HTTP.
  * svn checkout http://burned-area.googlecode.com/svn/releases/version_2.1.0 burned-area-read-only

### Installation ###
Same installation instructions as for Version 2.0.0.

### Dependencies ###
Same dependencies as for Version 2.0.0. Need to define $PREFIX to point to the directory in which you want the executables, static data, etc. to be installed.

### Data Preprocessing ###
This version of the burned area application requires the input Landsat products to be in the ESPA internal file format.

### Data Postprocessing ###
After compiling the espa-common raw\_binary libraries and tools, the convert\_espa\_to\_gtif and convert\_espa\_to\_hdf command-line tools can be used to convert the ESPA internal file format to HDF or GeoTIFF.  Otherwise the data will remain in the ESPA internal file format, which includes each band in the ENVI file format (i.e. raw binary file with associated ENVI header file) and an overall XML metadata file.

### Associated Scripts ###
Same scripts as for Version 2.0.0.

### Verification Data ###

### User Manual ###

### Product Guide ###


## Changes From Previous Version ##
#### Updates on May 27, 2015 - USGS EROS ####
  * burned-area
    1. Cleaned up code from the change in numpy and scikit environment for the Denver BA group.
    1. Added the ability to exclude high RMSE and high cloud scenes.  This exclusion is turned on in the do\_burned\_area.py script.
    1. Updated to use espa\_common.h vs. common.h.
    1. Updated to install in $PREFIX/bin vs. $BIN