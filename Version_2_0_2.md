## Burned-Area Version 2.0.2 Release Notes ##
Release Date: Dec. 23, 2014

### Downloads ###

**Burned area source code - available via the [burned-area Google Projects Source](https://code.google.com/p/burned-area/source/checkout) link**

  * Non-members may check out a read-only working copy anonymously over HTTP.
  * svn checkout http://burned-area.googlecode.com/svn/releases/version_2.0.2 burned-area-read-only

### Installation ###
Same installation instructions as for Version 2.0.0.

### Dependencies ###
Same dependencies as for Version 2.0.0.

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
#### Updates on December 23, 2014 - USGS EROS ####
  * burned-area
    1. Modified the main do\_burned\_area script to double-check if the ‘config’ directory is already available, in the event of race conditions while using multiple processors.  Also updated the model hash table to specify the models desired by the Burned Area team for processing various path/rows.  This table is still incomplete, and the default is to use the mountain west model.