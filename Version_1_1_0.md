## Burned-Area Version 1.1.0 Release Notes ##
Release Date: January 8, 2014

### Downloads ###

**Burned area source code - available via the [burned-area Google Projects Source](https://code.google.com/p/burned-area/source/checkout) link**

  * Non-members may check out a read-only working copy anonymously over HTTP.
  * svn checkout http://burned-area.googlecode.com/svn/tags/version_1.1.0 burned-area-read-only

### Installation ###
Same installation instructions as for Version 1.0.0, however a high-level Makefile is now available for the overall burned-area source code.  Thus the user can utilize the Makefile at the top level and it will traverse into the subdirectories to build the source code as needed.

### Dependencies ###
Same dependencies as for Version 1.0.0

### Associated Scripts ###
Same scripts as for Version 1.0.0, updated for 1.1.0.  Added a script called do\_burn\_area.py which was developed to handle the end-to-end processing of burned area products.

### Verification Data ###

### User Manual ###

### Product Guide ###


## Changes From Previous Version ##
#### Updates on January 8, 2014 - USGS EROS ####
  * burned-area
    1. Created a high-level Makefile to handle the overall build.  Modified the lower-level Makefiles to use install instead of cp to copy the executables and Python scripts to the $BIN directory.
    1. Added a script to handle the overall processing of burned area from end to end, called do\_burned\_area.py.
  * scripts/boosted\_regression\_tree
    1. Modified to utilize the cfmask vs. the LEDAPS QA bands.
    1. Based on feedback from the Lead scientist, pulled the cfmask usage back out and went back to LEDAPS QA bands.  cfmask wasn't identifying the cloud shadows as well as desired.
    1. Added a cmd-line flag which allows the L1G files to be excluded from the processing. The geolocation accuracy of these L1G files, in some cases, may cause the scene to not line up with the other scenes.
  * src/boosted\_regression\_tree
    1. Added the adjacent\_cloud QA bit to be masked from processing the pixel as a fire pixel.
  * scripts/seasonal\_summary
    1. Modified to utilize the cfmask vs. the LEDAPS QA bands.
    1. Based on feedback from the Lead scientist, pulled the cfmask usage back out and went back to LEDAPS QA bands.  cfmask wasn't identifying the cloud shadows as well as desired.
  * scripts/burn\_threshold
    1. This is a new addition to the burned area code.
    1. Added source code for third and final processing step which filters out the false positives from the boosted regression tree results and creates annual fire summaries.
    1. Added a cmd-line flag which allows the L1G files to be excluded from the processing. The geolocation accuracy of these L1G files, in some cases, may cause the scene to not line up with the other scenes.