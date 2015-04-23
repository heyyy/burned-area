## Burned-Area Version 1.1.0 Release Notes ##
Release Date: March 7, 2014

### Downloads ###

**Burned area source code - available via the [burned-area Google Projects Source](https://code.google.com/p/burned-area/source/checkout) link**

  * Non-members may check out a read-only working copy anonymously over HTTP.
  * svn checkout http://burned-area.googlecode.com/svn/tags/version_1.1.1 burned-area-read-only

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
#### Updates on March 7, 2014 - USGS EROS ####
  * burned-area
    1. Modified overall script for burned area to not try to process boosted regression for excluded L1G files.
  * seasonal-summary
    1. Modified overall script for burned area to not try to process boosted regression for excluded L1G files.