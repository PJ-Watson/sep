/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 *
 * This file is part of SEP
 *
 * All content except array comparison functions fqcmp() and fqmedian() is
 * distributed under an MIT license.
 *
 * Copyright 2014 SEP developers
 *
 * Array comparison functions fqcmp() and fqmedian() are distributed under an
 * LGPL license:
 *
 * Copyright 1993-2011 Emmanuel Bertin -- IAP/CNRS/UPMC
 * Copyright 2014 SEP developers
 *
 *%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sep.h"
#include "sepcore.h"

#define DETAILSIZE 512

#ifndef SEP_VERSION_STRING
#define SEP_VERSION_STRING "1.3.6"
#endif
const char * const sep_version_string = SEP_VERSION_STRING;
static _Thread_local char _errdetail_buffer[DETAILSIZE] = "";

/****************************************************************************/
/* data type conversion mechanics for runtime type conversion */

PIXTYPE convert_dbl(const void * ptr) {
  return *(const double *)ptr;
}

PIXTYPE convert_flt(const void * ptr) {
  return *(const float *)ptr;
}

PIXTYPE convert_int(const void * ptr) {
  return *(const int *)ptr;
}

PIXTYPE convert_byt(const void * ptr) {
  return *(const BYTE *)ptr;
}

/* return the correct converter depending on the datatype code */
int get_converter(int dtype, converter * f, int64_t * size) {
  int status = RETURN_OK;

  if (dtype == SEP_TFLOAT) {
    *f = convert_flt;
    *size = sizeof(float);
  } else if (dtype == SEP_TINT) {
    *f = convert_int;
    *size = sizeof(int);
  } else if (dtype == SEP_TDOUBLE) {
    *f = convert_dbl;
    *size = sizeof(double);
  } else if (dtype == SEP_TBYTE) {
    *f = convert_byt;
    *size = sizeof(BYTE);
  } else {
    *f = NULL;
    *size = 0;
    status = ILLEGAL_DTYPE;
  }
  return status;
}

/* array conversions */
void convert_array_flt(const void * ptr, int64_t n, PIXTYPE * target) {
  const float * source = ptr;
  int64_t i;
  for (i = 0; i < n; i++, source++) {
    target[i] = *source;
  }
}

void convert_array_dbl(const void * ptr, int64_t n, PIXTYPE * target) {
  const double * source = ptr;
  int64_t i;
  for (i = 0; i < n; i++, source++) {
    target[i] = *source;
  }
}

void convert_array_int(const void * ptr, int64_t n, PIXTYPE * target) {
  const int * source = ptr;
  int64_t i;
  for (i = 0; i < n; i++, source++) {
    target[i] = *source;
  }
}

void convert_array_byt(const void * ptr, int64_t n, PIXTYPE * target) {
  const BYTE * source = ptr;
  int64_t i;
  for (i = 0; i < n; i++, source++) {
    target[i] = *source;
  }
}

int get_array_converter(int dtype, array_converter * f, int64_t * size) {
  int status = RETURN_OK;

  if (dtype == SEP_TFLOAT) {
    *f = convert_array_flt;
    *size = sizeof(float);
  } else if (dtype == SEP_TBYTE) {
    *f = convert_array_byt;
    *size = sizeof(BYTE);
  } else if (dtype == SEP_TINT) {
    *f = convert_array_int;
    *size = sizeof(int);
  } else if (dtype == SEP_TDOUBLE) {
    *f = convert_array_dbl;
    *size = sizeof(double);
  } else {
    *f = NULL;
    *size = 0;
    status = ILLEGAL_DTYPE;
  }
  return status;
}


/****************************************************************************/
/* Copy a float array to various sorts of arrays */

void write_array_dbl(const float * ptr, int64_t n, void * target) {
  double * t = target;
  int64_t i;
  for (i = 0; i < n; i++, ptr++) {
    t[i] = (double)(*ptr);
  }
}

void write_array_int(const float * ptr, int64_t n, void * target) {
  int * t = target;
  int64_t i;
  for (i = 0; i < n; i++, ptr++) {
    t[i] = (int)(*ptr + 0.5);
  }
}

/* return the correct writer depending on the datatype code */
int get_array_writer(int dtype, array_writer * f, int64_t * size) {
  int status = RETURN_OK;

  if (dtype == SEP_TINT) {
    *f = write_array_int;
    *size = sizeof(int);
  } else if (dtype == SEP_TDOUBLE) {
    *f = write_array_dbl;
    *size = sizeof(double);
  } else {
    *f = NULL;
    *size = 0;
    status = ILLEGAL_DTYPE;
  }
  return status;
}

/* subtract a float array from arrays of various types */

void subtract_array_dbl(const float * ptr, int64_t n, void * target) {
  double * t = target;
  int64_t i;
  for (i = 0; i < n; i++, ptr++) {
    t[i] -= (double)(*ptr);
  }
}

void subtract_array_flt(const float * ptr, int64_t n, void * target) {
  float * t = target;
  int64_t i;
  for (i = 0; i < n; i++, ptr++) {
    t[i] -= *ptr;
  }
}

void subtract_array_int(const float * ptr, int64_t n, void * target) {
  int * t = target;
  int64_t i;
  for (i = 0; i < n; i++, ptr++) {
    t[i] -= (int)(*ptr + 0.5);
  }
}

/* return the correct subtractor depending on the datatype code */
int get_array_subtractor(int dtype, array_writer * f, int64_t * size) {
  int status = RETURN_OK;
  char errtext[80];

  if (dtype == SEP_TFLOAT) {
    *f = subtract_array_flt;
    *size = sizeof(float);
  } else if (dtype == SEP_TINT) {
    *f = subtract_array_int;
    *size = sizeof(int);
  } else if (dtype == SEP_TDOUBLE) {
    *f = subtract_array_dbl;
    *size = sizeof(double);
  } else {
    *f = NULL;
    *size = 0;
    status = ILLEGAL_DTYPE;
    sprintf(errtext, "in get_array_subtractor(): %d", dtype);
    put_errdetail(errtext);
  }
  return status;
}

/*****************************************************************************/
/* Error messaging */

void sep_get_errmsg(int status, char * errtext)
/* Return a short descriptive error message that corresponds to the input
 * error status value.  The message may be up to 60 characters long, plus
 * the terminating null character. */
{
  errtext[0] = '\0';
  switch (status) {
  case RETURN_OK:
    strcpy(errtext, "OK - no error");
    break;
  case MEMORY_ALLOC_ERROR:
    strcpy(errtext, "memory allocation");
    break;
  case PIXSTACK_FULL:
    strcpy(errtext, "internal pixel buffer full");
    break;
  case DEBLEND_OVERFLOW:
    strcpy(errtext, "object deblending overflow");
    break;
  case ILLEGAL_DTYPE:
    strcpy(errtext, "dtype not recognized/unsupported");
    break;
  case ILLEGAL_SUBPIX:
    strcpy(errtext, "subpix value must be nonnegative");
    break;
  case NON_ELLIPSE_PARAMS:
    strcpy(errtext, "parameters do not describe ellipse");
    break;
  case ILLEGAL_APER_PARAMS:
    strcpy(errtext, "invalid aperture parameters");
    break;
  case LINE_NOT_IN_BUF:
    strcpy(errtext, "array line out of buffer");
    break;
  case RELTHRESH_NO_NOISE:
    strcpy(errtext, "relative threshold but image has noise_type of NONE");
    break;
  case UNKNOWN_NOISE_TYPE:
    strcpy(errtext, "image has unknown noise_type");
    break;
  default:
    strcpy(errtext, "unknown error status");
    break;
  }
}

void sep_get_errdetail(char * errtext) {
  strcpy(errtext, _errdetail_buffer);
  memset(_errdetail_buffer, 0, DETAILSIZE);
}

void put_errdetail(const char * errtext) {
  strcpy(_errdetail_buffer, errtext);
}

/*****************************************************************************/
/* Array median */

static int fqcmp(const void * p1, const void * p2)
/* Sorting function for floats, used in fqmedian() below.
 * Return value is 1 if *p1>*p2, 0 if *p1==*p2, -1 otherwise */
{
  double f1 = *((const float *)p1);
  double f2 = *((const float *)p2);
  return f1 > f2 ? 1 : (f1 < f2 ? -1 : 0);
}

float fqmedian(float * ra, int64_t n)
/* Compute median of an array of floats.
 *
 * WARNING: input data are reordered! */
{
  qsort(ra, n, sizeof(float), fqcmp);
  if (n < 2) {
    return *ra;
  } else {
    return n & 1 ? ra[n / 2] : (ra[n / 2 - 1] + ra[n / 2]) / 2.0;
  }
}
