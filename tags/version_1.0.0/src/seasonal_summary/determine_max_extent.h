#ifndef _DETERMINE_MAX_EXTENT_H_
#define _DETERMINE_MAX_EXTENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bool.h"
#include "error_handler.h"

#define STR_SIZE 1024

/* Prototypes */
void usage ();

short get_args
(
    int argc,             /* I: number of cmd-line args */
    char *argv[],         /* I: string of cmd-line args */
    char **list_infile,   /* O: address of input list filename */
    char **extents_outfile, /* O: address of output extents filename */
    bool *verbose         /* O: verbose flag */
);

int read_extent
(
    char *infile,    /* I: input reflectance file to open and process */
    char *grid_name, /* I: name of the grid to read metadata from */
    double *east,    /* O: eastern projection coordinate of the file */
    double *west,    /* O: western projection coordinate of the file */
    double *north,   /* O: northern projection coordinate of the file */
    double *south    /* O: southern projection coordinate of the file */
);

#endif
