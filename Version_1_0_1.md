## Burned-Area Version 1.0.1 Release Notes ##
Release Date: November 5, 2013

### Downloads ###

**Burned area source code - available via the [burned-area Google Projects Source](https://code.google.com/p/burned-area/source/checkout) link**

### Installation ###
Same installation instructions as for Version 1.0.0

### Dependencies ###
Same dependencies as for Version 1.0.0

### Associated Scripts ###
Same scripts as for Version 1.0.0, updated for 1.0.1.

### Verification Data ###

### User Manual ###

### Product Guide ###


## Changes From Previous Version ##
#### Updates on November 5, 2013 - USGS EROS ####
  * scripts/boosted\_regression\_tree
    1. Added a Python script to assist in processing the boosted regression model by generating the configuration file needed for each scene.
  * scripts/seasonal\_summary
    1. Modified the resized filenames in the boosted regression algorithm to be “unique” so that multiple files could be processed simultaneously for the same path/row.
    1. Modified the seasonal summaries to produce fill products for seasons/years which don’t contain any valid inputs.