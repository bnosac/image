#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "adsf.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif

void error(const char *fmt, ...);
void *xmalloc(size_t size);
void gblur(double *y, double *x, int w, int h, int pd, double s);
