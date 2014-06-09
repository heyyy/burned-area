#ifndef _GENERATE_STACK_H_
#define _GENERATE_STACK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "error_handler.h"
#include "raw_binary_io.h"
#include "espa_metadata.h"
#include "parse_metadata.h"

/* Local defines */
typedef struct
{
    int day;
    int month;
    int year;
    int doy;
} Ba_date_t;

typedef struct
{
    char filename[STR_SIZE];   /* name of the input file */
    Ba_date_t acq_date;        /* acquisition date */
    char season[STR_SIZE];     /* season for this scene (winter, spring,
                                  summer, fall) */
    int wrs_path;              /* WRS path of this scene */
    int wrs_row;               /* WRS row of this scene */
    char satellite[STR_SIZE];  /* name of satellite (LANDSAT_4, LANDSAT_5,
                                  LANDSAT_7) */
    double bounding_coords[4]; /* geographic west, east, north, south */
    int nlines;                /* number of lines in the dataset */
    int nsamps;                /* number of samples in the dataset */
    float pixel_size[2];       /* pixel size (x, y) */
    int utm_zone;              /* UTM zone; use a negative number if this is a
                                  southern zone */
} Ba_scene_meta_t;

/* Prototypes */
void usage ();

short get_args
(
    int argc,             /* I: number of cmd-line args */
    char *argv[],         /* I: string of cmd-line args */
    char **list_infile,   /* O: address of input list filename */
    char **stack_file,    /* O: address of output stack filename */
    bool *verbose         /* O: verbose flag */
);

int read_xml
(
    char *xml_infile,            /* I: input XML file to open and read */
    Ba_scene_meta_t *scene_meta  /* O: scene metadata */
);

#endif
