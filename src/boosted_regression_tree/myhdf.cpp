/*****************************************************************************
FILE: myhdf.cpp
  
PURPOSE: Contains functions for handling HDF files.

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

#include <stdlib.h>
#include "myhdf.h"
#include "error.h"
#include "hdf.h"
#include "mfhdf.h"
#include "mystring.h"

/* Constants */

#define DIM_MAX_NCHAR (80)  /* Maximum size of a dimension name */

/* Possible ranges for data types */

#define MYHDF_CHAR8H     (        255  )
#define MYHDF_CHAR8L     (          0  )
#define MYHDF_INT8H      (        127  )
#define MYHDF_INT8L      (       -128  )
#define MYHDF_UINT8H     (        255  )
#define MYHDF_UINT8L     (          0  )
#define MYHDF_INT16H     (      32767  )
#define MYHDF_INT16L     (     -32768  )
#define MYHDF_UINT16H    (      65535u )
#define MYHDF_UINT16L    (          0u )
#define MYHDF_INT32H     ( 2147483647l )
#define MYHDF_INT32L     (-2147483647l )
#define MYHDF_UINT32H    ( 4294967295ul)
#define MYHDF_UINT32L    (          0ul)
#define MYHDF_FLOAT32H   (3.4028234e+38f)
#define MYHDF_FLOAT32L   (1.1754943e-38f)
#define MYHDF_FLOAT64H   (1.797693134862316e+308)
#define MYHDF_FLOAT64L   (2.225073858507201e-308)

/******************************************************************************
MODULE: GetSDSInfo

PURPOSE: Reads information for a specific SDS.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading SDS info
true           Successful reading of SDS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool GetSDSInfo
(
  int32 sds_file_id,   /* I: SDS file ID */
  Myhdf_sds_t *sds     /* O: SDS data structure */
)
{
  int32 dims[MYHDF_MAX_RANK];

  sds->index = SDnametoindex(sds_file_id, sds->name);
  if (sds->index == HDF_ERROR)
    RETURN_ERROR("getting sds index", "GetSDSInfo", false);

  sds->id = SDselect(sds_file_id, sds->index);
  if (sds->id == HDF_ERROR)
    RETURN_ERROR("getting sds id", "GetSDSInfo", false);

  if (SDgetinfo(sds->id, sds->name, &sds->rank, dims, &sds->type, &sds->nattr)
      == HDF_ERROR) {
    SDendaccess(sds->id);
    RETURN_ERROR("getting sds information", "GetSDSInfo", false);
  }
  if (sds->rank > MYHDF_MAX_RANK) {
    SDendaccess(sds->id);
    RETURN_ERROR("sds rank too large", "GetSDSInfo", false);
  }
  return true;
}


/******************************************************************************
MODULE: GetSDSDimInfo

PURPOSE: Reads information for a specific SDS dimension.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading SDS dimension info
true           Successful reading of SDS dimension

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool GetSDSDimInfo
(
  int32 sds_id,       /* I: SDS ID */
  Myhdf_dim_t *dim,   /* O: dimension information */
  int irank           /* I: rank/size of dimension */
)
{
  char dim_name[DIM_MAX_NCHAR];

  dim->id = SDgetdimid(sds_id, irank);
  if (dim->id == HDF_ERROR) 
    RETURN_ERROR("getting dimension id", "GetSDSDimInfo", false);

  if (SDdiminfo(dim->id, dim_name,
                &dim->nval, &dim->type, 
            &dim->nattr) == HDF_ERROR)
      RETURN_ERROR("getting dimension information", "GetSDSDimInfo", false);

  dim->name = DupString(dim_name);
  if (dim->name == (char *)NULL)
    RETURN_ERROR("copying dimension name", "GetSDSDimInfo", false);

  return true;
}


/******************************************************************************
MODULE: PutSDSInfo

PURPOSE: Creates an SDS and writes SDS information.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error creating SDS or writing SDS info
true           Successful creation of SDS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool PutSDSInfo
(
  int32 sds_file_id,   /* I: SDS file ID */
  Myhdf_sds_t *sds     /* I: SDS data structure */
)
{
  int irank;
  int32 dims[MYHDF_MAX_RANK];

  for (irank = 0; irank < sds->rank; irank++)
    dims[irank] = sds->dim[irank].nval;

  /* Create the SDS */
  sds->id = SDcreate(sds_file_id, sds->name, sds->type, 
                     sds->rank, dims);
  if (sds->id == HDF_ERROR)
    RETURN_ERROR("Creating sds", "PutSDSInfo", false);

  sds->index = SDnametoindex(sds_file_id, sds->name);
  if (sds->index == HDF_ERROR)
    RETURN_ERROR("Getting sds index", "PutSDSInfo", false);

  return true;
}


/******************************************************************************
MODULE: PutSDSDimInfo

PURPOSE: Writes information for a specific SDS dimension.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error writing SDS dimension info
true           Successful writing of SDS dimension

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool PutSDSDimInfo
(
  int32 sds_id,       /* I: SDS ID */
  Myhdf_dim_t *dim,   /* I: dimension information */
  int irank           /* I: rank/size of dimension */
)
{

  dim->id = SDgetdimid(sds_id, irank);
  if (dim->id == HDF_ERROR) 
    RETURN_ERROR("getting dimension id", "PutSDSDimInfo", false);

  if (SDsetdimname(dim->id, dim->name) == HDF_ERROR)
    RETURN_ERROR("setting dimension name", "PutSDSDimInfo", false);

  /* Set dimension type */

    /* !! do it !! */
 
  return true;
}


/******************************************************************************
MODULE: GetAttrDouble

PURPOSE: Reads an attribute into a parameter of type 'double'.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading the attribute
true           Successful reading of the attribute

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool GetAttrDouble
(
  int32 sds_id,         /* I: SDS ID */
  Myhdf_attr_t *attr,   /* I: attribute data structure */
  double *val           /* O: array of values from the HDF attribute, converted
                              from the native data type to 'double' */
)
{
  char8 val_char8[MYHDF_MAX_NATTR_VAL];
  uint8 val_int8[MYHDF_MAX_NATTR_VAL];
  uint8 val_uint8[MYHDF_MAX_NATTR_VAL];
  int16 val_int16[MYHDF_MAX_NATTR_VAL];
  uint16 val_uint16[MYHDF_MAX_NATTR_VAL];
  int32 val_int32[MYHDF_MAX_NATTR_VAL];
  uint32 val_uint32[MYHDF_MAX_NATTR_VAL];
  float32 val_float32[MYHDF_MAX_NATTR_VAL];
  float64 val_float64[MYHDF_MAX_NATTR_VAL];
  int i;
  char z_name[80];
  
  if ((attr->id = SDfindattr(sds_id, attr->name)) == HDF_ERROR)
    RETURN_ERROR("getting attribute id", "GetAttrDouble", false);
  if (SDattrinfo(sds_id, attr->id, z_name, &attr->type, &attr->nval) == 
      HDF_ERROR)
    RETURN_ERROR("getting attribute info", "GetAttrDouble", false);
  /* printf("attr name: %s\n", z_name); */

  if (attr->nval < 1)
    RETURN_ERROR("no attribute value", "GetAttrDouble", false);
  if (attr->nval > MYHDF_MAX_NATTR_VAL) 
    RETURN_ERROR("too many attribute values", "GetAttrDouble", false);

  switch (attr->type) {
  case DFNT_CHAR8:
    if (SDreadattr(sds_id, attr->id, val_char8) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (char8)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_char8[i];
    break;
  case DFNT_INT8:
    if (SDreadattr(sds_id, attr->id, val_int8) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (int8)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_int8[i];
    break;
  case DFNT_UINT8:
    if (SDreadattr(sds_id, attr->id, val_uint8) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (uint8)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_uint8[i];
    break;
  case DFNT_INT16:
    if (SDreadattr(sds_id, attr->id, val_int16) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (int16)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_int16[i];
    break;
  case DFNT_UINT16:
    if (SDreadattr(sds_id, attr->id, val_uint16) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (uint16)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_uint16[i];
    break;
  case DFNT_INT32:
    if (SDreadattr(sds_id, attr->id, val_int32) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (int32)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_int32[i];
    break;
  case DFNT_UINT32:
    if (SDreadattr(sds_id, attr->id, val_uint32) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (uint32)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_uint32[i];
    break;
  case DFNT_FLOAT32:
    if (SDreadattr(sds_id, attr->id, val_float32) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (float32)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_float32[i];
    break;
  case DFNT_FLOAT64:
    if (SDreadattr(sds_id, attr->id, val_float64) == HDF_ERROR) 
      RETURN_ERROR("reading attribute (float64)", "GetAttrDouble", false);
    for (i = 0; i < attr->nval; i++) 
      val[i] = (double)val_float64[i];
    break;
  default:
    RETURN_ERROR("unknown attribute type", "GetAttrDouble", false);
  }

  return true;
}


/******************************************************************************
MODULE: PutAttrDouble

PURPOSE: Writes an attribute from a parameter of type 'double' to an HDF file.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error writing the attribute
true           Successful writing of the attribute

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool PutAttrDouble
(
  int32 sds_id,         /* I: SDS ID */
  Myhdf_attr_t *attr,   /* I: attribute data structure */
  double *val           /* I: array of 'double' values to be written to the HDF
                              file; converted to the specified data type */
)
{
  char8 val_char8[MYHDF_MAX_NATTR_VAL];
  int8 val_int8[MYHDF_MAX_NATTR_VAL];
  uint8 val_uint8[MYHDF_MAX_NATTR_VAL];
  int16 val_int16[MYHDF_MAX_NATTR_VAL];
  uint16 val_uint16[MYHDF_MAX_NATTR_VAL];
  int32 val_int32[MYHDF_MAX_NATTR_VAL];
  uint32 val_uint32[MYHDF_MAX_NATTR_VAL];
  float32 val_float32[MYHDF_MAX_NATTR_VAL];
  float64 val_float64[MYHDF_MAX_NATTR_VAL];
  int i;
  void *buf = NULL;

  if (attr->nval <= 0  ||  attr->nval > MYHDF_MAX_NATTR_VAL) 
    RETURN_ERROR("invalid number of values", "PutAttrDouble", false);

  switch (attr->type) {
    case DFNT_CHAR8:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_CHAR8H)) val_char8[i] = MYHDF_CHAR8H;
        else if (val[i] <= ((double)MYHDF_CHAR8L)) val_char8[i] = MYHDF_CHAR8L;
        else if (val[i] >= 0.0) val_char8[i] = (char8)(val[i] + 0.5);
        else                    val_char8[i] = -((char8)(-val[i] + 0.5));
      }
      buf = (void *)val_char8;
      break;

    case DFNT_INT8:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_INT8H)) val_int8[i] = MYHDF_INT8H;
        else if (val[i] <= ((double)MYHDF_INT8L)) val_int8[i] = MYHDF_INT8L;
        else if (val[i] >= 0.0) val_int8[i] =   (int8)( val[i] + 0.5);
        else                    val_int8[i] = -((int8)(-val[i] + 0.5));
      }
      buf = (void *)val_int8;
      break;

    case DFNT_UINT8:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_UINT8H)) val_uint8[i] = MYHDF_UINT8H;
        else if (val[i] <= ((double)MYHDF_UINT8L)) val_uint8[i] = MYHDF_UINT8L;
        else if (val[i] >= 0.0) val_uint8[i] =   (uint8)( val[i] + 0.5);
        else                    val_uint8[i] = -((uint8)(-val[i] + 0.5));
      }
      buf = (void *)val_uint8;
      break;

    case DFNT_INT16:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_INT16H)) val_int16[i] = MYHDF_INT16H;
        else if (val[i] <= ((double)MYHDF_INT16L)) val_int16[i] = MYHDF_INT16L;
        else if (val[i] >= 0.0) val_int16[i] =   (int16)( val[i] + 0.5);
        else                    val_int16[i] = -((int16)(-val[i] + 0.5));
      }
      buf = (void *)val_int16;
      break;

    case DFNT_UINT16:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_UINT16H)) val_uint16[i] = MYHDF_UINT16H;
        else if (val[i] <= ((double)MYHDF_UINT16L)) val_uint16[i] = MYHDF_UINT16L;
        else if (val[i] >= 0.0) val_uint16[i] =   (uint16)( val[i] + 0.5);
        else                    val_uint16[i] = -((uint16)(-val[i] + 0.5));
      }
      buf = (void *)val_uint16;
      break;

    case DFNT_INT32:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_INT32H)) val_int32[i] = MYHDF_INT32H;
        else if (val[i] <= ((double)MYHDF_INT32L)) val_int32[i] = MYHDF_INT32L;
        else if (val[i] >= 0.0) val_int32[i] =   (int32)( val[i] + 0.5);
        else                    val_int32[i] = -((int32)(-val[i] + 0.5));
      }
      buf = (void *)val_int32;
      break;

    case DFNT_UINT32:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_UINT32H)) val_uint32[i] = MYHDF_UINT32H;
        else if (val[i] <= ((double)MYHDF_UINT32L)) val_uint32[i] = MYHDF_UINT32L;
        else if (val[i] >= 0.0) val_uint32[i] =   (uint32)( val[i] + 0.5);
        else                    val_uint32[i] = -((uint32)(-val[i] + 0.5));
      }
      buf = (void *)val_uint32;
      break;

    case DFNT_FLOAT32:
      for (i = 0; i < attr->nval; i++) {
        if (     val[i] >= ((double)MYHDF_FLOAT32H)) val_float32[i] = MYHDF_FLOAT32H;
        else if (val[i] <= ((double)MYHDF_FLOAT32L)) val_float32[i] = MYHDF_FLOAT32L;
        else if (val[i] >= 0.0) val_float32[i] =   (float32)( val[i] + 0.5);
        else                    val_float32[i] = -((float32)(-val[i] + 0.5));
      }
      buf = (void *)val_float32;
      break;

    case DFNT_FLOAT64:
      if (sizeof(float64) == sizeof(double))
        buf = (void *)val;
      else {
        for (i = 0; i < attr->nval; i++)
          val_float64[i] = val[i];
        buf = (void *)val_float64;
      }
      break;

    default: 
      RETURN_ERROR("unimplmented type", "PutAttrDouble", false);
  }

  if (SDsetattr(sds_id, attr->name, attr->type, attr->nval, buf) == HDF_ERROR)
    RETURN_ERROR("setting attribute", "PutAttrDouble", false);

  return true;
}


/******************************************************************************
MODULE: GetAttrString

PURPOSE: Reads an attribute into a parameter of type 'char *'.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error reading the attribute
true           Successful reading of the attribute

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool GetAttrString
(
  int32 sds_id,         /* I: SDS ID */
  Myhdf_attr_t *attr,   /* I: attribute data structure */
  char *string          /* O: array of values from the HDF attribute, converted
                              from the native data type to 'char *' */
)
{
  char8 val_char8[MYHDF_MAX_NATTR_VAL];
  int i,i_length;
  char z_name[80];
  void *buf = NULL;
  
  if ((attr->id = SDfindattr(sds_id, attr->name)) == HDF_ERROR)
    RETURN_ERROR("getting attribute id", "GetAttrString", false);
  if (SDattrinfo(sds_id, attr->id, z_name, &attr->type, &attr->nval) == 
      HDF_ERROR)
    RETURN_ERROR("getting attribute info", "GetAttrString", false);
  /* printf("attr name: %s\n", z_name); */

  if (attr->nval < 1)
    RETURN_ERROR("no attribute value", "GetAttrString", false);
  if (attr->nval > MYHDF_MAX_NATTR_VAL) 
    RETURN_ERROR("too many attribute values", "GetAttrString", false);
  if (attr->type != DFNT_CHAR8) 
    RETURN_ERROR("invalid type - not string (char8)", "GetAttrString", false);

  if (sizeof(char8) == sizeof(char))
    buf = (void *)string;
  else
    buf = (void *)val_char8;

  if (SDreadattr(sds_id, attr->id, buf) == HDF_ERROR) 
    RETURN_ERROR("reading attribute", "GetAttrString", false);

  if (sizeof(char8) != sizeof(char)) {
    for (i = 0; i < attr->nval; i++) 
      string[i] = (char)val_char8[i];
  }

  i_length= (int)attr->nval;
  string[i_length]= '\0';

  return true;
}


/******************************************************************************
MODULE: PutAttrString

PURPOSE: Writes an attribute from a parameter of type 'char*' to an HDF file.
 
RETURN VALUE:
Type = bool
Value          Description
-----          -----------
false          Error writing the attribute
true           Successful writing of the attribute

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
9/15/2012     Jodi Riegle      Original development (based largely on routines
                               from the LEDAPS lndsr application)
9/3/2013      Gail Schmidt     Modified to work in the ESPA environment

NOTES:
*****************************************************************************/
bool PutAttrString
(
  int32 sds_id,         /* I: SDS ID */
  Myhdf_attr_t *attr,   /* I: attribute data structure */
  char *string          /* I: array of 'char' values to be written to the HDF
                              file; converted to the specified data type */
)
{
  char8 val_char8[MYHDF_MAX_NATTR_VAL];
  int i;
  void *buf = NULL;

  if (attr->nval <= 0  ||  attr->nval > MYHDF_MAX_NATTR_VAL) 
    RETURN_ERROR("invalid number of values", "PutAttrString", false);

  if (attr->type != DFNT_CHAR8) 
    RETURN_ERROR("invalid type -- not string (char8)", "PutAttrString", false);

  if (sizeof(char8) == sizeof(char))
    buf = (void *)string;
  else {
    for (i = 0; i < attr->nval; i++) 
      val_char8[i] = (char8)string[i];
    buf = (void *)val_char8;
  }

  if (SDsetattr(sds_id, attr->name, attr->type, attr->nval, buf) == HDF_ERROR)
    RETURN_ERROR("setting attribute", "PutAttrString", false);

  return true;
}
