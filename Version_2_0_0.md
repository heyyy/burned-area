## Burned-Area Version 2.0.0 Release Notes ##
Release Date: June 9, 2014

### Downloads ###

**Burned area source code - available via the [burned-area Google Projects Source](https://code.google.com/p/burned-area/source/checkout) link**

  * Non-members may check out a read-only working copy anonymously over HTTP.
  * svn checkout http://burned-area.googlecode.com/svn/releases/version_2.0.0 burned-area-read-only

### Installation ###
Same installation instructions as for Version 1.0.0, however a high-level Makefile is now available for the overall burned-area source code.  Thus the user can utilize the Makefile at the top level and it will traverse into the subdirectories to build the source code as needed.  In addition the ESPA raw binary libraries and tools in the [espa\_common Google Project](https://code.google.com/p/espa-common/) are needed.  The user will need to build the src/raw\_binary code to create the libraries used for building the current version of the spectral indices application.

### Dependencies ###
Same dependencies as for Version 1.0.0 with the addition of the ESPA raw binary libraries and tools in the [espa\_common Google Project](https://code.google.com/p/espa-common/).  The user will need to build the src/raw\_binary code to create the libraries used for building the current version of the spectral indices application.

### Data Preprocessing ###
This version of the burned area application requires the input Landsat products to be in the ESPA internal file format.

### Data Postprocessing ###
After compiling the espa-common raw\_binary libraries and tools, the convert\_espa\_to\_gtif and convert\_espa\_to\_hdf command-line tools can be used to convert the ESPA internal file format to HDF or GeoTIFF.  Otherwise the data will remain in the ESPA internal file format, which includes each band in the ENVI file format (i.e. raw binary file with associated ENVI header file) and an overall XML metadata file.

### Associated Scripts ###
Same scripts as for Version 1.0.0, updated for 1.1.0.  Added a script called do\_burn\_area.py which was developed to handle the end-to-end processing of burned area products.  The LPGS GeoTIFF products need to be processed to the ESPA internal file format (using convert\_espa\_to\_gtif), then the LEDAPS source code needs to be run to generate the TOA and Surface Reflectance bands.

### Verification Data ###

### User Manual ###

### Product Guide ###


## Changes From Previous Version ##
#### Updates on June 9, 2014 - USGS EROS ####
  * burned-area
    1. Modified overall script for burned area handle the modifications for the ESPA internal file format.
    1. Modified to support a number of processors argument to pass along for multi-threading.

  * seasonal-summary
    1. Modified seasonal\_summary/determine\_max\_extent to read and process a list of XML files instead of a list of HDF files.
    1. Added seasonal\_summary/generate\_stack to generate the stack of metadata for each of the XML files.
    1. Modified seasonal\_summary/process\_temporal\_ba\_stack.py to process XML files instead of HDF files and to write ENVI files vs. GeoTIFF.  The conversion of HDF to GeoTIFF is no longer needed.

  * boosted-regression
    1. Modified boosted\_regression to use the ESPA internal file format.
    1. Modified to take the resampled scenes as input so that the resampling of annual maximums and seasonal summaries to the scene boundaries is no longer needed.  The last step of the BA process is run at the maximum geographic extent boundary, so this mod makes sense and saves resampling time.
    1. Modified to use the single QA mask generated as part of the seasonal summaries vs. the multiple QA masks from the original scene.
    1. Added a multi-threading loop to the overall burn area processing script to support multi-threaded processing of scenes through boosted-regression.
    1. Cleaned up code that was no longer necessary for the HDF metadata.

  * burn-threshold
    1. Modified burn thresholding to use the ESPA internal file format.
    1. Modified to expect that the burn probabilities from the boosted regression step will be produced at a common geographic extent for all the scenes in the stack.  Thus, resampling of the seasonal summaries and annual maximums is not needed, and neither is resampling of the individual scenes themselves.
    1. Added support for multi-threading.