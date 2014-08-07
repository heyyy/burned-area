/*****************************************************************************
FILE: myproj_const.h

PURPOSE: Header file for declaring constants describing the map projections,
spheres, and map projection parameters.

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

#ifndef PROJ_CONST_H
#define PROJ_CONST_H

#include "myproj.h"

/* Constants describing the supported spheres */
const Proj_sphere_t Proj_sphere[PROJ_NSPHERE] = { 
  {(char *)"Clarke 1866",           6378206.4,    6356583.8},  
  {(char *)"Clarke 1880",           6378249.145,  6356514.86955},
  {(char *)"Bessel",                6377397.155,  6356078.96284}, 
  {(char *)"International 1967",    6378157.5,    6356772.2}, 
  {(char *)"International 1909",    6378388.0,    6356911.94613}, 
  {(char *)"WGS 72",                6378135.0,    6356750.519915}, 
  {(char *)"Everest",               6377276.3452, 6356075.4133}, 
  {(char *)"WGS 66",                6378145.0,    6356759.769356}, 
  {(char *)"GRS 1980/WGS 84",       6378137.0,    6356752.31414}, 
  {(char *)"Airy",                  6377563.396,  6356256.91}, 
  {(char *)"Modified Everest",      6377304.063,  6356103.039},  
  {(char *)"Modified Airy",         6377340.189,  6356034.448}, 
  {(char *)"Walbeck",               6378137.0,    6356752.314245}, 
  {(char *)"Southeast Asia",        6378155.0,    6356773.3205}, 
  {(char *)"Australian National",   6378160.0,    6356774.719}, 
  {(char *)"Krassovsky",            6378245.0,    6356863.0188}, 
  {(char *)"Hough",                 6378270.0,    6356794.343479}, 
  {(char *)"Mercury 1960",          6378166.0,    6356784.283666}, 
  {(char *)"Modified Mercury 1968", 6378150.0,    6356768.337303}, 
  {(char *)"Sphere of Radius 6370997 meters", 6370997.0, 6370997.0}};

/* Constants describing the supported map projections */
const Proj_type_t Proj_type[PROJ_NPROJ] = {
  {PROJ_GEO,    (char *)"GEO",    (char *)"Geographic"}, 
  {PROJ_UTM,    (char *)"UTM",    (char *)"Universal Transverse Mercator (UTM)"}, 
  {PROJ_SPCS,   (char *)"SPCS",   (char *)"State Plane Coordinates"}, 
  {PROJ_ALBERS, (char *)"ALBERS", (char *)"Albers Conical Equal Area"}, 
  {PROJ_LAMCC,  (char *)"LAMCC",  (char *)"Lambert Conformal Conic"}, 
  {PROJ_MERCAT, (char *)"MERCAT", (char *)"Mercator"}, 
  {PROJ_PS ,    (char *)"PS",     (char *)"Polar Stereographic"}, 
  {PROJ_POLYC,  (char *)"POLYC",  (char *)"Polyconic"}, 
  {PROJ_EQUIDC, (char *)"EQUIDC", (char *)"Equidistant Conic"}, 
  {PROJ_TM,     (char *)"TM",     (char *)"Transverse Mercator"}, 
  {PROJ_STEREO, (char *)"STEREO", (char *)"Stereographic"}, 
  {PROJ_LAMAZ,  (char *)"LAMAZ",  (char *)"Lambert Azimuthal Equal Area"}, 
  {PROJ_AZMEQD, (char *)"AZMEQD", (char *)"Azimuthal Equidistant"}, 
  {PROJ_GNOMON, (char *)"GNOMON", (char *)"Gnomonic"}, 
  {PROJ_ORTHO,  (char *)"ORTHO",  (char *)"Orthographic"}, 
  {PROJ_GVNSP,  (char *)"GVNSP",  (char *)"General Vertical Near-Side Perspective"}, 
  {PROJ_SNSOID, (char *)"SNSOID", (char *)"Sinusoidal"}, 
  {PROJ_EQRECT, (char *)"EQRECT", (char *)"Equirectangular"}, 
  {PROJ_MILLER, (char *)"MILLER", (char *)"Miller Cylindrical"}, 
  {PROJ_VGRINT, (char *)"VGRINT", (char *)"Van der Grinten"}, 
  {PROJ_HOM,    (char *)"HOM",    (char *)"(Hotine) Oblique Mercator"}, 
  {PROJ_ROBIN,  (char *)"ROBIN",  (char *)"Robinson"}, 
  {PROJ_SOM,    (char *)"SOM",    (char *)"Space Oblique Mercator (SOM)"}, 
  {PROJ_ALASKA, (char *)"ALASKA", (char *)"Alaska Conformal"}, 
  {PROJ_GOODE,  (char *)"GOODE",  (char *)"Interrupted Goode Homolosine"}, 
  {PROJ_MOLL,   (char *)"MOLL",   (char *)"Mollweide"}, 
  {PROJ_IMOLL,  (char *)"IMOLL",  (char *)"Interrupted Mollweide"}, 
  {PROJ_HAMMER, (char *)"HAMMER", (char *)"Hammer"}, 
  {PROJ_WAGIV,  (char *)"WAGIV",  (char *)"Wagner IV"}, 
  {PROJ_WAGVII, (char *)"WAGVII", (char *)"Wagner VII"}, 
  {PROJ_OBEQA,  (char *)"OBEQA",  (char *)"Oblated Equal Area"}, 
  {PROJ_ISINUS, (char *)"ISINUS", (char *)"Integerized Sinusoidial"}};

/* Constants describing the map projection parameters */
const Proj_param_type_t Proj_param_type[PROJ_PARAM_NTYPE] = {
  {PROJ_PARAM_NULL,           (char *)"NULL",           (char *)"(null value)"}, 
  {PROJ_PARAM_MAJOR_AXIS,     (char *)"MAJOR_AXIS",     (char *)"Major axis (meters)"}, 
  {PROJ_PARAM_MINOR_AXIS,     (char *)"MINOR_AXIS",     (char *)"Minor axis (meters)"},  
  {PROJ_PARAM_RADIUS,         (char *)"RADIUS",         (char *)"Radius (meters)"},  
  {PROJ_PARAM_FALSE_EASTING,  (char *)"FALSE_EASTING",  (char *)"False easting (meters)"}, 
  {PROJ_PARAM_FALSE_NORTHING, (char *)"FALSE_NORTHING", (char *)"False northing (meters)"}, 
  {PROJ_PARAM_LAT1,           (char *)"LAT1",           (char *)"1st standard parallel"}, 
  {PROJ_PARAM_LAT2,           (char *)"LAT2",           (char *)"2nd standard parallel"}, 
  {PROJ_PARAM_CENTER_LONG,    (char *)"CENTER_LONG",    (char *)"Center longitude"}, 
  {PROJ_PARAM_LAT_ORIGIN,     (char *)"LAT_ORIGIN",     (char *)"Latitude at origin"}, 
  {PROJ_PARAM_MODE,           (char *)"MODE",           (char *) "Initialization method to use"}, 
  {PROJ_PARAM_SCALE_FACTOR,   (char *)"SCALE_FACTOR",   (char *)"Scale factor"}, 
  {PROJ_PARAM_CENTER_LAT,     (char *)"CENTER_LAT",     (char *)"Center latitude"}, 
  {PROJ_PARAM_AZIMUTH,        (char *)"AZIMUTH",        (char *)"Azimuth"}, 
  {PROJ_PARAM_LON1,           (char *)"LON1",           (char *)"1st longitude"}, 
  {PROJ_PARAM_LON2,           (char *)"LON2",           (char *)"2nd longitude"}, 
  {PROJ_PARAM_LON_ORIGIN,     (char *)"LON_ORIGIN",     (char *)"Longitude at origin"}, 
  {PROJ_PARAM_H,              (char *)"H",              (char *)"Height above sphere"}, 
  {PROJ_PARAM_PATH,           (char *)"PATH",           (char *)"Path number"},  
  {PROJ_PARAM_SATNUM,         (char *)"SATNUM",         (char *)"Satellite number"}, 
  {PROJ_PARAM_ALF,            (char *)"ALF",            (char *)"Angle"},
  {PROJ_PARAM_TIME,           (char *)"TIME",           (char *)"Time"},
  {PROJ_PARAM_SAT_RATIO,      (char *)"SAT_RATIO",      (char *)"Satellite ratio which specifies the start point"}, 
  {PROJ_PARAM_START,          (char *)"START",          (char *)"Start flag - beginning or end"}, 
  {PROJ_PARAM_SHAPE_M,        (char *)"SHAPE_M",        (char *)"Constant 'm'"}, 
  {PROJ_PARAM_SHAPE_N,        (char *)"SHAPE_N",        (char *)"Constant 'n'"}, 
  {PROJ_PARAM_ANGLE,          (char *)"ANGLE",          (char *)"Rotation angle"}, 
  {PROJ_PARAM_NZONE,          (char *)"NZONE",          (char *)"Number of longitudinal zones"}, 
  {PROJ_PARAM_JUSTIFY,        (char *)"JUSTIFY",        (char *)"Justify flag"}}; 

/* Constants describing the map projection parameters for each map projection */
#define PROJ_NPARAM (15)

/* Need special cases for utm: scale_factor always 0.9996           */
/*                         if (zone == 0) zone calculated with      */
/*                            outparam[0]: long, outparam[1]: lat   */
/*                            if outdatum < 0: clarke_1866)         */
/* Need special case for SPCS: ...)                                 */
 
const int Proj_param_value_type[PROJ_NPROJ + 2][PROJ_NPARAM] = {
  /* GEO */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL}, 
  /* UTM */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* SPCS */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* ALBERS */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_LAT1,
   PROJ_PARAM_LAT2,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT_ORIGIN, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* LAMCC */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_LAT1,
   PROJ_PARAM_LAT2,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT_ORIGIN, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* MERCAT */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT1,
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* PS */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT1,
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* POLYC */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT_ORIGIN,
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* EQUIDC */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_LAT1,
   PROJ_PARAM_LAT2,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT_ORIGIN, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_MODE, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* TM */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_SCALE_FACTOR,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT_ORIGIN,
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* STEREO */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_CENTER_LAT, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* LAMAZ */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_CENTER_LAT, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* AZMEQD */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_CENTER_LAT, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* GNOMON */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_CENTER_LAT, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* ORTHO */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_CENTER_LAT, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* GVNSP */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_H,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_CENTER_LAT, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* SNSOID */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* EQRECT */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_LAT1, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* MILLER */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* VGRINT */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* HOM */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_SCALE_FACTOR,
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_LAT_ORIGIN, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_LON1, 
   PROJ_PARAM_LAT1,          PROJ_PARAM_LON2,           PROJ_PARAM_LAT2, 
   PROJ_PARAM_MODE,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* ROBIN */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* SOM */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_SATNUM,
   PROJ_PARAM_PATH,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_MODE,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* ALASKA */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* GOODE */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* MOLL */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* IMOLL */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* HAMMER */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* WAGIV */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* WAGVII */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* OBEQA */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_SHAPE_M,
   PROJ_PARAM_SHAPE_N,       PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_CENTER_LAT, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_ANGLE, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* ISINUS */
  {PROJ_PARAM_RADIUS,        PROJ_PARAM_NULL,           PROJ_PARAM_NULL,
   PROJ_PARAM_NULL,          PROJ_PARAM_CENTER_LONG,    PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NZONE, 
   PROJ_PARAM_NULL,          PROJ_PARAM_JUSTIFY,        PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* HOM_ALT */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_SCALE_FACTOR,
   PROJ_PARAM_AZIMUTH,       PROJ_PARAM_LON_ORIGIN,     PROJ_PARAM_LAT_ORIGIN, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_NULL, 
   PROJ_PARAM_NULL,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL, 
   PROJ_PARAM_MODE,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL},
  /* SOM_ALT */
  {PROJ_PARAM_MAJOR_AXIS,    PROJ_PARAM_MINOR_AXIS,     PROJ_PARAM_SATNUM,
   PROJ_PARAM_ALF,           PROJ_PARAM_LON1,           PROJ_PARAM_NULL, 
   PROJ_PARAM_FALSE_EASTING, PROJ_PARAM_FALSE_NORTHING, PROJ_PARAM_TIME, 
   PROJ_PARAM_SAT_RATIO,     PROJ_PARAM_START,          PROJ_PARAM_NULL, 
   PROJ_PARAM_MODE,          PROJ_PARAM_NULL,           PROJ_PARAM_NULL}};
#endif
