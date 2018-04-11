/*
 * Copyright 2011 IPOL Image Processing On Line http://www.ipol.im/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file tools.c
 * @brief Miscelaneous tools for the Canny filter
 *
 * @author Vincent Maioli <vincent.maioli@crans.org>
 */

#include <R.h> 

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "canny.h"

// this function prints an error message and aborts the program
void error(const char *fmt, ...)
{
  Rf_error(fmt);
  /*
	va_list argp;
	fprintf(stderr, "\nERROR: ");
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fprintf(stderr, "\n\n");
	fflush(NULL);
#ifdef NDEBUG
	exit(-1);
#else
	exit(*(int *)0x43);
#endif
   */
}


// this function always returns a valid pointer
void *xmalloc(size_t size)
{
	if (size == 0)
		error("xmalloc: zero size");
	void *new = malloc(size);
	if (!new)
	{
		double sm = size / (0x100000 * 1.0);
		error("xmalloc: out of memory when requesting "
			"%zu bytes (%gMB)",//:\"%s\"",
			size, sm);//, strerror(errno));
	}
	return new;
}


#define FORI(n) for(int i=0;i<(n);i++)
#define FORJ(n) for(int j=0;j<(n);j++)
#define FORL(n) for(int l=0;l<(n);l++)

#ifndef USE_WISDOM
void evoke_wisdom(void) {}
void bequeath_wisdom(void) {}
#else//USE_WISDOM
#include "fftwisdom.c"
#endif//USE_WISDOM

// wrapper around FFTW3 that computes the complex-valued Fourier transform
// of a real-valued image
static void fft_2ddouble(fftw_complex *fx, double *x, int w, int h)
{
	fftw_complex *a = fftw_malloc(w*h*sizeof*a);

	//fprintf(stderr, "planning...\n");
	evoke_wisdom();
	fftw_plan p = fftw_plan_dft_2d(h, w, a, fx,
						FFTW_FORWARD, FFTW_ESTIMATE);
	bequeath_wisdom();
	//fprintf(stderr, "...planned!\n");

	FORI(w*h) a[i] = x[i]; // complex assignment!
	fftw_execute(p);

	fftw_destroy_plan(p);
	fftw_free(a);
	fftw_cleanup();
}

// Wrapper around FFTW3 that computes the real-valued inverse Fourier transform
// of a complex-valued frequantial image.
// The input data must be hermitic.
static void ifft_2ddouble(double *ifx,  fftw_complex *fx, int w, int h)
{
	fftw_complex *a = fftw_malloc(w*h*sizeof*a);
	fftw_complex *b = fftw_malloc(w*h*sizeof*b);

	//fprintf(stderr, "planning...\n");
	evoke_wisdom();
	fftw_plan p = fftw_plan_dft_2d(h, w, a, b,
						FFTW_BACKWARD, FFTW_ESTIMATE);
	bequeath_wisdom();
	//fprintf(stderr, "...planned!\n");

	FORI(w*h) a[i] = fx[i];

	fftw_execute(p);
	double scale = 1.0/(w*h);
	FORI(w*h) {
		fftw_complex z = b[i] * scale;
		ifx[i] = crealf(z);	
		assert(cimagf(z) < 0.001);
	}
	fftw_destroy_plan(p);
	fftw_free(a);
	fftw_free(b);
	fftw_cleanup();
}

static void pointwise_complex_multiplication(fftw_complex *w,
		fftw_complex *z, fftw_complex *x, int n)
{
	FORI(n) {
		w[i] = z[i] * x[i];
	}
}

static void fill_2d_gaussian_image(double *gg, int w, int h, double inv_s)
{
	double (*g)[w] = (void *)gg;
	double alpha = inv_s*inv_s/(M_PI);

	FORJ(h) FORI(w) {
		double x = i < w/2 ? i : i - w;
		double y = j < h/2 ? j : j - h;
		double r = hypot(x, y);
		g[j][i] = alpha * exp(-r*r*inv_s*inv_s);
	}

	// if the kernel is too large, it escapes the domain, so the
	// normalization above must be corrected
	double m = 0;
	FORJ(h) FORI(w) m += g[j][i];
	FORJ(h) FORI(w) g[j][i] /= m;
}

// gaussian blur of a gray 2D image
static void gblur_gray(double *y, double *x, int w, int h, double s)
{
	s = 1/s;

	fftw_complex *fx = fftw_malloc(w*h*sizeof*fx);
	fft_2ddouble(fx, x, w, h);

	double *g = xmalloc(w*h*sizeof*g);
	fill_2d_gaussian_image(g, w, h, s);

	fftw_complex *fg = fftw_malloc(w*h*sizeof*fg);
	fft_2ddouble(fg, g, w, h);

	pointwise_complex_multiplication(fx, fx, fg, w*h);
	ifft_2ddouble(y, fx, w, h);

	fftw_free(fx);
	fftw_free(fg);
	free(g);
}

// gausian blur of a 2D image with pd-dimensional pixels
// (the blurring is performed independently for each co-ordinate)
void gblur(double *y, double *x, int w, int h, int pd, double s)
{
	double *c = xmalloc(w*h*sizeof*c);
	double *gc = xmalloc(w*h*sizeof*gc);
	FORL(pd) {
		FORI(w*h)
			c[i] = x[i*pd + l];
		gblur_gray(gc, c, w, h, s);
		FORI(w*h)
			y[i*pd + l] = gc[i];
	}
	free(c);
	free(gc);
}
