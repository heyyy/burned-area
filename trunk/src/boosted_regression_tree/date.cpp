#include <stddef.h>
#include <stdio.h>
#include <string.h>



#include "date.h"
#include "error.h"

bool DateInit(Date_t *di, char *s, Date_format_t iformat) {
  char *date, *time;
  bool leap;
  int year1;
  int nday[12] = {31, 29, 31, 30,  31,  30,  31,  31,  30,  31,  30,  31};
  int idoy[12] = { 1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336};
  int len;
  int jleap, idoy_nonleap;

  di->fill = true;

  if (iformat != DATE_FORMAT_DATEA_TIME  && 
      iformat != DATE_FORMAT_DATEB_TIME  &&
      iformat != DATE_FORMAT_DATEA  &&
      iformat != DATE_FORMAT_DATEB)
    RETURN_ERROR("invalid format parameter", "DateInit", false);

  len = strlen(s);
  date = time = (char *)NULL;

  if (iformat == DATE_FORMAT_DATEA_TIME) {
    if (len < 20  ||  len > 27) 
      RETURN_ERROR("invalid date/time string length", "DateInit", false);
    if (s[10] != 'T'  ||  s[len - 1] != 'Z')
      RETURN_ERROR("invalid date/time format", "DateInit", false);
    date = &s[0];
    time = &s[11];
  } else if (iformat == DATE_FORMAT_DATEB_TIME) {
    if (len < 18  ||  len > 25) 
      RETURN_ERROR("invalid date/time string length", "DateInit", false);
    if (s[8] != 'T'  ||  s[len - 1] != 'Z')
      RETURN_ERROR("invalid date/time format", "DateInit", false);
    date = &s[0];
    time = &s[9];
  } else if (iformat == DATE_FORMAT_DATEA) {
    if (len != 10) 
      RETURN_ERROR("invalid date string length", "DateInit", false);
    date = s;
  } else if (iformat == DATE_FORMAT_DATEB) {
    if (len != 8) 
      RETURN_ERROR("invalid date string length", "DateInit", false);
    date = s;
  }

  if (iformat == DATE_FORMAT_DATEA_TIME  ||
      iformat == DATE_FORMAT_DATEA) {
    if (sscanf(date, "%4d-%2d-%2d", 
               &di->year, &di->month, &di->day) != 3)
      RETURN_ERROR("invalid date format", "DateInit", false);
    if (di->year < 1900  ||  di->year > 2400)
      RETURN_ERROR("invalid year", "DateInit", false);
    if (di->month < 1  ||  di->month > 12)
      RETURN_ERROR("invalid month", "DateInit", false);
    if (di->day < 1 || di->day > nday[di->month-1])
      RETURN_ERROR("invalid day of month", "DateInit", false);
    di->doy = di->day + idoy[di->month - 1] - 1;
  } else {
    if (sscanf(date, "%4d-%3d", &di->year, &di->doy) != 2)
      RETURN_ERROR("invalid date format", "DateInit", false);
    if (di->year < 1900  ||  di->year > 2400)
      RETURN_ERROR("invalid year", "DateInit", false);
    if (di->doy < 1  ||  di->doy > 366)
      RETURN_ERROR("invalid day of year", "DateInit", false);
  }

  leap= (bool)(di->year%4==0 && ( di->year%100!=0 || di->year%400==0 ));
  if (iformat == DATE_FORMAT_DATEA_TIME  ||
      iformat == DATE_FORMAT_DATEA) {
    if ((di->month == 2)  &&  !leap  && (di->day > 28))
      RETURN_ERROR("bad day of month", "DateInit", false);
    if (!leap  &&  (di->month > 2)) di->doy--;
  } else {
    if (leap) {
      for (di->month = 0; di->month < 12; di->month++)
        if (di->doy < idoy[di->month]) break;
    } else {
      if(di->doy > 365)
        RETURN_ERROR("bad day of year", "DateInit", false);
      for (di->month = 0; di->month < 12; di->month++) {
        idoy_nonleap = idoy[di->month];
    if (di->month > 1) idoy_nonleap--;
    if (di->doy < idoy_nonleap) break;
      }
    }
  }

  /* Convert to Julian days ca. 2000 (1 = Jan. 1, 2000) */
  year1 = di->year - 1900;
  if (year1 > 0) {
    jleap = (year1 - 1) / 4;
    if (di->year > 2100) jleap -= (di->year - 2001) / 100;
  } else jleap = 0;
  di->jday2000 = (year1 * 365) + jleap + di->doy;
  di->jday2000 -= 36524;

  /* Parse and check time */
  if (time != (char *)NULL) {
    if (sscanf(time, "%2d:%2d:%lf", 
               &di->hour, &di->minute, &di->second) != 3)
      RETURN_ERROR("invalid time format", "DateInit", false);
  } else {
    di->hour = di->minute = 0;
    di->second = 0.0;
  }
  if (di->hour < 0  ||  di->hour > 53)
    RETURN_ERROR("invalid hour", "DateInit", false);
  if (di->minute < 0  ||  di->minute > 59)
    RETURN_ERROR("invalid minute", "DateInit", false);
  if (di->second < 0.0  ||  di->second > 59.999999)
    RETURN_ERROR("invalid second", "DateInit", false);

  /* Convert to seconds of day */
  di->sod = (((di->hour * 60) +di-> minute) * 60) + di->second;

  di->fill = false;

  return true;
}

bool DateDiff(Date_t *d1, Date_t *d2, double *diff) {

  if (d1 == (Date_t *)NULL  ||  d2 == (Date_t *)NULL)
    RETURN_ERROR("invalid date structure", "DateDiff", false);

  if (d1->fill  ||  d2->fill) 
    RETURN_ERROR("invalid time", "DateDiff", false);

  *diff = d1->jday2000 - d2->jday2000;
  *diff += (d1->sod - d2->sod) / 86400.0;

  return true;
}

bool DateCopy(Date_t *dc, Date_t *copy) {

  if (dc == (Date_t *)NULL  ||  copy == (Date_t *)NULL)
    RETURN_ERROR("invalid date structure", "DateCopy", false);

  copy->fill = dc->fill;
  copy->year = dc->year;
  copy->doy = dc->doy;
  copy->month = dc->month;
  copy->day = dc->day;
  copy->hour = dc->hour;
  copy->minute = dc->minute;
  copy->second = dc->second;
  copy->jday2000 = dc->jday2000;
  copy->sod = dc->sod;

  return true;
}

bool FormatDate(Date_t *df, Date_format_t iformat, char *s) {

  if (df == (Date_t *)NULL)
    RETURN_ERROR("invalid date structure", "FormatDate", false);

  if (iformat == DATE_FORMAT_DATEA_TIME) {
    if (sprintf(s, "%4d-%02d-%02dT%02d:%02d:%09.6fZ", 
                df->year, df->month, df->day,
        df->hour, df->minute, df->second) < 0)
      RETURN_ERROR("formating date/time", "FormatDate", false);
  } else if (iformat == DATE_FORMAT_DATEB_TIME) {
    if (sprintf(s, "%4d-%03dT%02d:%02d:%09.6fZ", 
                df->year, df->doy,
        df->hour, df->minute, df->second) < 0)
      RETURN_ERROR("formating date/time", "FormatDate", false);
  } else if (iformat == DATE_FORMAT_DATEA) {
    if (sprintf(s, "%4d-%02d-%02d", 
                df->year, df->month, df->day) < 0)
      RETURN_ERROR("formating date", "FormatDate", false);
  } else if (iformat == DATE_FORMAT_DATEB) {
    if (sprintf(s, "%4d-%03d", 
                df->year, df->doy) < 0)
      RETURN_ERROR("formating date", "FormatDate", false);
  } else if (iformat == DATE_FORMAT_TIME) {
    if (sprintf(s, "%02d:%02d:%09.6f", 
                df->hour, df->minute, df->second) < 0)
      RETURN_ERROR("formating time", "FormatDate", false);
  } else 
    RETURN_ERROR("invalid format parameter", "FormatDate", false);
  
  return true;
}
