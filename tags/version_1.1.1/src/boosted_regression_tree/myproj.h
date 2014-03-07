/*****************************************************************************
FILE: myproj.h

PURPOSE: Header file for declaring map projection structures and some constants

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
9/15/2012   Jodi Riegle      Original development (based largely on routines
                             from the LEDAPS lndsr application)
9/3/2013    Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/

#ifndef MYPROJ_H
#define MYPROJ_H

/* Constant */
#define PROJ_NSPHERE (20)

/* Structure to store information for the supported spheres */
typedef struct {
  char *name;
  double major_axis;
  double minor_axis;
} Proj_sphere_t;

/* Structure to store information for the supported map projections */
typedef struct {
  int num;
  char *short_name;
  char *name;
} Proj_type_t;

/* Structure to store information for the map projection parameters */
typedef struct {
  int itype;
  char *short_name;
  char *name;
} Proj_param_type_t;

/* Constants */
enum {
  PROJ_GEO,    PROJ_UTM,     PROJ_SPCS,   PROJ_ALBERS, PROJ_LAMCC,
  PROJ_MERCAT, PROJ_PS ,     PROJ_POLYC,  PROJ_EQUIDC, PROJ_TM, 
  PROJ_STEREO, PROJ_LAMAZ,   PROJ_AZMEQD, PROJ_GNOMON, PROJ_ORTHO, 
  PROJ_GVNSP,  PROJ_SNSOID,  PROJ_EQRECT, PROJ_MILLER, PROJ_VGRINT, 
  PROJ_HOM,    PROJ_ROBIN,   PROJ_SOM,    PROJ_ALASKA, PROJ_GOODE, 
  PROJ_MOLL,   PROJ_IMOLL,   PROJ_HAMMER, PROJ_WAGIV,  PROJ_WAGVII, 
  PROJ_OBEQA,  PROJ_FILL,    PROJ_NPROJ,  PROJ_ISINUS = 99};

#define PROJ_HOM_ALT  (PROJ_NPROJ)
#define PROJ_SOM_ALT (PROJ_NPROJ + 1)

enum {
  PROJ_PARAM_NULL, 
  PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS, 
  PROJ_PARAM_RADIUS,
  PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, 
  PROJ_PARAM_LAT1,          PROJ_PARAM_LAT2, 
  PROJ_PARAM_CENTER_LONG,   PROJ_PARAM_LAT_ORIGIN, 
  PROJ_PARAM_MODE,          PROJ_PARAM_SCALE_FACTOR, 
  PROJ_PARAM_CENTER_LAT,    PROJ_PARAM_AZIMUTH, 
  PROJ_PARAM_LON1,          PROJ_PARAM_LON2, 
  PROJ_PARAM_LON_ORIGIN,    PROJ_PARAM_H,
  PROJ_PARAM_PATH,          PROJ_PARAM_SATNUM, 
  PROJ_PARAM_ALF,           PROJ_PARAM_TIME, 
  PROJ_PARAM_SAT_RATIO,     PROJ_PARAM_START, 
  PROJ_PARAM_SHAPE_M,       PROJ_PARAM_SHAPE_N, 
  PROJ_PARAM_ANGLE,         PROJ_PARAM_NZONE, 
  PROJ_PARAM_JUSTIFY,       PROJ_PARAM_NTYPE}; 

#endif
