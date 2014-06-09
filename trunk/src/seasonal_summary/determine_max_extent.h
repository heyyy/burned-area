#ifndef _DETERMINE_MAX_EXTENT_H_
#define _DETERMINE_MAX_EXTENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "error_handler.h"
#include "raw_binary_io.h"
#include "espa_metadata.h"
#include "parse_metadata.h"

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
    char *xml_infile,  /* I: input XML file to open and read */
    double *east,      /* O: eastern projection coordinate of the file */
    double *west,      /* O: western projection coordinate of the file */
    double *north,     /* O: northern projection coordinate of the file */
    double *south      /* O: southern projection coordinate of the file */
);

#endif
