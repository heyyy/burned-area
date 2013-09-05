/*****************************************************************************
FILE: PredictBurnedArea.cpp
  
PURPOSE: Contains constructors and descructors for the PredictBurnedArea class.
Note that other class modules are contained in input.cpp, output.cpp, etc.,
depending on their functionality.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
11/26/2012    Jodi Riegle      Original development
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "PredictBurnedArea.h"
#include "output.h"
#include "input.h"

PredictBurnedArea::PredictBurnedArea() {
    trueCnt = 0;
}

PredictBurnedArea::~PredictBurnedArea() {
}

