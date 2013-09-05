/*****************************************************************************
FILE: mystring.h
  
PURPOSE: Contains string-related constants, structures, and prototypes

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

#ifndef MYSTRING_H
#define MYSTRING_H

#include <stdio.h>

#define MAX_STR_LEN (510)
#define MAX_NUM_VALUE (20)

/* Key string type definition */
typedef struct {
  int key;
  char *string;
} Key_string_t;

/* Key type definition */
typedef struct {
  char *key;               /* Key string */
  size_t len_key;          /* Length of key */
  int nval;                /* Number of values */
  char *value[MAX_NUM_VALUE];  /* Value strings */
  size_t len_value[MAX_NUM_VALUE];  /* Length of value strings */
} Key_t;

char *DupString(char *string);
int GetLine(FILE *fp, char *s);
bool StringParse(char *s, Key_t *key);
#endif
