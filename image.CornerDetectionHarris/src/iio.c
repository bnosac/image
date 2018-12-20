// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright 2012 Enric Meinhardt-Llopis <enric.meinhardt@cmla.ens-cachan.fr>
// All rights reserved.




// IIO: a library for reading small images {{{1
//
// Goal: load an image (of unknown format) from a given FILE*
//
// Technique: read the first few bytes of the file to identify the format, and
// call the appropriate image library (lipng, lipjpeg, libtiff, etc).  For
// simple, uncompressed formats, write the loading code by hand.  For formats
// having a library with a nice API, call the library functions.  For other or
// unrecognized formats use an external program (convert, anytopnm,
// gm convert...) to convert them into a readable format.  If anything else
// fails, assume that the image is in headings+raw format, and try to extract
// its dimensions and headings using some heuristics (file name containing
// "%dx%d", headings containing ascii numbers, etc.)
//
// Difficulties: most image libraries expect to be fed a whole file, not a
// beheaded file.  Thus, some hackery is necessary.
//
//
// Copyright 2012 Enric Meinhardt-Llopis <enric.meinhardt@cmla.ens-cachan.fr>
//



#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "iio.h"                // only for IIO_MAX_DIMENSION



#ifdef I_CAN_HAS_LIBPNG
// ugly "feature" in png.h forces this header to be included first
#include <png.h>
#endif



// #defines {{{1

//
// configuration
//

#if _POSIX_C_SOURCE >= 200809L
#define I_CAN_HAS_FMEMOPEN 1
#endif

#if _POSIX_C_SOURCE >= 200112L
#define I_CAN_HAS_MKSTEMP 1
#endif

//
// enum-like, only used internally
//

#define IIO_TYPE_INT8 1
#define IIO_TYPE_UINT8 2
#define IIO_TYPE_INT16 3
#define IIO_TYPE_UINT16 4
#define IIO_TYPE_INT32 5
#define IIO_TYPE_UINT32 6
#define IIO_TYPE_FLOAT 7
#define IIO_TYPE_DOUBLE 8
#define IIO_TYPE_LONGDOUBLE 9
#define IIO_TYPE_INT64 10
#define IIO_TYPE_UINT64 11
#define IIO_TYPE_HALF 12
#define IIO_TYPE_UINT1 13
#define IIO_TYPE_UINT2 14
#define IIO_TYPE_UINT4 15
#define IIO_TYPE_CHAR 16
#define IIO_TYPE_SHORT 17
#define IIO_TYPE_INT 18
#define IIO_TYPE_LONG 19
#define IIO_TYPE_LONGLONG 20

#define IIO_FORMAT_WHATEVER 0
#define IIO_FORMAT_QNM 1
#define IIO_FORMAT_PNG 2
#define IIO_FORMAT_JPEG 3
#define IIO_FORMAT_TIFF 4
#define IIO_FORMAT_RIM 5
#define IIO_FORMAT_BMP 6
#define IIO_FORMAT_EXR 7
#define IIO_FORMAT_JP2 8
#define IIO_FORMAT_VTK 9
#define IIO_FORMAT_CIMG 10
#define IIO_FORMAT_PAU 11
#define IIO_FORMAT_DICOM 12
#define IIO_FORMAT_PFM 13
#define IIO_FORMAT_NIFTI 14
#define IIO_FORMAT_PCX 15
#define IIO_FORMAT_GIF 16
#define IIO_FORMAT_XPM 17
#define IIO_FORMAT_RAFA 18
#define IIO_FORMAT_FLO 19
#define IIO_FORMAT_JUV 20
#define IIO_FORMAT_LUM 21
#define IIO_FORMAT_PCM 22
#define IIO_FORMAT_UNRECOGNIZED (-1)

//
// sugar
//
#define FORI(n) for(int i=0;i<(int)(n);i++)
#define FORJ(n) for(int j=0;j<(int)(n);j++)
#define FORK(n) for(int k=0;k<(int)(n);k++)
#define FORL(n) for(int l=0;l<(int)(n);l++)

#ifdef IIO_SHOW_DEBUG_MESSAGES
#define IIO_DEBUG(...) do {\
        fprintf(stderr,"DEBUG(%s:%d:%s): ",__FILE__,__LINE__,__PRETTY_FUNCTION__);\
        fprintf(stderr,__VA_ARGS__);} while(0)
#else //IIO_SHOW_DEBUG_MESSAGES
#define IIO_DEBUG(...) do { ; } while(0)        /* no-res */
#endif //IIO_SHOW_DEBUG_MESSAGES

//
// hacks
//
#ifndef __attribute__
#ifndef __GNUC__
#define __attribute__(x) /*NOTHING*/
#endif
#endif
// typedefs {{{1
typedef long long longong;
typedef long double longdouble;




// utility functions {{{1

static bool
checkbounds (int a, int x, int b)
{
  return a <= x && x < b;
}

#ifndef IIO_ABORT_ON_ERROR

// NOTE: libpng has a nasty "feature" whereby you have to include libpng.h
// before setjmp.h if you want to use both.   This induces the following
// hackery:
#ifndef I_CAN_HAS_LIBPNG
#include <setjmp.h>
#endif //I_CAN_HAS_LIBPNG
static jmp_buf global_jump_buffer;
#endif //IIO_ABORT_ON_ERROR

//#include <errno.h> // only for errno
#include <ctype.h>              // for isspace
#include <math.h>               // for floorf
#include <stdlib.h>

#ifdef I_CAN_HAS_LINUX
#include <unistd.h>
static const char *emptystring = "";
static const char *
myname (void)
{
#define n 0x29a
  static char buf[n];
  pid_t p = getpid ();
  snprintf (buf, n, "/proc/%d/cmdline", p);
  FILE *f = fopen (buf, "r");
  if (!f)
    return emptystring;
  int c, i = 0;
  while ((c = fgetc (f)) != EOF && i < n)
    {
#undef n
      buf[i] = c ? c : ' ';
      i += 1;
    }
  if (i)
    buf[i - 1] = '\0';
  fclose (f);
  return buf;
}
#else
static const char *
myname (void)
{
  return "";
}
#endif //I_CAN_HAS_LINUX

//static int iio_single_jmpstuff(bool setup, bool do_return)
//{
//      if (setup) {
//}

static void error (const char *fmt, ...)
  __attribute__ ((noreturn, format (printf, 1, 2)));
static void
error (const char *fmt, ...)
{
  va_list argp;
  fprintf (stderr, "\nERROR(\"%s\"): ", myname ());
  va_start (argp, fmt);
  vfprintf (stderr, fmt, argp);
  va_end (argp);
  fprintf (stderr, "\n\n");
  fflush (NULL);
//      if (global_hack_to_never_fail)
//      {
//              IIO_DEBUG("now wave a dead chicken and press enter\n");
//              getchar();
//              return;
//      }
//      exit(43);
#ifndef IIO_ABORT_ON_ERROR
  longjmp (global_jump_buffer, 1);
  //iio_single_jmpstuff(true, false);
#else //IIO_ABORT_ON_ERROR
#ifdef NDEBUG
  exit (-1);
#else //NDEBUG
  //print_trace(stderr);
  exit (*(int *) 0x43);
#endif //NDEBUG
#endif //IIO_ABORT_ON_ERROR
}

static void *
xmalloc (size_t size)
{
  if (size == 0)
    error ("xmalloc: zero size");
  void *new = malloc (size);
  if (!new)
    {
      double sm = size / (0x100000 * 1.0);
      error ("xmalloc: out of memory when requesting " "%zu bytes (%gMB)",      //:\"%s\"",
             size, sm);         //, strerror(errno));
    }
  return new;
}

static void *
xrealloc (void *p, size_t s)
{
  void *r = realloc (p, s);
  if (!r)
    error ("realloc failed");
  return r;
}

static void
xfree (void *p)
{
  if (!p)
    error ("thou shalt not free a null pointer!");
  free (p);
}

static const
  char *global_variable_containing_the_name_of_the_last_opened_file = NULL;

static FILE *
xfopen (const char *s, const char *p)
{
  global_variable_containing_the_name_of_the_last_opened_file = NULL;
  FILE *f;

  if (!s)
    error ("trying to open a file with NULL name");

  if (0 == strcmp ("-", s))
    {
      if (0 == strcmp ("w", p))
        return stdout;
      else if (0 == strcmp ("r", p))
        return stdin;
      else
        error ("unknown fopen mode \"%s\"", p);
    }
  if (0 == strcmp ("--", s) && 0 == strcmp ("w", p))
    return stderr;

  f = fopen (s, p);
  if (f == NULL)
    error ("can not open file \"%s\" in mode \"%s\"",   // (%s)",
           s, p);               //, strerror(errno));
  //global_variable_containing_the_name_of_the_last_opened_file = (char*)s;
  global_variable_containing_the_name_of_the_last_opened_file = s;
  return f;
}

static void
xfclose (FILE * f)
{
  global_variable_containing_the_name_of_the_last_opened_file = NULL;
  if (f != stdout && f != stdin && f != stderr)
    {
      int r = fclose (f);
      if (r)
        error ("fclose error"); // \"%s\"", strerror(errno));
    }
}

static int
pilla_caracter_segur (FILE * f)
{
  int c = getc (f);
  if (EOF == c)
    error ("input file ended before expected");
  //IIO_DEBUG("pcs = '%c'\n", c);
  return c;
}

static void
menja_espais (FILE * f)
{
  //IIO_DEBUG("inside menja espais\n");
  int c;
  do
    c = pilla_caracter_segur (f);
  while (isspace (c));
  ungetc (c, f);
}

static void
menja_linia (FILE * f)
{
  //IIO_DEBUG("inside menja linia\n");
  while (pilla_caracter_segur (f) != '\n')
    ;
}

static void
menja_espais_i_comentaris (FILE * f)
{
  //IIO_DEBUG("inside menja espais i comentaris\n");
  int c, comment_char = '#';
  menja_espais (f);
  while (1)
    {
      c = pilla_caracter_segur (f);
      if (c == comment_char)
        {
          menja_linia (f);
          menja_espais (f);
        }
      else
        ungetc (c, f);
      if (c == comment_char)
        break;
    }
}



// struct iio_image { ... }; {{{1

// This struct is used for exchanging image information between internal
// functions.  It could be safely eliminated, and this information be passed as
// five or six variables.
struct iio_image
{
  int dimension;                // 1, 2, 3 or 4
  int sizes[IIO_MAX_DIMENSION];
  int pixel_dimension;
  int type;                     // IIO_TYPE_*

  int meta;                     // IIO_META_*
  int format;                   // IIO_FORMAT_*

  bool contiguous_data;
  bool caca[3];
  void *data;
};


// struct iio_image management {{{1

// TODO: reduce the silly number of constructors to 1

static void
iio_image_assert_struct_consistency (struct iio_image *x)
{
  assert (x->dimension > 0);
  assert (x->dimension <= IIO_MAX_DIMENSION);
  FORI (x->dimension) assert (x->sizes[i] > 0);
  assert (x->pixel_dimension > 0);
  switch (x->type)
    {
    case IIO_TYPE_INT8:
    case IIO_TYPE_UINT8:
    case IIO_TYPE_INT16:
    case IIO_TYPE_UINT16:
    case IIO_TYPE_INT32:
    case IIO_TYPE_UINT32:
    case IIO_TYPE_FLOAT:
    case IIO_TYPE_DOUBLE:
    case IIO_TYPE_LONGDOUBLE:
    case IIO_TYPE_INT64:
    case IIO_TYPE_UINT64:
    case IIO_TYPE_HALF:
    case IIO_TYPE_CHAR:
    case IIO_TYPE_SHORT:
    case IIO_TYPE_INT:
    case IIO_TYPE_LONG:
    case IIO_TYPE_LONGLONG:
      break;
    default:
      assert (false);
    }
  //if (x->contiguous_data)
  //      assert(x->data == (void*)(x+1));
}

// API
static size_t
iio_type_size (int type)
{
  switch (type)
    {
    case IIO_TYPE_INT8:
      return 1;
    case IIO_TYPE_UINT8:
      return 1;
    case IIO_TYPE_INT16:
      return 2;
    case IIO_TYPE_UINT16:
      return 2;
    case IIO_TYPE_INT32:
      return 4;
    case IIO_TYPE_UINT32:
      return 4;
    case IIO_TYPE_INT64:
      return 8;
    case IIO_TYPE_UINT64:
      return 8;
    case IIO_TYPE_CHAR:
      return sizeof (char);     // 1
    case IIO_TYPE_SHORT:
      return sizeof (short);
    case IIO_TYPE_INT:
      return sizeof (int);
    case IIO_TYPE_LONG:
      return sizeof (long);
    case IIO_TYPE_LONGLONG:
      return sizeof (long long);
    case IIO_TYPE_FLOAT:
      return sizeof (float);
    case IIO_TYPE_DOUBLE:
      return sizeof (double);
    case IIO_TYPE_LONGDOUBLE:
      return sizeof (long double);
    case IIO_TYPE_HALF:
      return sizeof (float) / 2;
    default:
      error ("unrecognized type %d", type);
    }
}

// XXX TODO FIXME: this is architecture dependent!
// this function actually requires A LOT of magic to be portable
static int
normalize_type (int type_in)
{
  int type_out;
  switch (type_in)
    {
    case IIO_TYPE_CHAR:
      type_out = IIO_TYPE_UINT8;
      break;
    case IIO_TYPE_SHORT:
      type_out = IIO_TYPE_INT16;
      break;
    case IIO_TYPE_INT:
      type_out = IIO_TYPE_UINT32;
      break;
    default:
      type_out = type_in;
      break;
    }
  if (type_out != type_in)
    {
      // the following assertion fails on many architectures
      assert (iio_type_size (type_in) == iio_type_size (type_out));
    }
  return type_out;
}


// internal API
static int
iio_type_id (size_t sample_size, bool ieeefp_sample, bool signed_sample)
{
  if (ieeefp_sample)
    {
      if (signed_sample)
        error ("signed floats are a no-no!");
      switch (sample_size)
        {
        case sizeof (float):
          return IIO_TYPE_FLOAT;
        case sizeof (double):
          return IIO_TYPE_DOUBLE;
        case sizeof (long double):
          return IIO_TYPE_LONGDOUBLE;
        case sizeof (float) / 2:
          return IIO_TYPE_HALF;
        default:
          error ("bad float size %zu", sample_size);
        }
    }
  else
    {
      switch (sample_size)
        {
        case 1:
          return signed_sample ? IIO_TYPE_INT8 : IIO_TYPE_UINT8;
        case 2:
          return signed_sample ? IIO_TYPE_INT16 : IIO_TYPE_UINT16;
        case 4:
          return signed_sample ? IIO_TYPE_INT32 : IIO_TYPE_UINT32;
        case 8:
          return signed_sample ? IIO_TYPE_INT64 : IIO_TYPE_UINT64;
        default:
          error ("bad integral size %zu", sample_size);
        }
    }
}

// internal API
static void
iio_type_unid (int *size, bool * ieefp, bool * signedness, int type)
{
  int t = normalize_type (type);
  *size = iio_type_size (t);
  *ieefp = (t == IIO_TYPE_HALF || t == IIO_TYPE_FLOAT
            || t == IIO_TYPE_DOUBLE || t == IIO_TYPE_LONGDOUBLE);
  *signedness = (t == IIO_TYPE_INT8 || t == IIO_TYPE_INT16
                 || t == IIO_TYPE_INT32 || t == IIO_TYPE_INT64);
}

// internal API
static int
iio_image_number_of_elements (struct iio_image *x)
{
  iio_image_assert_struct_consistency (x);
  int r = 1;
  FORI (x->dimension) r *= x->sizes[i];
  return r;
}

// internal API
static int
iio_image_number_of_samples (struct iio_image *x)
{
  return iio_image_number_of_elements (x) * x->pixel_dimension;
}

// internal API
static int
iio_image_data_size (struct iio_image *x)
{
  return iio_type_size (x->type) * iio_image_number_of_samples (x);
}

// internal API
static bool
iio_image_sample_integerP (struct iio_image *x)
{
  if (false
      || x->type == IIO_TYPE_HALF
      || x->type == IIO_TYPE_FLOAT
      || x->type == IIO_TYPE_DOUBLE || x->type == IIO_TYPE_LONGDOUBLE)
    return false;
  else
    return true;
}

// internal API
static size_t
iio_image_sample_size (struct iio_image *x)
{
  return iio_type_size (x->type);
}

static const char *
iio_strtyp (int type)
{
#define M(t) case IIO_TYPE_ ## t: return #t
  switch (type)
    {
      M (INT8);
      M (UINT8);
      M (INT16);
      M (UINT16);
      M (INT32);
      M (UINT32);
      M (INT64);
      M (UINT64);
      M (FLOAT);
      M (DOUBLE);
      M (LONGDOUBLE);
      M (HALF);
      M (UINT1);
      M (UINT2);
      M (UINT4);
      M (CHAR);
      M (SHORT);
      M (INT);
      M (LONG);
      M (LONGLONG);
    default:
      return "unrecognized";
    }
#undef M
}

static const char *
iio_strfmt (int format)
{
#define M(f) case IIO_FORMAT_ ## f: return #f
  switch (format)
    {
      M (WHATEVER);
      M (QNM);
      M (PNG);
      M (JPEG);
      M (TIFF);
      M (RIM);
      M (BMP);
      M (EXR);
      M (JP2);
      M (VTK);
      M (CIMG);
      M (PAU);
      M (DICOM);
      M (PFM);
      M (NIFTI);
      M (PCX);
      M (GIF);
      M (XPM);
      M (RAFA);
      M (UNRECOGNIZED);
    default:
      error ("caca de la grossa");
    }
#undef M
}

static void
iio_print_image_info (FILE * f, struct iio_image *x)
{
  fprintf (f, "iio_print_image_info %p\n", (void *) x);
  fprintf (f, "dimension = %d\n", x->dimension);
  int *s = x->sizes;
  switch (x->dimension)
    {
    case 1:
      fprintf (f, "size = %d\n", s[0]);
      break;
    case 2:
      fprintf (f, "sizes = %dx%d\n", s[0], s[1]);
      break;
    case 3:
      fprintf (f, "sizes = %dx%dx%d\n", s[0], s[1], s[2]);
      break;
    case 4:
      fprintf (f, "sizes = %dx%dx%dx%d\n", s[0], s[1], s[2], s[3]);
      break;
    default:
      error ("unsupported dimension %d", x->dimension);
    }
  fprintf (f, "pixel_dimension = %d\n", x->pixel_dimension);
  fprintf (f, "type = %s\n", iio_strtyp (x->type));
  //fprintf(f, "format = %d\n", x->format);
  //fprintf(f, "meta = %d\n", x->meta);
  //fprintf(f, "contiguous_data = %d\n", x->contiguous_data);
  fprintf (f, "data = %p\n", (void *) x->data);
}


//static void fill_canonic_type_descriptors(struct iio_image *x, int type)
//{
//      bool s; int i; int f;
//      switch(type) {
//      case IIO_TYPE_INT8:       s=true; i=1; f=0; break;
//      case IIO_TYPE_INT16:      s=true; i=2; f=0; break;
//      case IIO_TYPE_INT32:      s=true; i=4; f=0; break;
//      case IIO_TYPE_INT64:      s=true; i=4; f=0; break;
//      case IIO_TYPE_UINT8:      s=false; i=1; f=0; break;
//      case IIO_TYPE_UINT16:     s=false; i=2; f=0; break;
//      case IIO_TYPE_UINT32:     s=false; i=4; f=0; break;
//      case IIO_TYPE_UINT64:     s=false; i=8; f=0; break;
//      case IIO_TYPE_FLOAT:      s=false; i=0; f=4; break;
//      case IIO_TYPE_DOUBLE:     s=false; i=0; f=8; break;
//      case IIO_TYPE_HALF:       s=false; i=0; f=2; break;
//      default: error("unrecognized type %d", type);
//      }
//      x->signedness = s;
//      x->integer_size = i;
//      x->float_size = f;
//}


//static size_t typesize(int type)
//{
//      switch(type) {
//      case IIO_TYPE_CHAR: return sizeof(char);
//      case IIO_TYPE_SHORT: return sizeof(short);
//      case IIO_TYPE_INT: return sizeof(int);
//      case IIO_TYPE_LONG: return sizeof(long);
//      case IIO_TYPE_FLOAT: return sizeof(float);
//      case IIO_TYPE_DOUBLE: return sizeof(double);
//      default: return -1;
//      }
//}

static void
fill_struct_2d (struct iio_image *x, int w, int h, int pd, int type,
                void *data)
{
  x->dimension = 2;
  x->type = type;
  x->pixel_dimension = pd;
  x->data = data;
  x->sizes[0] = w;
  x->sizes[1] = h;
}

static void
iio_image_fill (struct iio_image *x,
                int dimension, int *sizes, int type, int pixel_dimension)
{
  x->dimension = dimension;
  FORI (dimension) x->sizes[i] = sizes[i];
  x->type = type;
  x->pixel_dimension = pixel_dimension;
  x->data = NULL;
  x->format = -1;
  x->meta = -1;
  x->contiguous_data = false;
}


static void
iio_wrap_image_struct_around_data (struct iio_image *x,
                                   int dimension, int *sizes,
                                   int pixel_dimension, int type, void *data)
{
  assert (dimension < IIO_MAX_DIMENSION);
  x->dimension = dimension;
  FORI (x->dimension) x->sizes[i] = sizes[i];
  x->pixel_dimension = pixel_dimension;
  x->type = type;
  x->data = data;
  x->contiguous_data = false;
  x->meta = -42;
  x->format = -42;
}


static void
iio_image_build_independent (struct iio_image *x,
                             int dimension, int *sizes, int type,
                             int pixel_dimension)
{
  assert (dimension > 0);
  assert (dimension <= 4);
  assert (pixel_dimension > 0);
  iio_image_fill (x, dimension, sizes, type, pixel_dimension);
  size_t datalength = 1;
  FORI (dimension) datalength *= sizes[i];
  size_t datasize = datalength * iio_type_size (type) * pixel_dimension;
  x->data = xmalloc (datasize);
}

// low-level API: constructor
static struct iio_image *
iio_image_build_contiguous (int dimension, int *sizes,
                            int type, int pixel_dimension, void *data)
{
  assert (!data);
  assert (dimension > 0);
  assert (dimension <= 4);
  assert (pixel_dimension > 0);
  size_t datalength = 1;
  FORI (dimension) datalength *= sizes[i];
  size_t datasize = datalength * iio_type_size (type) * pixel_dimension;
  struct iio_image *r = xmalloc (datasize + sizeof *r);
  iio_image_fill (r, dimension, sizes, type, pixel_dimension);
  r->data = sizeof *r + (char *) r;
  r->contiguous_data = true;
  return r;
}

// low-level API: queries
static int
iio_image_get_dimension (struct iio_image *x)
{
  return x->dimension;
}

static int *
iio_image_get_sizes (struct iio_image *x)
{
  return x->sizes;
}

static int
iio_image_get_type (struct iio_image *x)
{
  return x->type;
}

static int
iio_image_get_pixel_dimension (struct iio_image *x)
{
  return x->pixel_dimension;
}

static int
iio_image_get_meta (struct iio_image *x)
{
  return x->meta;
}

//int iio_image_get_layout(struct iio_image *x) { return x->layout; }
static int
iio_image_get_format (struct iio_image *x)
{
  return x->format;
}

static void *
iio_image_get_data (struct iio_image *x)
{
  return x->data;
}






// data conversion {{{1

#define T0(x) ((x)>0?(x):0)
#define T8(x) ((x)>0?((x)<0xff?(x):0xff):0)
#define T6(x) ((x)>0?((x)<0xffff?(x):0xffff):0)

#define CC(a,b) (77*(a)+(b))    // hash number of a conversion pair
#define I8 IIO_TYPE_INT8
#define U8 IIO_TYPE_UINT8
#define I6 IIO_TYPE_INT16
#define U6 IIO_TYPE_UINT16
#define I2 IIO_TYPE_INT32
#define U2 IIO_TYPE_UINT32
#define I4 IIO_TYPE_INT64
#define U4 IIO_TYPE_UINT64
#define F4 IIO_TYPE_FLOAT
#define F8 IIO_TYPE_DOUBLE
#define F6 IIO_TYPE_LONGDOUBLE
static void
convert_datum (void *dest, void *src, int dest_fmt, int src_fmt)
{
  // NOTE: verify that this switch is optimized outside of the loop in
  // which it is contained.  Otherwise, produce some self-modifying code
  // here.
  switch (CC (dest_fmt, src_fmt))
    {

      // change of sign (lossless, but changes interpretation)
    case CC (I8, U8):
      *(int8_t *) dest = *(uint8_t *) src;
      break;
    case CC (I6, U6):
      *(int16_t *) dest = *(uint16_t *) src;
      break;
    case CC (I2, U2):
      *(int32_t *) dest = *(uint32_t *) src;
      break;
    case CC (U8, I8):
      *(uint8_t *) dest = *(int8_t *) src;
      break;
    case CC (U6, I6):
      *(uint16_t *) dest = *(int16_t *) src;
      break;
    case CC (U2, I2):
      *(uint32_t *) dest = *(int32_t *) src;
      break;

      // different size signed integers (3 lossy, 3 lossless)
    case CC (I8, I6):
      *(int8_t *) dest = *(int16_t *) src;
      break;                    //iw810
    case CC (I8, I2):
      *(int8_t *) dest = *(int32_t *) src;
      break;                    //iw810
    case CC (I6, I2):
      *(int16_t *) dest = *(int32_t *) src;
      break;                    //iw810
    case CC (I6, I8):
      *(int16_t *) dest = *(int8_t *) src;
      break;
    case CC (I2, I8):
      *(int32_t *) dest = *(int8_t *) src;
      break;
    case CC (I2, I6):
      *(int32_t *) dest = *(int16_t *) src;
      break;

      // different size unsigned integers (3 lossy, 3 lossless)
    case CC (U8, U6):
      *(uint8_t *) dest = *(uint16_t *) src;
      break;                    //iw810
    case CC (U8, U2):
      *(uint8_t *) dest = *(uint32_t *) src;
      break;                    //iw810
    case CC (U6, U2):
      *(uint16_t *) dest = *(uint32_t *) src;
      break;                    //iw810
    case CC (U6, U8):
      *(uint16_t *) dest = *(uint8_t *) src;
      break;
    case CC (U2, U8):
      *(uint32_t *) dest = *(uint8_t *) src;
      break;
    case CC (U2, U6):
      *(uint32_t *) dest = *(uint16_t *) src;
      break;

      // diferent size mixed integers, make signed (3 lossy, 3 lossless)
    case CC (I8, U6):
      *(int8_t *) dest = *(uint16_t *) src;
      break;                    //iw810
    case CC (I8, U2):
      *(int8_t *) dest = *(uint32_t *) src;
      break;                    //iw810
    case CC (I6, U2):
      *(int16_t *) dest = *(uint32_t *) src;
      break;                    //iw810
    case CC (I6, U8):
      *(int16_t *) dest = *(uint8_t *) src;
      break;
    case CC (I2, U8):
      *(int32_t *) dest = *(uint8_t *) src;
      break;
    case CC (I2, U6):
      *(int32_t *) dest = *(uint16_t *) src;
      break;

      // diferent size mixed integers, make unsigned (3 lossy, 3 lossless)
    case CC (U8, I6):
      *(uint8_t *) dest = *(int16_t *) src;
      break;                    //iw810
    case CC (U8, I2):
      *(uint8_t *) dest = *(int32_t *) src;
      break;                    //iw810
    case CC (U6, I2):
      *(uint16_t *) dest = *(int32_t *) src;
      break;                    //iw810
    case CC (U6, I8):
      *(uint16_t *) dest = *(int8_t *) src;
      break;
    case CC (U2, I8):
      *(uint32_t *) dest = *(int8_t *) src;
      break;
    case CC (U2, I6):
      *(uint32_t *) dest = *(int16_t *) src;
      break;

      // from float (typically lossy, except for small integers)
    case CC (I8, F4):
      *(int8_t *) dest = *(float *) src;
      break;                    //iw810
    case CC (I6, F4):
      *(int16_t *) dest = *(float *) src;
      break;                    //iw810
    case CC (I2, F4):
      *(int32_t *) dest = *(float *) src;
      break;
    case CC (I8, F8):
      *(int8_t *) dest = *(double *) src;
      break;                    //iw810
    case CC (I6, F8):
      *(int16_t *) dest = *(double *) src;
      break;                    //iw810
    case CC (I2, F8):
      *(int32_t *) dest = *(double *) src;
      break;                    //iw810
    case CC (U8, F4):
      *(uint8_t *) dest = T8 (0.5 + *(float *) src);
      break;                    //iw810
    case CC (U6, F4):
      *(uint16_t *) dest = T6 (0.5 + *(float *) src);
      break;                    //iw810
    case CC (U2, F4):
      *(uint32_t *) dest = *(float *) src;
      break;
    case CC (U8, F8):
      *(uint8_t *) dest = T8 (0.5 + *(double *) src);
      break;                    //iw810
    case CC (U6, F8):
      *(uint16_t *) dest = T6 (0.5 + *(double *) src);
      break;                    //iw810
    case CC (U2, F8):
      *(uint32_t *) dest = *(double *) src;
      break;                    //iw810

      // to float (typically lossless, except for large integers)
    case CC (F4, I8):
      *(float *) dest = *(int8_t *) src;
      break;
    case CC (F4, I6):
      *(float *) dest = *(int16_t *) src;
      break;
    case CC (F4, I2):
      *(float *) dest = *(int32_t *) src;
      break;                    //iw810
    case CC (F8, I8):
      *(double *) dest = *(int8_t *) src;
      break;
    case CC (F8, I6):
      *(double *) dest = *(int16_t *) src;
      break;
    case CC (F8, I2):
      *(double *) dest = *(int32_t *) src;
      break;
    case CC (F4, U8):
      *(float *) dest = *(uint8_t *) src;
      break;
    case CC (F4, U6):
      *(float *) dest = *(uint16_t *) src;
      break;
    case CC (F4, U2):
      *(float *) dest = *(uint32_t *) src;
      break;                    //iw810
    case CC (F8, U8):
      *(double *) dest = *(uint8_t *) src;
      break;
    case CC (F8, U6):
      *(double *) dest = *(uint16_t *) src;
      break;
    case CC (F8, U2):
      *(double *) dest = *(uint32_t *) src;
      break;

#ifdef I_CAN_HAS_INT64
      // to int64_t and uint64_t
    case CC (I4, I8):
      *(int64_t *) dest = *(int8_t *) src;
      break;
    case CC (I4, I6):
      *(int64_t *) dest = *(int16_t *) src;
      break;
    case CC (I4, I2):
      *(int64_t *) dest = *(int32_t *) src;
      break;
    case CC (I4, U8):
      *(int64_t *) dest = *(uint8_t *) src;
      break;
    case CC (I4, U6):
      *(int64_t *) dest = *(uint16_t *) src;
      break;
    case CC (I4, U2):
      *(int64_t *) dest = *(uint32_t *) src;
      break;
    case CC (I4, F4):
      *(int64_t *) dest = *(float *) src;
      break;
    case CC (I4, F8):
      *(int64_t *) dest = *(double *) src;
      break;
    case CC (U4, I8):
      *(uint64_t *) dest = *(int8_t *) src;
      break;
    case CC (U4, I6):
      *(uint64_t *) dest = *(int16_t *) src;
      break;
    case CC (U4, I2):
      *(uint64_t *) dest = *(int32_t *) src;
      break;
    case CC (U4, U8):
      *(uint64_t *) dest = *(uint8_t *) src;
      break;
    case CC (U4, U6):
      *(uint64_t *) dest = *(uint16_t *) src;
      break;
    case CC (U4, U2):
      *(uint64_t *) dest = *(uint32_t *) src;
      break;
    case CC (U4, F4):
      *(uint64_t *) dest = *(float *) src;
      break;
    case CC (U4, F8):
      *(uint64_t *) dest = *(double *) src;
      break;

      // from int64_t and uint64_t
    case CC (I8, I4):
      *(int8_t *) dest = *(int64_t *) src;
      break;
    case CC (I6, I4):
      *(int16_t *) dest = *(int64_t *) src;
      break;
    case CC (I2, I4):
      *(int32_t *) dest = *(int64_t *) src;
      break;
    case CC (U8, I4):
      *(uint8_t *) dest = *(int64_t *) src;
      break;
    case CC (U6, I4):
      *(uint16_t *) dest = *(int64_t *) src;
      break;
    case CC (U2, I4):
      *(uint32_t *) dest = *(int64_t *) src;
      break;
    case CC (F4, I4):
      *(float *) dest = *(int64_t *) src;
      break;
    case CC (F8, I4):
      *(double *) dest = *(int64_t *) src;
      break;
    case CC (I8, U4):
      *(int8_t *) dest = *(uint64_t *) src;
      break;
    case CC (I6, U4):
      *(int16_t *) dest = *(uint64_t *) src;
      break;
    case CC (I2, U4):
      *(int32_t *) dest = *(uint64_t *) src;
      break;
    case CC (U8, U4):
      *(uint8_t *) dest = *(uint64_t *) src;
      break;
    case CC (U6, U4):
      *(uint16_t *) dest = *(uint64_t *) src;
      break;
    case CC (U2, U4):
      *(uint32_t *) dest = *(uint64_t *) src;
      break;
    case CC (F4, U4):
      *(float *) dest = *(uint64_t *) src;
      break;
    case CC (F8, U4):
      *(double *) dest = *(uint64_t *) src;
      break;
#endif //I_CAN_HAS_INT64

#ifdef I_CAN_HAS_LONGDOUBLE
      // to longdouble
    case CC (F6, I8):
      *(longdouble *) dest = *(int8_t *) src;
      break;
    case CC (F6, I6):
      *(longdouble *) dest = *(int16_t *) src;
      break;
    case CC (F6, I2):
      *(longdouble *) dest = *(int32_t *) src;
      break;
    case CC (F6, U8):
      *(longdouble *) dest = *(uint8_t *) src;
      break;
    case CC (F6, U6):
      *(longdouble *) dest = *(uint16_t *) src;
      break;
    case CC (F6, U2):
      *(longdouble *) dest = *(uint32_t *) src;
      break;
    case CC (F6, F4):
      *(longdouble *) dest = *(float *) src;
      break;
    case CC (F6, F8):
      *(longdouble *) dest = *(double *) src;
      break;

      // from longdouble
    case CC (I8, F6):
      *(int8_t *) dest = *(longdouble *) src;
      break;
    case CC (I6, F6):
      *(int16_t *) dest = *(longdouble *) src;
      break;
    case CC (I2, F6):
      *(int32_t *) dest = *(longdouble *) src;
      break;
    case CC (U8, F6):
      *(uint8_t *) dest = T8 (0.5 + *(longdouble *) src);
      break;
    case CC (U6, F6):
      *(uint16_t *) dest = T6 (0.5 + *(longdouble *) src);
      break;
    case CC (U2, F6):
      *(uint32_t *) dest = *(longdouble *) src;
      break;
    case CC (F4, F6):
      *(float *) dest = *(longdouble *) src;
      break;
    case CC (F8, F6):
      *(double *) dest = *(longdouble *) src;
      break;

#ifdef I_CAN_HAS_INT64          //(nested)
    case CC (F6, I4):
      *(longdouble *) dest = *(int64_t *) src;
      break;
    case CC (F6, U4):
      *(longdouble *) dest = *(uint64_t *) src;
      break;
    case CC (I4, F6):
      *(int64_t *) dest = *(longdouble *) src;
      break;
    case CC (U4, F6):
      *(uint64_t *) dest = *(longdouble *) src;
      break;
#endif //I_CAN_HAS_INT64 (nested)

#endif //I_CAN_HAS_LONGDOUBLE

    default:
      error ("bad conversion from %d to %d", src_fmt, dest_fmt);
    }
}

#undef CC
#undef I8
#undef U8
#undef I6
#undef U6
#undef I2
#undef U2
#undef F4
#undef F8
#undef F6

static void *
convert_data (void *src, int n, int dest_fmt, int src_fmt)
{
  if (src_fmt == IIO_TYPE_FLOAT)
    IIO_DEBUG ("first float sample = %g\n", *(float *) src);
  size_t src_width = iio_type_size (src_fmt);
  size_t dest_width = iio_type_size (dest_fmt);
  IIO_DEBUG ("converting %d samples from %s to %s\n", n, iio_strtyp (src_fmt),
             iio_strtyp (dest_fmt));
  IIO_DEBUG ("src width = %zu\n", src_width);
  IIO_DEBUG ("dest width = %zu\n", dest_width);
  char *r = xmalloc (n * dest_width);
  // NOTE: the switch inside "convert_datum" should be optimized
  // outside of this loop
  FORI (n)
  {
    void *to = i * dest_width + r;
    void *from = i * src_width + (char *) src;
    convert_datum (to, from, dest_fmt, src_fmt);
  }
  xfree (src);
  if (dest_fmt == IIO_TYPE_INT16)
    IIO_DEBUG ("first short sample = %d\n", *(int16_t *) r);
  return r;
}

static void
unpack_nibbles_to_bytes (uint8_t out[2], uint8_t in)
{
  out[1] = (in & 0x0f);         //0b00001111
  out[0] = (in & 0xf0) >> 4;    //0b11110000
}

static void
unpack_couples_to_bytes (uint8_t out[4], uint8_t in)
{
  out[3] = (in & 0x03);         //0b00000011
  out[2] = (in & 0x0c) >> 2;    //0b00001100
  out[1] = (in & 0x30) >> 4;    //0b00110000
  out[0] = (in & 0xc0) >> 6;    //0b11000000
}

static void
unpack_bits_to_bytes (uint8_t out[8], uint8_t in)
{
  out[7] = (in & 1) ? 1 : 0;
  out[6] = (in & 2) ? 1 : 0;
  out[5] = (in & 4) ? 1 : 0;
  out[4] = (in & 8) ? 1 : 0;
  out[3] = (in & 16) ? 1 : 0;
  out[2] = (in & 32) ? 1 : 0;
  out[1] = (in & 64) ? 1 : 0;
  out[0] = (in & 128) ? 1 : 0;
}

static void
unpack_to_bytes_here (uint8_t * dest, uint8_t * src, int n, int bits)
{
  fprintf (stderr, "unpacking %d bytes %d-fold\n", n, bits);
  assert (bits == 1 || bits == 2 || bits == 4);
  size_t unpack_factor = 8 / bits;
  switch (bits)
    {
    case 1:
      FORI (n) unpack_bits_to_bytes (dest + 8 * i, src[i]);
      break;
    case 2:
      FORI (n) unpack_couples_to_bytes (dest + 4 * i, src[i]);
      break;
    case 4:
      FORI (n) unpack_nibbles_to_bytes (dest + 2 * i, src[i]);
      break;
    default:
      error ("very strange error");
    }
}

//static uint8_t *unpack_to_bytes(uint8_t *s, int n, int src_bits)
//{
//      assert(src_bits==1 || src_bits==2 || src_bits==4);
//      size_t unpack_factor = 8 / src_bits;
//      uint8_t *r = xmalloc(n * unpack_factor);
//      switch(src_bits) {
//      case 1: FORI(n)    unpack_bits_to_bytes(r + 8*i, s[i]); break;
//      case 2: FORI(n) unpack_couples_to_bytes(r + 4*i, s[i]); break;
//      case 4: FORI(n) unpack_nibbles_to_bytes(r + 2*i, s[i]); break;
//      default: error("very strange error");
//      }
//      xfree(s);
//      return r;
//}

static void
iio_convert_samples (struct iio_image *x, int desired_type)
{
  assert (!x->contiguous_data);
  int source_type = normalize_type (x->type);
  if (source_type == desired_type)
    return;
  IIO_DEBUG ("converting from %s to %s\n", iio_strtyp (x->type),
             iio_strtyp (desired_type));
  int n = iio_image_number_of_samples (x);
  x->data = convert_data (x->data, n, desired_type, source_type);
  x->type = desired_type;
}

static void
iio_hacky_colorize (struct iio_image *x, int pd)
{
  assert (!x->contiguous_data);
  if (x->pixel_dimension != 1)
    error ("please, do not colorize color stuff");
  int source_type = normalize_type (x->type);
  int n = iio_image_number_of_elements (x);
  int ss = iio_image_sample_size (x);
  void *rdata = xmalloc (n * ss * pd);
  char *data_dest = rdata;
  char *data_src = x->data;
  FORI (n)
    FORJ (pd) memcpy (data_dest + (pd * i + j) * ss, data_src + i * ss, ss);
  xfree (x->data);
  x->data = rdata;
  x->pixel_dimension = pd;
}

// uncolorize
static void
iio_hacky_uncolorize (struct iio_image *x)
{
  assert (!x->contiguous_data);
  if (x->pixel_dimension != 3)
    error ("please, do not uncolorize non-color stuff");
  assert (x->pixel_dimension == 3);
  int source_type = normalize_type (x->type);
  int n = iio_image_number_of_elements (x);
  switch (source_type)
    {
    case IIO_TYPE_UINT8:
      {
        uint8_t (*xd)[3] = x->data;
        uint8_t *r = xmalloc (n * sizeof *r);
        FORI (n) r[i] = .299 * xd[i][0] + .587 * xd[i][1] + .114 * xd[i][2];
        xfree (x->data);
        x->data = r;
      }
      break;
    case IIO_TYPE_FLOAT:
      {
        float (*xd)[3] = x->data;
        float *r = xmalloc (n * sizeof *r);
        FORI (n) r[i] = .299 * xd[i][0] + .587 * xd[i][1] + .114 * xd[i][2];
        xfree (x->data);
        x->data = r;
      }
      break;
    default:
      error ("uncolorize type not supported");
    }
  x->pixel_dimension = 1;
}

// uncolorize
static void
iio_hacky_uncolorizea (struct iio_image *x)
{
  assert (!x->contiguous_data);
  if (x->pixel_dimension != 4)
    error ("please, do not uncolorizea non-colora stuff");
  assert (x->pixel_dimension == 4);
  int source_type = normalize_type (x->type);
  int n = iio_image_number_of_elements (x);
  switch (source_type)
    {
    case IIO_TYPE_UINT8:
      {
        uint8_t (*xd)[4] = x->data;
        uint8_t *r = xmalloc (n * sizeof *r);
        FORI (n) r[i] = .299 * xd[i][0] + .587 * xd[i][1] + .114 * xd[i][2];
        xfree (x->data);
        x->data = r;
      }
      break;
    case IIO_TYPE_FLOAT:
      {
        float (*xd)[4] = x->data;
        float *r = xmalloc (n * sizeof *r);
        FORI (n) r[i] = .299 * xd[i][0] + .587 * xd[i][1] + .114 * xd[i][2];
        xfree (x->data);
        x->data = r;
      }
      break;
    default:
      error ("uncolorizea type not supported");
    }
  x->pixel_dimension = 1;
}



// (minimal) image processing using the iio_image {{{1

//TODO: remove this section

// whatever

// get/set pixel {{{2

// the "getpixel" function can be used also as a setpixel
static void *
iio_getpixel (struct iio_image *x, int *p)
{
  FORI (x->dimension) if (!checkbounds (0, p[i], x->sizes[i]))
    return NULL;
  int n = iio_image_sample_size (x);
  char *r = x->data;
  switch (x->dimension)
    {
    case 1:
      return n * p[0] + r;
    case 2:
      return n * p[0] + x->sizes[0] * p[1] + r;
    case 3:
      return n * p[0] + x->sizes[0] * (p[1] + x->sizes[1] * p[2]) + r;
    default:
      error ("getpixel not yet for dimension %d", x->dimension);
    }
}

static void *
iio_getsample (struct iio_image *x, int *p, int c)
{
  if (!checkbounds (0, c, x->pixel_dimension))
    return NULL;
  void *pixel = iio_getpixel (x, p);
  int n = iio_image_sample_size (x);
  return c * n + (char *) pixel;
}

// crop/slice/select {{{2
// To CROP an image means building an image with a "rectangular" subset of its
// numbers As particular cases it contains taking a slice from a video, or a
// color channel.  It does not change the signature.
// signs equals 1.


static void
iio_general_crop (struct iio_image *x, int *corner, int *sizes,
                  int firstchan, int lastchan)
{
  // silently ignores out of range bounds
  int newsizes[x->dimension];
}


// rotate/deinterlace {{{2
// To TRANSPOSE an image means building an image with the same numbers in
// different order.  As particular it contains rotation, (de)interlacing, etc.


static void
iio_general_transposition (struct iio_image *x, int c[2])
{
}


// ntiply/colorize {{{2
// to NTIPLY an image means building an image where some numbers are repeated
// As particular cases it contains zooming in an image (with nearest neighbor
// interpolation), or colorizing a gray image.


static void
iio_general_ntiply (struct iio_image *x, int *factors, int pfac)
{
  // too complicated for now...
}



// normalize signature {{{2
static void
iio_general_normalize_signature (struct iio_image *x)
{
}

// general memory and file utilities {{{1


// Input: a partially read stream "f"
// (of which "bufn" bytes are already read into "buf")
//
// Output: a malloc'd block with the whole file content
//
// Implementation: re-invent the wheel
static void *
load_rest_of_file (long *on, FILE * f, void *buf, size_t bufn)
{
  size_t n = bufn, ntop = n + 0x3000;
  char *t = xmalloc (ntop);
  if (!t)
    error ("out of mem (%zu) while loading file", ntop);
  memcpy (t, buf, bufn);
  while (1)
    {
      if (n >= ntop)
        {
          ntop = 1000 + 2 * (ntop + 1);
          t = xrealloc (t, ntop);
          if (!t)
            error ("out of mem (%zu) loading file", ntop);

        }
      int r = fgetc (f);
      if (r == EOF)
        break;
      t[n] = r;                 //iw810
      n += 1;
    }
  *on = n;
  return t;
}

// Input: a pointer to raw data
//
// Output: the name of a temporary file containing the data
//
// Implementation: re-invent the wheel
static char *
put_data_into_temporary_file (void *filedata, size_t filesize)
{
#ifdef I_CAN_HAS_MKSTEMP
  static char filename[] = "/tmp/iio_temporal_file_XXXXXX\0";
  int r = mkstemp (filename);
  if (r == -1)
    error ("caca [pditf]");
#else
  // WARNING XXX XXX XXX ERROR FIXME TODO WARNING:
  // this function is not reentrant
  static char buf[L_tmpnam + 1];
  //
  // from TMPNAM(3):
  //
  //The  tmpnam()  function  returns  a pointer to a string that is a
  //valid filename, and such that a file with this name did  not  exist
  //at  some point  in  time, so that naive programmers may think it a
  //suitable name for a temporary file.
  //
  char *filename = tmpnam (buf);
  // MULTIPLE RACE CONDITIONS HERE
#endif
  FILE *f = xfopen (filename, "w");
  int cx = fwrite (filedata, filesize, 1, f);
  if (cx != 1)
    error ("fwrite to temporary file failed");
  xfclose (f);
  return filename;
}

static void
delete_temporary_file (char *filename)
{
#ifdef NDEBUG
  remove (filename);
#else
  //IIO_DEBUG("WARNING: kept temporary file %s around\n", filename);
  fprintf (stderr, "WARNING: kept temporary file %s around\n", filename);
#endif
}


// Allows read access to memory via a FILE*
// Always returns a valid FILE*
static FILE *
iio_fmemopen (void *data, size_t size)
{
#ifdef I_CAN_HAS_FMEMOPEN       // GNU case
  FILE *r = fmemopen (data, size, "r");
  if (!r)
    error ("fmemopen failed");
  return r;
#elif  I_CAN_HAS_FUNOPEN        // BSD case
  error ("implement fmemopen using funopen here");
#else // portable case
  FILE *f = tmpfile ();
  if (!f)
    error ("tmpfile failed");
  int cx = fwrite (data, size, 1, f);
  if (cx != 1)
    error ("fwrite failed");
  rewind (f);
  return f;
#endif // I_CAN_HAS_...
}


// beautiful hack follows
static void *
matrix_build (int w, int h, size_t n)
{
  size_t p = sizeof (void *);
  char *r = xmalloc (h * p + w * h * n);
  for (int i = 0; i < h; i++)
    *(void **) (r + i * p) = r + h * p + i * w * n;
  return r;
}

static void *
wrap_2dmatrix_around_data (void *x, int w, int h, size_t s)
{
  void *r = matrix_build (w, h, s);
  char *y = h * sizeof (void *) + (char *) r;
  memcpy (y, x, w * h * s);
  xfree (x);
  return r;
}


static int
atoin (char *s)
{
  int r = 0;
  while (isdigit (*s))
    {
      r *= 10;
      r += *s - '0';
      s += 1;
    }
  return r;
}

// heurística per trobar les dimensions a partir del nom
static bool
find_dimensions_in_string_2d (int out[2], char *s)
{
  // preprocessem el text per trobar-ne els dígits
  int n = strlen (s), nnums = 0, numbegs[n];
  for (int i = 1; i < n; i++)
    if (isdigit (s[i]) && !isdigit (s[i - 1]))
      numbegs[nnums++] = i;
  if (nnums < 2)
    return false;

  // si el text conté algun parell "%dx%d", retornem el primer parell
  for (int i = 1; i < nnums; i++)
    if (tolower (s[numbegs[i] - 1]) == 'x')
      {
        out[0] = atoin (s + numbegs[i - 1]);
        out[1] = atoin (s + numbegs[i]);
        return true;
      }

  // altrament, cerquem els dos primers números
  out[0] = atoin (s + numbegs[0]);
  out[1] = atoin (s + numbegs[1]);
  return true;
}

static bool
find_dimensions_in_string_3d (int out[3], char *s)
{
  // preprocessem el text per trobar-ne els dígits
  int n = strlen (s), nnums = 0, numbegs[n];
  for (int i = 1; i < n; i++)
    if (isdigit (s[i]) && !isdigit (s[i - 1]))
      numbegs[nnums++] = i;
  if (nnums < 3)
    return false;

  // si el text conté algun trio "%dx%dx%d", retornem el primer trio
  for (int i = 2; i < nnums; i++)
    if (tolower (s[numbegs[i] - 1]) == 'x'
        && tolower (s[numbegs[i - 1] - 1]) == 'x')
      {
        out[0] = atoin (s + numbegs[i - 2]);
        out[1] = atoin (s + numbegs[i - 1]);
        out[2] = atoin (s + numbegs[i]);
        return true;
      }

  // altrament, cerquem els dos primers números
  out[0] = atoin (s + numbegs[0]);
  out[1] = atoin (s + numbegs[1]);
  out[2] = atoin (s + numbegs[2]);
  return true;
}

static void
break_pixels (void *broken, void *clear, int ss, int n, int pd)
{
  char *to = broken;
  char *from = clear;
  FORI (n) FORL (pd) FORK (ss)
    to[(n * l + i) * ss + k] = from[(pd * i + l) * ss + k];

}

static void
recover_broken_pixels (void *c, void *b, int ss, int n, int pd)
{
  char *to = c;
  char *from = b;
  FORL (pd) FORI (n) FORK (ss)
    to[(pd * i + l) * ss + k] = from[(n * l + i) * ss + k];
}

// todo make this function more general, or a front-end to a general
// "data trasposition" routine
static void
break_pixels_float (float *broken, float *clear, int n, int pd)
{
  //fprintf(stderr, "breaking %d %d-dimensional vectors\n", n, pd);
  FORI (n) FORL (pd) broken[n * l + i] = clear[pd * i + l];
}

static void
recover_broken_pixels_float (float *clear, float *broken, int n, int pd)
{
  //fprintf(stderr, "unbreaking %d %d-dimensional vectors\n", n, pd);
  FORL (pd) FORI (n) clear[pd * i + l] = broken[n * l + i];
}

// individual format readers {{{1
// PNG reader {{{2

static void
swap_two_bytes (char *here)
{
  char tmp = here[0];
  here[0] = here[1];
  here[1] = tmp;
}

#ifdef I_CAN_HAS_LIBPNG
//#include <png.h>
#include <limits.h>             // for CHAR_BIT only
static int
read_beheaded_png (struct iio_image *x, FILE * f, char *header, int nheader)
{
  // TODO: reorder this mess
  png_structp pp = png_create_read_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (!pp)
    error ("png_create_read_struct fail");
  png_infop pi = png_create_info_struct (pp);
  if (!pi)
    error ("png_create_info_struct fail");
  if (setjmp (png_jmpbuf (pp)))
    error ("png error");
  png_init_io (pp, f);
  png_set_sig_bytes (pp, nheader);
  int transforms = PNG_TRANSFORM_IDENTITY
    | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND;
  png_read_png (pp, pi, transforms, NULL);
  png_uint_32 w, h;
  int channels, rowbytes;
  int depth, color, interl, compr, filt;
  w = png_get_image_width (pp, pi);
  h = png_get_image_height (pp, pi);
  //png_get_IHDR(pp, pi, &w, &h, &depth, &color, &interl, &compr, &filt);
  channels = png_get_channels (pp, pi);
  rowbytes = png_get_rowbytes (pp, pi);
  depth = png_get_bit_depth (pp, pi);
  color = png_get_color_type (pp, pi);
  IIO_DEBUG ("png get width = %d\n", (int) w);
  IIO_DEBUG ("png get height = %d\n", (int) h);
  IIO_DEBUG ("png get channels = %d\n", channels);
  IIO_DEBUG ("png get rowbytes = %d\n", rowbytes);
  IIO_DEBUG ("png get depth = %d\n", depth);
  IIO_DEBUG ("png get color = %d\n", color);
  //IIO_DEBUG("png ihdr width = %d\n", (int)w);
  //IIO_DEBUG("png ihdr height = %d\n", (int)h);
  //IIO_DEBUG("png ihdr depth = %d\n", depth);
  //IIO_DEBUG("png ihdr color = %d\n", color);
  //IIO_DEBUG("png ihdr interl = %d\n", interl);
  //IIO_DEBUG("png ihdr compr = %d\n", compr);
  //IIO_DEBUG("png ihdr filt = %d\n", filt);
  //IIO_DEBUG("png channels = %d\n", channels);
  //IIO_DEBUG("png rowbytes = %d\n", rowbytes);
  //assert(rowbytes == ceil((channels * (int)w * depth)/CHAR_BIT));
  //if (depth == 16) png_set_swap(pp); (does not seem to work)
  int sizes[2] = { w, h };
  png_bytepp rows = png_get_rows (pp, pi);
  x->format = IIO_FORMAT_PNG;
  x->meta = -42;
  switch (depth)
    {
    case 1:
    case 8:
      IIO_DEBUG ("first byte = %d\n", (int) rows[0][0]);
      iio_image_build_independent (x, 2, sizes, IIO_TYPE_CHAR, channels);
      FORJ (h) FORI (w) FORL (channels)
      {
        char *data = x->data;
        png_byte *b = rows[j] + i * channels + l;
        data[(i + j * w) * channels + l] = *b;
      }
      x->type = IIO_TYPE_CHAR;
      break;
    case 16:
      iio_image_build_independent (x, 2, sizes, IIO_TYPE_UINT16, channels);
      FORJ (h) FORI (w) FORL (channels)
      {
        uint16_t *data = x->data;
        png_byte *b = rows[j] + 2 * (i * channels + l);
        swap_two_bytes ((char *) b);
        uint16_t *tb = (uint16_t *) b;
        data[(i + j * w) * channels + l] = *tb;
      }
      x->type = IIO_TYPE_UINT16;
      break;
    default:
      error ("unsuported bit depth %d", depth);
    }
  //FORJ(h) FORIk
  //png_destroy_read_struct(&pp, &pi, &pe);
  png_destroy_read_struct (&pp, &pi, NULL);
  return 0;
}

#endif //I_CAN_HAS_LIBPNG

// JPEG reader {{{2

#ifdef I_CAN_HAS_LIBJPEG
#include <jpeglib.h>

static int
read_whole_jpeg (struct iio_image *x, FILE * f)
{
  // allocate and initialize a JPEG decompression object
  struct jpeg_decompress_struct cinfo[1];
  struct jpeg_error_mgr jerr[1];
  cinfo->err = jpeg_std_error (jerr);
  jpeg_create_decompress (cinfo);

  // specify the source of the compressed data
  jpeg_stdio_src (cinfo, f);

  // obtain image info
  jpeg_read_header (cinfo, 1);
  int size[2], depth;
  size[0] = cinfo->image_width;
  size[1] = cinfo->image_height;
  depth = cinfo->num_components;
  IIO_DEBUG ("jpeg header widht = %d\n", size[0]);
  IIO_DEBUG ("jpeg header height = %d\n", size[1]);
  IIO_DEBUG ("jpeg header colordepth = %d\n", depth);
  iio_image_build_independent (x, 2, size, IIO_TYPE_CHAR, depth);

  // set parameters for decompression
  // cinfo->do_fancy_upsampling = 0;
  // colorspace selection, etc

  // start decompress
  jpeg_start_decompress (cinfo);
  assert (size[0] == (int) cinfo->output_width);
  assert (size[1] == (int) cinfo->output_height);
  assert (depth == cinfo->out_color_components);
  assert (cinfo->output_components == cinfo->out_color_components);

  // read scanlines
  FORI (size[1])
  {
    void *wheretoputit = i * depth * size[0] + (char *) x->data;
    //FORJ(size[0]*depth) ((char*)wheretoputit)[j] = 6;
    JSAMPROW scanline[1] = { wheretoputit };
    int r = jpeg_read_scanlines (cinfo, scanline, 1);
    //IIO_DEBUG("read %dth scanline (r=%d) {%d}\n", i, r, (int)sizeof(JSAMPLE));
    if (1 != r)
      error ("failed to rean jpeg scanline %d", i);
  }

  // finish decompress
  jpeg_finish_decompress (cinfo);

  // release the JPEG decompression object
  jpeg_destroy_decompress (cinfo);

  return 0;
}

static int
read_beheaded_jpeg (struct iio_image *x,
                    FILE * fin, char *header, int nheader)
{
  long filesize;
  // TODO: if "f" is rewindable, rewind it!
  void *filedata = load_rest_of_file (&filesize, fin, header, nheader);
  FILE *f = iio_fmemopen (filedata, filesize);

  int r = read_whole_jpeg (x, f);
  if (r)
    error ("read whole jpeg returned %d", r);
  fclose (f);
  xfree (filedata);

  return 0;
}
#endif //I_CAN_HAS_LIBJPEG

// TIFF reader {{{2

#ifdef I_CAN_HAS_LIBTIFF
#include <tiffio.h>

static int
read_whole_tiff (struct iio_image *x, const char *filename)
{
  // tries to read data in the correct format (via scanlines)
  // if it fails, it tries to read ABGR data

  TIFF *tif = TIFFOpen (filename, "r");
  if (!tif)
    error ("could not open TIFF file \"%s\"", filename);
  uint32_t w, h;
  uint16_t spp, bps, fmt;
  int r = 0, fmt_iio = -1;
  r += TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &w);
  IIO_DEBUG ("tiff get field width %d (r=%d)\n", (int) w, r);
  r += TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &h);
  IIO_DEBUG ("tiff get field length %d (r=%d)\n", (int) h, r);
  if (r != 2)
    error ("can not read tiff of unknown size");

  r = TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
  if (!r)
    spp = 1;
  if (r)
    IIO_DEBUG ("tiff get field spp %d (r=%d)\n", spp, r);

  r = TIFFGetField (tif, TIFFTAG_BITSPERSAMPLE, &bps);
  if (!r)
    bps = 1;
  if (r)
    IIO_DEBUG ("tiff get field bps %d (r=%d)\n", bps, r);

  r = TIFFGetField (tif, TIFFTAG_SAMPLEFORMAT, &fmt);
  if (!r)
    fmt = SAMPLEFORMAT_UINT;
  if (r)
    IIO_DEBUG ("tiff get field fmt %d (r=%d)\n", fmt, r);
  //if (r != 5) error("some tiff getfield failed (r=%d)", r);

  // TODO: consider the missing cases (run through PerlMagick's format database)

  IIO_DEBUG ("fmt  = %d\n", fmt);
  // set appropriate size and type flags
  if (fmt == SAMPLEFORMAT_UINT)
    {
      if (1 == bps)
        fmt_iio = IIO_TYPE_UINT1;
      else if (2 == bps)
        fmt_iio = IIO_TYPE_UINT2;
      else if (4 == bps)
        fmt_iio = IIO_TYPE_UINT4;
      else if (8 == bps)
        fmt_iio = IIO_TYPE_UINT8;
      else if (16 == bps)
        fmt_iio = IIO_TYPE_UINT16;
      else if (32 == bps)
        fmt_iio = IIO_TYPE_UINT32;
      else
        error ("unrecognized UINT type of size %d bits", bps);
    }
  else if (fmt == SAMPLEFORMAT_INT)
    {
      if (8 == bps)
        fmt_iio = IIO_TYPE_INT8;
      else if (16 == bps)
        fmt_iio = IIO_TYPE_INT16;
      else if (32 == bps)
        fmt_iio = IIO_TYPE_INT32;
      else
        error ("unrecognized INT type of size %d bits", bps);
    }
  else if (fmt == SAMPLEFORMAT_IEEEFP)
    {
      IIO_DEBUG ("floating tiff!\n");
      if (32 == bps)
        fmt_iio = IIO_TYPE_FLOAT;
      else if (64 == bps)
        fmt_iio = IIO_TYPE_DOUBLE;
      else
        error ("unrecognized FLOAT type of size %d bits", bps);
    }
  else
    error ("unrecognized tiff sample format %d (see tiff.h)", fmt);

  if (bps >= 8 && bps != 8 * iio_type_size (fmt_iio))
    {
      IIO_DEBUG ("bps = %d\n", bps);
      IIO_DEBUG ("fmt_iio = %d\n", fmt_iio);
      IIO_DEBUG ("ts = %zu\n", iio_type_size (fmt_iio));
      IIO_DEBUG ("8*ts = %zu\n", 8 * iio_type_size (fmt_iio));
    }
  if (bps >= 8)
    assert (bps == 8 * iio_type_size (fmt_iio));


  // acquire memory block
  uint32_t scanline_size = (w * spp * bps) / 8;
  int rbps = bps / 8 ? bps / 8 : 1;
  uint32_t uscanline_size = w * spp * rbps;
  IIO_DEBUG ("bps = %d\n", (int) bps);
  IIO_DEBUG ("spp = %d\n", (int) spp);
  IIO_DEBUG ("sls = %d\n", (int) scanline_size);
  int sls = TIFFScanlineSize (tif);
  IIO_DEBUG ("sls(r) = %d\n", (int) sls);
  assert ((int) scanline_size == sls);
  scanline_size = sls;
  uint8_t *data = xmalloc (w * h * spp * rbps);
  //FORI(h*scanline_size) data[i] = 42;
  uint8_t *buf = xmalloc (scanline_size);

  // dump scanline data
  FORI (h)
  {
    //r = TIFFReadScanline(tif, data + i * scanline_size, i,  0);
    r = TIFFReadScanline (tif, buf, i, 0);
    if (r < 0)
      error ("error reading tiff row %d/%d", i, (int) h);

    if (bps < 8)
      {
        fprintf (stderr, "unpacking %dth scanline\n", i);
        unpack_to_bytes_here (data + i * uscanline_size, buf,
                              scanline_size, bps);
        fmt_iio = IIO_TYPE_UINT8;
      }
    else
      {
        //assert(uscanline_size == scanline_size);
        memcpy (data + i * scanline_size, buf, scanline_size);
      }
  }
  TIFFClose (tif);


  xfree (buf);

  // fill struct fields
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = spp;
  x->type = fmt_iio;
  x->format = x->meta = -42;
  x->data = data;
  x->contiguous_data = false;
  return 0;
}

// Note: TIFF library only reads from a named file.  The only way to use the
// library if you have your image in a stream, is to write the data to a
// temporary file, and read it from there.  This can be optimized in the future
// by recovering the original filename through hacks, in case it was not a pipe.
static int
read_beheaded_tiff (struct iio_image *x,
                    FILE * fin, char *header, int nheader)
{
  if (global_variable_containing_the_name_of_the_last_opened_file)
    {
      int r = read_whole_tiff (x,
                               global_variable_containing_the_name_of_the_last_opened_file);
      if (r)
        error ("read whole tiff returned %d", r);
      return 0;
    }

  long filesize;
  void *filedata = load_rest_of_file (&filesize, fin, header, nheader);
  char *filename = put_data_into_temporary_file (filedata, filesize);
  xfree (filedata);
  //IIO_DEBUG("tmpfile = \"%s\"\n", filename);


  int r = read_whole_tiff (x, filename);
  if (r)
    error ("read whole tiff returned %d", r);

  delete_temporary_file (filename);

  return 0;
}

#endif //I_CAN_HAS_LIBTIFF

// QNM readers {{{2

#include <ctype.h>

static void
llegeix_floats_en_bytes (FILE * don, float *on, int quants)
{
  for (int i = 0; i < quants; i++)
    {
      float c;
      c = pilla_caracter_segur (don);   //iw810
      on[i] = c;
    }
}

static void
llegeix_floats_en_shorts (FILE * don, float *on, int quants)
{
  for (int i = 0; i < quants; i++)
    {
      float c;
      c = pilla_caracter_segur (don);
      c *= 256;
      c += pilla_caracter_segur (don);
      on[i] = c;
    }
}

static void
llegeix_floats_en_ascii (FILE * don, float *on, int quants)
{
  for (int i = 0; i < quants; i++)
    {
      int r;
      float c;
      r = fscanf (don, "%f ", &c);
      if (r != 1)
        error ("no s'han pogut llegir %d numerets del fitxer &%p\n",
               quants, (void *) don);
      on[i] = c;
    }
}

static int
read_qnm_numbers (float *data, FILE * f, int n, int m, bool use_ascii)
{
  if (use_ascii)
    llegeix_floats_en_ascii (f, data, n);
  else
    {
      if (m < 0x100)
        llegeix_floats_en_bytes (f, data, n);
      else if (m < 0x10000)
        llegeix_floats_en_shorts (f, data, n);
      else
        error ("too large maxval %d", m);
    }
  // TODO: error checking
  return n;
}

// qnm_types:
//  2 P2 (ascii  2d grayscale pgm)
//  5 P5 (binary 2d grayscale pgm)
//  3 P3 (ascii  2d color     ppm)
//  6 P6 (binary 2d color     ppm)
// 12 Q2 (ascii  3d grayscale pgm)
// 15 Q5 (binary 3d grayscale pgm)
// 13 Q3 (ascii  3d color     ppm)
// 16 Q6 (binary 3d color     ppm)
// 17 Q7 (ascii  3d nd           )
// 19 Q9 (binary 3d nd           )
static int
read_beheaded_qnm (struct iio_image *x, FILE * f, char *header, int nheader)
{
  assert (nheader == 2);
  int w, h, d = 1, m, pd = 1;
  int c1 = header[0];
  int c2 = header[1] - '0';
  IIO_DEBUG ("QNM reader (%c %d)...\n", c1, c2);
  menja_espais_i_comentaris (f);
  if (1 != fscanf (f, "%d", &w))
    return -1;
  menja_espais_i_comentaris (f);
  if (1 != fscanf (f, "%d", &h))
    return -2;
  if (c1 == 'Q')
    {
      if (1 != fscanf (f, "%d", &d))
        return -3;
      menja_espais_i_comentaris (f);
    }
  if (c2 == 7 || c2 == 9)
    {
      if (1 != fscanf (f, "%d", &pd))
        return -4;
      menja_espais_i_comentaris (f);
    }
  if (1 != fscanf (f, "%d", &m))
    return -5;
  // maxval is ignored and the image is always read into floats
  if (!isspace (pilla_caracter_segur (f)))
    return -6;

  bool use_ascii = (c2 == 2 || c2 == 3 || c2 == 7);
  bool use_2d = (d == 1);
  if (!use_2d)
    assert (c1 == 'Q');
  if (c2 == 3 || c2 == 6)
    pd = 3;
  size_t nn = w * h * d * pd;   // number of numbers
  float *data = xmalloc (nn * sizeof *data);
  IIO_DEBUG ("QNM reader w = %d\n", w);
  IIO_DEBUG ("QNM reader h = %d\n", h);
  IIO_DEBUG ("QNM reader d = %d\n", d);
  IIO_DEBUG ("QNM reader pd = %d\n", pd);
  IIO_DEBUG ("QNM reader m = %d\n", m);
  IIO_DEBUG ("QNM reader use_2d = %d\n", use_2d);
  IIO_DEBUG ("QNM reader use_ascii = %d\n", use_ascii);
  int r = read_qnm_numbers (data, f, nn, m, use_ascii);
  if (nn - r)
    return -7;

  x->dimension = use_2d ? 2 : 3;
  x->sizes[0] = w;
  x->sizes[1] = h;
  if (d > 1)
    x->sizes[2] = d;
  x->pixel_dimension = pd;
  x->type = IIO_TYPE_FLOAT;
  x->contiguous_data = false;
  x->data = data;
  return 0;
}

// PCM reader {{{2
// PCM is a file format to store complex float images
// it is used by some people also for optical flow fields
static int
read_beheaded_pcm (struct iio_image *x, FILE * f, char *header, int nheader)
{
  assert (nheader == 2);
  int w, h;
  float scale;
  //menja_espais_i_comentaris(f);
  if (1 != fscanf (f, " %d", &w))
    return -1;
  //menja_espais_i_comentaris(f);
  if (1 != fscanf (f, " %d", &h))
    return -2;
  //menja_espais_i_comentaris(f);
  if (1 != fscanf (f, " %g", &scale))
    return -3;
  if (!isspace (pilla_caracter_segur (f)))
    return -6;

  fprintf (stderr, "%d PCM w h scale = %d %d %g\n", nheader, w, h, scale);

  assert (sizeof (float) == 4);
  float *data = xmalloc (w * h * 2 * sizeof (float));
  int r = fread (data, sizeof (float), w * h * 2, f);
  if (r != w * h * 2)
    return -7;
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 2;
  x->type = IIO_TYPE_FLOAT;
  x->contiguous_data = false;
  x->data = data;
  return 0;
}


// RIM reader {{{2


// CIMAGE header bytes: ("IMG" format)
// short 0: 'MI'
// short 1: comment length
// short 2: image width
// short 3: image height
//

// FIMAGE header bytes: ("RIM" format)
// short 0: 'IR'
// short 1: comment length
// short 2: image width
// short 3: image height
//

// CCIMAGE header bytes: ("MTI" format)
//
//
//

static uint16_t
rim_getshort (FILE * f, bool swp)
{
  int a = pilla_caracter_segur (f);
  int b = pilla_caracter_segur (f);
  IIO_DEBUG ("rgs %.2x%.2x\n", a, b);
  assert (a >= 0);
  assert (b >= 0);
  assert (a < 256);
  assert (b < 256);
  uint16_t r = swp ? b * 0x100 + a : a * 0x100 + b;
  return r;
}

// Note: different order than for shorts.
// Fascinating braindeadness.
static uint32_t
rim_getint (FILE * f, bool swp)
{
  int a = pilla_caracter_segur (f);
  int b = pilla_caracter_segur (f);
  int c = pilla_caracter_segur (f);
  int d = pilla_caracter_segur (f);
  IIO_DEBUG ("rgi %.2x%.2x %.2x%.2x\n", a, b, c, d);
  assert (a >= 0);
  assert (b >= 0);
  assert (c >= 0);
  assert (d >= 0);
  assert (a < 256);
  assert (b < 256);
  assert (c < 256);
  assert (d < 256);
  uint32_t r = swp ?
    a * 0x1000000 + b * 0x10000 + c * 0x100 + d :
    d * 0x1000000 + c * 0x10000 + b * 0x100 + a;
  return r;

}

static int
read_beheaded_rim_cimage (struct iio_image *x, FILE * f, bool swp)
{
  uint16_t lencomm = rim_getshort (f, swp);
  uint16_t dx = rim_getshort (f, swp);
  uint16_t dy = rim_getshort (f, swp);
  IIO_DEBUG ("RIM READER lencomm = %d\n", (int) lencomm);
  IIO_DEBUG ("RIM READER dx = %d\n", (int) dx);
  IIO_DEBUG ("RIM READER dy = %d\n", (int) dy);
  FORI (28) rim_getshort (f, swp);      // skip shit (ascii numbers and zeros)
  FORI (lencomm)
  {
    int c = pilla_caracter_segur (f);   // skip further shit (comments)
    IIO_DEBUG ("RIM READER comment[%d] = '%c'\n", i, c);
  }
  float *data = xmalloc (dx * dy);
  size_t r = fread (data, 1, dx * dy, f);
  if (r != (size_t) (dx * dy))
    error ("could not read entire RIM file:\n"
           "expected %zu chars, but got only %zu", (size_t) dx * dy, r);
  int s[2] = { dx, dy };
  iio_wrap_image_struct_around_data (x, 2, s, 1, IIO_TYPE_UINT8, data);
  return 0;
}

static void
byteswap4 (void *data, int n)
{
  char *t = data;
  FORI (n)
  {
    char *t4 = t + 4 * i;
    char tt[4] = { t4[3], t4[2], t4[1], t4[0] };
    FORL (4) t4[l] = tt[l];
  }
}

static int
read_beheaded_rim_fimage (struct iio_image *x, FILE * f, bool swp)
{
  IIO_DEBUG ("rim reader fimage swp = %d", swp);
  uint16_t lencomm = rim_getshort (f, swp);
  uint16_t dx = rim_getshort (f, swp);
  uint16_t dy = rim_getshort (f, swp);
  //IIO_DEBUG("RIM READER lencomm = %d\n", (int)lencomm);
  //IIO_DEBUG("RIM READER dx = %d\n", (int)dx);
  //IIO_DEBUG("RIM READER dy = %d\n", (int)dy);
  FORI (28) rim_getshort (f, swp);      // skip shit (ascii numbers and zeros)
  FORI (lencomm)
  {
    int c = pilla_caracter_segur (f);   // skip further shit (comments)
    //IIO_DEBUG("RIM READER comment[%d] = '%c'\n", i, c);
  }
  // now, read dx*dy floats
  float *data = xmalloc (dx * dy * sizeof *data);
  size_t r = fread (data, sizeof *data, dx * dy, f);
  if (r != (size_t) (dx * dy))
    error ("could not read entire RIM file:\n"
           "expected %zu floats, but got only %zu", (size_t) dx * dy, r);
  assert (sizeof (float) == 4);
  if (swp)
    byteswap4 (data, r);
  int s[2] = { dx, dy };
  iio_wrap_image_struct_around_data (x, 2, s, 1, IIO_TYPE_FLOAT, data);
  return 0;
}

static int
read_beheaded_rim_ccimage (struct iio_image *x, FILE * f, bool swp)
{
  uint16_t iv = rim_getshort (f, swp);
  if (iv != 0x4956 && iv != 0x5649 && iv != 0x4557 && iv != 0x5745)
    error ("bad ccimage header %x", (int) iv);
  uint32_t pm_np = rim_getint (f, swp);
  uint32_t pm_nrow = rim_getint (f, swp);
  uint32_t pm_ncol = rim_getint (f, swp);
  uint32_t pm_band = rim_getint (f, swp);
  uint32_t pm_form = rim_getint (f, swp);
  uint32_t pm_cmtsize = rim_getint (f, swp);

  IIO_DEBUG ("RIM READER pm_np = %d\n", (int) pm_np);
  IIO_DEBUG ("RIM READER pm_nrow = %d\n", (int) pm_nrow);
  IIO_DEBUG ("RIM READER pm_ncol = %d\n", (int) pm_ncol);
  IIO_DEBUG ("RIM READER pm_band = %d\n", (int) pm_band);
  IIO_DEBUG ("RIM READER pm_form = %x\n", (int) pm_form);
  IIO_DEBUG ("RIM READER pm_cmtsize = %d\n", (int) pm_cmtsize);
  uint32_t nsamples = pm_np * pm_nrow * pm_ncol;
  if (pm_form == 0x8001)
    {                           // ccimage
      uint8_t *data = xmalloc (nsamples);
      size_t r = fread (data, 1, nsamples, f);
      if (r != nsamples)
        error ("rim reader failed at reading %zu "
               "samples (got only %zu)\n", (size_t) nsamples, r);
      uint8_t *good_data = xmalloc (nsamples);
      FORJ (pm_nrow) FORI (pm_ncol) FORL (pm_np)
        good_data[l + (i + j * pm_ncol) * pm_np] =
        data[i + j * pm_ncol + l * pm_ncol * pm_nrow];
      xfree (data);
      data = good_data;
      int s[2] = { pm_ncol, pm_nrow };
      iio_wrap_image_struct_around_data (x, 2, s, pm_np, IIO_TYPE_UINT8,
                                         data);
    }
  else if (pm_form == 0xc004)
    {                           // cfimage
      float *data = xmalloc (4 * nsamples);
      size_t r = fread (data, 4, nsamples, f);
      if (r != nsamples)
        error ("rim reader failed at reading %zu "
               "samples (got only %zu)\n", (size_t) nsamples, r);
      float *good_data = xmalloc (4 * nsamples);
      FORJ (pm_nrow) FORI (pm_ncol) FORL (pm_np)
        good_data[l + (i + j * pm_ncol) * pm_np] =
        data[i + j * pm_ncol + l * pm_ncol * pm_nrow];
      xfree (data);
      data = good_data;
      int s[2] = { pm_ncol, pm_nrow };
      iio_wrap_image_struct_around_data (x, 2, s, pm_np, IIO_TYPE_FLOAT,
                                         data);
    }
  else
    error ("unsupported PM_form %x", pm_form);
  return 0;
}

static int
read_beheaded_rim (struct iio_image *x, FILE * f, char *h, int nh)
{
  assert (nh == 2);
  if (h[0] == 'I' && h[1] == 'R')
    return read_beheaded_rim_fimage (x, f, false);
  if (h[0] == 'R' && h[1] == 'I')
    return read_beheaded_rim_fimage (x, f, true);

  if (h[0] == 'M' && h[1] == 'I')
    return read_beheaded_rim_cimage (x, f, false);
  if (h[0] == 'I' && h[1] == 'M')
    return read_beheaded_rim_cimage (x, f, true);

  if (h[0] == 'W' && h[1] == 'E')
    return read_beheaded_rim_ccimage (x, f, false);
  if (h[0] == 'V' && h[1] == 'I')
    return read_beheaded_rim_ccimage (x, f, true);
  return 1;
}

static void
switch_4endianness (void *tt, int n)
{
  char *t = tt;
  FORI (n)
  {
    char tmp[4] = { t[0], t[1], t[2], t[3] };
    t[0] = tmp[3];
    t[1] = tmp[2];
    t[2] = tmp[1];
    t[3] = tmp[0];
    t += 4;
  }
}

// PFM reader {{{2
static int
read_beheaded_pfm (struct iio_image *x, FILE * f, char *header, int nheader)
{
  assert (4 == sizeof (float));
  assert (nheader == 2);
  assert ('f' == tolower (header[1]));
  int w, h, pd = isupper (header[1]) ? 3 : 1;
  float scale;
  if (!isspace (pilla_caracter_segur (f)))
    return -1;
  if (3 != fscanf (f, "%d %d\n%g", &w, &h, &scale))
    return -2;
  if (!isspace (pilla_caracter_segur (f)))
    return -3;
  float *data = xmalloc (w * h * 4 * pd);
  if (1 != fread (data, w * h * 4 * pd, 1, f))
    return -4;
  //if (scale < 0) switch_4endianness(data, w*h*pd);

  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = pd;
  x->type = IIO_TYPE_FLOAT;
  x->contiguous_data = false;
  x->data = data;
  return 0;
}

// FLO reader {{{2
static int
read_beheaded_flo (struct iio_image *x, FILE * f, char *header, int nheader)
{
  int w = rim_getint (f, false);
  int h = rim_getint (f, false);
  float *data = xmalloc (w * h * 2 * sizeof *data);
  if (1 != fread (data, w * h * 4 * 2, 1, f))
    return -1;

  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 2;
  x->type = IIO_TYPE_FLOAT;
  x->contiguous_data = false;
  x->data = data;
  return 0;
}

// JUV reader {{{2
static int
read_beheaded_juv (struct iio_image *x, FILE * f, char *header, int nheader)
{
  char buf[255];
  FORI (nheader) buf[i] = header[i];
  FORI (255 - nheader) buf[i + nheader] = pilla_caracter_segur (f);
  int w, h, r = sscanf (buf, "#UV {\n dimx %d dimy %d\n}\n", &w, &h);
  if (r != 2)
    return -1;
  size_t sf = sizeof (float);
  float *u = xmalloc (w * h * sf);
  r = fread (u, sf, w * h, f);
  if (r != w * h)
    return -2;
  float *v = xmalloc (w * h * sf);
  r = fread (v, sf, w * h, f);
  if (r != w * h)
    return -2;
  float *uv = xmalloc (2 * w * h * sf);
  FORI (w * h) uv[2 * i] = u[i];
  FORI (w * h) uv[2 * i + 1] = v[i];
  xfree (u);
  xfree (v);
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 2;
  x->type = IIO_TYPE_FLOAT;
  x->contiguous_data = false;
  x->data = uv;
  return 0;
}

// LUM reader {{{2

static int
lum_pickshort (char *ss)
{
  uint8_t *s = (uint8_t *) ss;
  int a = s[0];
  int b = s[1];
  return 0x100 * a + b;
}

static int
read_beheaded_lum (struct iio_image *x, FILE * f, char *header, int nheader)
{
  int w = lum_pickshort (header + 2);
  int h = lum_pickshort (header + 6);
  while (nheader++ < 0xf94)
    pilla_caracter_segur (f);
  float *data = xmalloc (w * h * sizeof *data);
  if (1 != fread (data, w * h * 4, 1, f))
    return -1;
  switch_4endianness (data, w * h);
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 1;
  x->type = IIO_TYPE_FLOAT;
  x->contiguous_data = false;
  x->data = data;
  return 0;
}

// BMP reader {{{2
static int
read_beheaded_bmp (struct iio_image *x, FILE * f, char *header, int nheader)
{
  long len;
  char *bmp = load_rest_of_file (&len, f, header, nheader);
  uint32_t pix_offset = *(uint32_t *) (bmp + 0xa);

  error ("BMP reader not yet finished");

  xfree (bmp);

  //x->dimension = 2;
  //x->sizes[0] = dib_width;
  //x->sizes[1] = dib_height;
  //x->pixel_dimension = 1;
  //x->type = IIO_TYPE_FLOAT;
  //x->contiguous_data = false;
  //x->data = data;
  return 0;
  //fprintf(stderr, "bmp reader\n");
  //uint32_t aoffset = *(uint32_t*)(header+0xa);
  //fprintf(stderr, "aoffset = %d\n", aoffset);
  //assert(nheader == 14); // ignore the file header
  //uint32_t dib_size = rim_getint(f, false);
  //fprintf(stderr, "dib_size = %d\n", dib_size);
  //if (dib_size != 40) error("only windows-like bitmaps ara supported");
  //int32_t dib_width = rim_getint(f, false);
  //int32_t dib_height = rim_getint(f, false);
  //uint16_t dib_planes = rim_getshort(f, true);
  //uint16_t dib_bpp = rim_getshort(f, true);
  //uint32_t dib_compression = rim_getint(f, false);
  //fprintf(stderr, "w,h = %dx%d, bpp=%d (p=%d), compression=%d\n",
  //              dib_width, dib_height,
  //              dib_bpp, dib_planes, dib_compression);
  ////if (dib_planes != 1) error("BMP READER: only one plane is supported (not %d)", dib_planes);
  //if (dib_compression) error("compressed BMP are not supported");
  //uint32_t dib_imsize = rim_getint(f, false);
  //int32_t dib_hres = rim_getint(f, false);
  //int32_t dib_vres = rim_getint(f, false);
  //uint32_t dib_ncolors = rim_getint(f, false);
  //uint32_t dib_nicolors = rim_getint(f, false);
  //fprintf(stderr, "ncolors = %d\n", dib_ncolors);
  //error("fins aquí hem arribat!");
}


// EXR reader {{{2

#ifdef I_CAN_HAS_LIBEXR
#include <ImfCRgbaFile.h>
// EXTERNALIZED TO :  read_exr_float.cpp
//extern int read_whole_exr(struct iio_image *x, char *filename);

static int
read_whole_exr (struct iio_image *x, char *filename)
{
  struct ImfInputFile *f = ImfOpenInputFile (filename);
  if (!f)
    error ("could not read exr from %s", filename);
  int r;

  const char *nom = ImfInputFileName (f);
  IIO_DEBUG ("ImfInputFileName returned %s\n", nom);

  r = ImfInputChannels (f);
  IIO_DEBUG ("ImfInputChannels returned %d\n", r);
  // this data is only for information.  The file is always
  // converted to RGB
  //if (r != IMF_WRITE_RGBA)
  //      error("only RGBA EXR supported so far (got %d)", r);

  const struct ImfHeader *header = ImfInputHeader (f);
  int xmin, ymin, xmax, ymax;
  ImfHeaderDataWindow (header, &xmin, &ymin, &xmax, &ymax);
  IIO_DEBUG ("xmin ymin xmax ymax = %d %d %d %d\n", xmin, ymin, xmax, ymax);

  int width = xmax - xmin + 1;
  int height = ymax - ymin + 1;
  struct ImfRgba *data = xmalloc (width * height * sizeof *data);

  r = ImfInputSetFrameBuffer (f, data, 1, width);
  IIO_DEBUG ("ImfInputSetFrameBuffer returned %d\n", r);

  r = ImfInputReadPixels (f, ymin, ymax);
  IIO_DEBUG ("ImfInputReadPixels returned %d\n", r);

  r = ImfCloseInputFile (f);
  IIO_DEBUG ("ImfCloseInputFile returned %d\n", r);

  float *finaldata = xmalloc (4 * width * height * sizeof *data);
  FORI (width * height)
  {
    finaldata[4 * i + 0] = ImfHalfToFloat (data[i].r);
    finaldata[4 * i + 1] = ImfHalfToFloat (data[i].g);
    finaldata[4 * i + 2] = ImfHalfToFloat (data[i].b);
    finaldata[4 * i + 3] = ImfHalfToFloat (data[i].a);
    //IIO_DEBUG("read %d rgb %g %g %g\n", i, finaldata[3*i+0], finaldata[3*i+1], finaldata[3*i+2]);
  }
  xfree (data);

  // fill struct fields
  x->dimension = 2;
  x->sizes[0] = width;
  x->sizes[1] = height;
  x->pixel_dimension = 4;
  x->type = IIO_TYPE_FLOAT;
  x->format = x->meta = -42;
  x->data = finaldata;
  x->contiguous_data = false;
  return 0;
}

// Note: OpenEXR library only reads from a named file.  The only way to use the
// library if you have your image in a stream, is to write the data to a
// temporary file, and read it from there.  This can be optimized in the future
// by recovering the original filename through hacks, in case it was not a pipe.
static int
read_beheaded_exr (struct iio_image *x, FILE * fin, char *header, int nheader)
{
  if (global_variable_containing_the_name_of_the_last_opened_file)
    {
      int r = read_whole_exr (x,
                              global_variable_containing_the_name_of_the_last_opened_file);
      if (r)
        error ("read whole tiff returned %d", r);
      return 0;
    }

  long filesize;
  void *filedata = load_rest_of_file (&filesize, fin, header, nheader);
  char *filename = put_data_into_temporary_file (filedata, filesize);
  xfree (filedata);
  //IIO_DEBUG("tmpfile = \"%s\"\n", filename);


  int r = read_whole_exr (x, filename);
  if (r)
    error ("read whole exr returned %d", r);

  delete_temporary_file (filename);

  return 0;
}

#endif //I_CAN_HAS_LIBEXR

// WHATEVER reader {{{2

//static int read_image(struct iio_image*, const char *);
static int read_image_f (struct iio_image *, FILE *);
static int
read_beheaded_whatever (struct iio_image *x,
                        FILE * fin, char *header, int nheader)
{
  // dump data to file
  long filesize;
  void *filedata = load_rest_of_file (&filesize, fin, header, nheader);
  char *filename = put_data_into_temporary_file (filedata, filesize);
  xfree (filedata);
  //IIO_DEBUG("tmpfile = \"%s\"\n", filename);

  //char command_format[] = "convert - %s < %s\0";
  char command_format[] = "/usr/bin/convert - %s < %s\0";
  char ppmname[strlen (filename) + 5];
  sprintf (ppmname, "%s.ppm", filename);
  char command[strlen (command_format) + 1 + 2 * strlen (filename)];
  sprintf (command, command_format, ppmname, filename);
  IIO_DEBUG ("COMMAND: %s\n", command);
  int r = system (command);
  IIO_DEBUG ("command returned %d\n", r);
  if (r)
    error ("could not run command \"%s\" successfully", command);

  FILE *f = xfopen (ppmname, "r");
  r = read_image_f (x, f);
  xfclose (f);

  delete_temporary_file (filename);
  delete_temporary_file (ppmname);

  return r;
}

// individual format writers {{{1
// PNG writer {{{2

#ifdef I_CAN_HAS_LIBPNG

static void
iio_save_image_as_png (const char *filename, struct iio_image *x)
{
  png_structp pp = png_create_write_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (!pp)
    error ("png_create_write_struct fail");
  png_infop pi = png_create_info_struct (pp);
  if (!pi)
    error ("png_create_info_struct fail");
  if (setjmp (png_jmpbuf (pp)))
    error ("png write error");

  if (x->dimension != 2)
    error ("can only save 2d images");
  int width = x->sizes[0];
  int height = x->sizes[1];
  int bit_depth = 0;
  switch (normalize_type (x->type))
    {
    case IIO_TYPE_UINT16:
    case IIO_TYPE_INT16:
      bit_depth = 16;
      break;
    case IIO_TYPE_INT8:
    case IIO_TYPE_UINT8:
      bit_depth = 8;
      break;
    default:
      error ("can not yet save samples of type %s as PNG",
             iio_strtyp (x->type));
    }
  assert (bit_depth > 0);
  int color_type = PNG_COLOR_TYPE_PALETTE;
  switch (x->pixel_dimension)
    {
    case 1:
      color_type = PNG_COLOR_TYPE_GRAY;
      break;
    case 2:
      color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
      break;
    case 3:
      color_type = PNG_COLOR_TYPE_RGB;
      break;
    case 4:
      color_type = PNG_COLOR_TYPE_RGB_ALPHA;
      break;
    default:
      error ("can not save %d-dimensional samples as PNG",
             x->pixel_dimension);
    }
  assert (color_type != PNG_COLOR_TYPE_PALETTE);

  FILE *f = xfopen (filename, "w");
  png_init_io (pp, f);

  int ss = bit_depth / 8;
  int pd = x->pixel_dimension;
  png_bytepp row = xmalloc (height * sizeof (png_bytep));
  FORI (height) row[i] = i * pd * width * ss + (uint8_t *) x->data;
  png_set_IHDR (pp, pi, width, height, bit_depth, color_type,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
  png_set_rows (pp, pi, row);
  int transforms = PNG_TRANSFORM_IDENTITY;
  png_write_png (pp, pi, transforms, NULL);
  xfclose (f);
  png_destroy_write_struct (&pp, &pi);
  xfree (row);
}

#endif //I_CAN_HAS_LIBPNG

// TIFF writer {{{2

#ifdef I_CAN_HAS_LIBTIFF

static void
iio_save_image_as_tiff (const char *filename, struct iio_image *x)
{
  //fprintf(stderr, "saving image as tiff file \"%s\"\n", filename);
  if (x->dimension != 2)
    error ("only 2d images can be saved as TIFFs");
  TIFF *tif = TIFFOpen (filename, "w");
  if (!tif)
    error ("could not open TIFF file \"%s\"", filename);

  int ss = iio_image_sample_size (x);
  int sls = x->sizes[0] * x->pixel_dimension * ss;
  int tsf;

  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, x->sizes[0]);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, x->sizes[1]);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, x->pixel_dimension);
  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, ss * 8);
  uint16 caca[1] = { EXTRASAMPLE_UNASSALPHA };
  switch (x->pixel_dimension)
    {
    case 1:
      TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
      break;
    case 3:
      TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      break;
    case 2:
      TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
      //TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, caca);
      break;
    case 4:
      TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, caca);
      break;
    default:
      TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
      //error("bad pixel dimension %d for TIFF", x->pixel_dimension);
    }
  /////TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
  //TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  switch (x->type)
    {
    case IIO_TYPE_FLOAT:
      tsf = SAMPLEFORMAT_IEEEFP;
      break;
    case IIO_TYPE_INT8:
    case IIO_TYPE_INT16:
    case IIO_TYPE_INT32:
      tsf = SAMPLEFORMAT_INT;
      break;
    case IIO_TYPE_UINT8:
    case IIO_TYPE_UINT16:
    case IIO_TYPE_UINT32:
      tsf = SAMPLEFORMAT_UINT;
      break;
    default:
      error ("can not save samples of type %s on tiff file",
             iio_strtyp (x->type));
    }
  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, tsf);

  FORI (x->sizes[1])
  {
    void *line = i * sls + (char *) x->data;
    int r = TIFFWriteScanline (tif, line, i, 0);
    if (r < 0)
      error ("error writing %dth TIFF scanline", i);
  }

  TIFFClose (tif);
}

static void
iio_save_image_as_tiff_smarter (const char *filename, struct iio_image *x)
{
  char *tiffname = strstr (filename, "TIFF:");
  if (tiffname == filename)
    {
      iio_save_image_as_tiff_smarter (filename + 5, x);
      return;
    }
  if (0 == strcmp (filename, "-"))
    {
#ifdef I_CAN_HAS_MKSTEMP
      static char tfn[] = "/tmp/iio_temporal_tiff_XXXXXX\0";
      int r = mkstemp (tfn);
      if (r == -1)
        error ("caca [tiff smarter]");
#else
      static char buf[L_tmpnam + 1];
      char *tfn = tmpnam (buf);
#endif //I_CAN_HAS_MKSTEMP
      iio_save_image_as_tiff (tfn, x);
      FILE *f = xfopen (tfn, "r");
      int c;
      while ((c = fgetc (f)) != EOF)
        fputc (c, stdout);
      fclose (f);
      delete_temporary_file (tfn);
    }
  else
    iio_save_image_as_tiff (filename, x);
}

#endif //I_CAN_HAS_LIBTIFF


// JUV writer {{{2
static void
iio_save_image_as_juv (const char *filename, struct iio_image *x)
{
  assert (x->type == IIO_TYPE_FLOAT);
  assert (x->dimension == 2);
  char buf[255];
  FORI (255) buf[i] = ' ';
  snprintf (buf, 255, "#UV {\n dimx %d dimy %d\n}\n",
            x->sizes[0], x->sizes[1]);
  size_t sf = sizeof (float);
  int w = x->sizes[0];
  int h = x->sizes[1];
  float *uv = x->data;
  float *u = xmalloc (w * h * sf);
  FORI (w * h) u[i] = uv[2 * i];
  float *v = xmalloc (w * h * sf);
  FORI (w * h) v[i] = uv[2 * i + 1];
  FILE *f = xfopen (filename, "w");
  fwrite (buf, 1, 255, f);
  fwrite (u, sf, w * h, f);
  fwrite (v, sf, w * h, f);
  xfclose (f);
  xfree (u);
  xfree (v);
}


// FLO writer {{{2
static void
iio_save_image_as_flo (const char *filename, struct iio_image *x)
{
  assert (x->type == IIO_TYPE_FLOAT);
  assert (x->dimension == 2);
  union
  {
    char s[4];
    float f;
  } pieh =
  {
  "PIEH"};
  assert (sizeof (float) == 4);
  assert (pieh.f == 202021.25);
  uint32_t w = x->sizes[0];
  uint32_t h = x->sizes[1];
  FILE *f = xfopen (filename, "w");
  fwrite (&pieh.f, 4, 1, f);
  fwrite (&w, 4, 1, f);
  fwrite (&h, 4, 1, f);
  fwrite (x->data, 4, w * h * 2, f);
  xfclose (f);
}

// RIM writer {{{2

static void
rim_putshort (FILE * f, uint16_t n)
{
  int a = n % 0x100;
  int b = n / 0x100;
  assert (a >= 0);
  assert (b >= 0);
  assert (a < 256);
  assert (b < 256);
  fputc (b, f);
  fputc (a, f);
}

static void
iio_save_image_as_rim_fimage (const char *fname, struct iio_image *x)
{
  if (x->type != IIO_TYPE_FLOAT)
    error ("fimage expects float data");
  if (x->dimension != 2)
    error ("fimage expects 2d image");
  if (x->pixel_dimension != 1)
    error ("fimage expects gray image");
  FILE *f = xfopen (fname, "w");
  fputc ('I', f);
  fputc ('R', f);
  rim_putshort (f, 2);
  rim_putshort (f, x->sizes[0]);
  rim_putshort (f, x->sizes[1]);
  FORI (29) rim_putshort (f, 0);
  int r = fwrite (x->data, sizeof (float), x->sizes[0] * x->sizes[1], f);
  if (r != x->sizes[0] * x->sizes[1])
    error ("could not write an entire fimage for some reason");
  xfclose (f);
}

static void
iio_save_image_as_rim_cimage (const char *fname, struct iio_image *x)
{
  if (x->type != IIO_TYPE_UINT8)
    error ("cimage expects byte data");
  if (x->dimension != 2)
    error ("cimage expects 2d image");
  if (x->pixel_dimension != 1)
    error ("cimage expects gray image");
  FILE *f = xfopen (fname, "w");
  fputc ('M', f);
  fputc ('I', f);
  rim_putshort (f, 2);
  rim_putshort (f, x->sizes[0]);
  rim_putshort (f, x->sizes[1]);
  FORI (29) rim_putshort (f, 0);
  int r = fwrite (x->data, 1, x->sizes[0] * x->sizes[1], f);
  if (r != x->sizes[0] * x->sizes[1])
    error ("could not write an entire cimage for some reason");
  xfclose (f);
}

// guess format using magic {{{1


static char
add_to_header_buffer (FILE * f, uint8_t * buf, int *nbuf, int bufmax)
{
  int c = pilla_caracter_segur (f);
  if (*nbuf >= bufmax)
    error ("buffer header too small (%d)", bufmax);
  buf[*nbuf] = c;               //iw810
  IIO_DEBUG ("ATHB[%d] = %x \"%c\"\n", *nbuf, c, c);
  *nbuf += 1;
  return c;
}

static int
guess_format (FILE * f, char *buf, int *nbuf, int bufmax)
{
  assert (sizeof (uint8_t) == sizeof (char));
  uint8_t *b = (uint8_t *) buf;
  *nbuf = 0;

  //
  // program a state machine here
  //

  b[0] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[1] = add_to_header_buffer (f, b, nbuf, bufmax);
  if (b[0] == 'P' || b[0] == 'Q')
    if (b[1] >= '1' && b[1] <= '9')
      return IIO_FORMAT_QNM;

  if (b[0] == 'P' && (b[1] == 'F' || b[1] == 'f'))
    return IIO_FORMAT_PFM;

#ifdef I_CAN_HAS_LIBTIFF
  if ((b[0] == 'M' && buf[1] == 'M') || (b[0] == 'I' && buf[1] == 'I'))
    return IIO_FORMAT_TIFF;
#endif //I_CAN_HAS_LIBTIFF

  if (b[0] == 'I' && b[1] == 'R')
    return IIO_FORMAT_RIM;
  if (b[0] == 'R' && b[1] == 'I')
    return IIO_FORMAT_RIM;
  if (b[0] == 'M' && b[1] == 'I')
    return IIO_FORMAT_RIM;
  if (b[0] == 'I' && b[1] == 'M')
    return IIO_FORMAT_RIM;
  if (b[0] == 'W' && b[1] == 'E')
    return IIO_FORMAT_RIM;
  if (b[0] == 'V' && b[1] == 'I')
    return IIO_FORMAT_RIM;

  if (b[0] == 'P' && b[1] == 'C')
    return IIO_FORMAT_PCM;

  if (b[0] == 'B' && b[1] == 'M')
    {
      FORI (12) add_to_header_buffer (f, b, nbuf, bufmax);
      return IIO_FORMAT_BMP;
    }



  b[2] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[3] = add_to_header_buffer (f, b, nbuf, bufmax);

#ifdef I_CAN_HAS_LIBPNG
  if (b[1] == 'P' && b[2] == 'N' && b[3] == 'G')
    return IIO_FORMAT_PNG;
#endif //I_CAN_HAS_LIBPNG

#ifdef I_CAN_HAS_LIBEXR
  if (b[0] == 0x76 && b[1] == 0x2f && b[2] == 0x31 && b[3] == 0x01)
    return IIO_FORMAT_EXR;
#endif //I_CAN_HAS_LIBEXR

  if (b[0] == '#' && b[1] == 'U' && b[2] == 'V')
    return IIO_FORMAT_JUV;

  if (b[0] == 'P' && b[1] == 'I' && b[2] == 'E' && b[3] == 'H')
    return IIO_FORMAT_FLO;

  b[4] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[5] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[6] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[7] = add_to_header_buffer (f, b, nbuf, bufmax);

#ifdef I_CAN_HAS_LIBJPEG
  if (b[0] == 0xff && b[1] == 0xd8 && b[2] == 0xff)
    {
      if (b[3] == 0xe0 && b[6] == 'J' && b[7] == 'F')
        return IIO_FORMAT_JPEG;
      if (b[3] == 0xe1 && b[6] == 'E' && b[7] == 'x')
        return IIO_FORMAT_JPEG;
    }
#endif //I_CAN_HAS_LIBPNG

  b[8] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[9] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[10] = add_to_header_buffer (f, b, nbuf, bufmax);
  b[11] = add_to_header_buffer (f, b, nbuf, bufmax);

  if (b[8] == 'F' && b[9] == 'L' && b[10] == 'O' && b[11] == 'A')
    return IIO_FORMAT_LUM;

  return IIO_FORMAT_UNRECOGNIZED;
}

// dispatcher {{{1

// "centralized dispatcher"
static int
read_beheaded_image (struct iio_image *x, FILE * f, char *h, int hn, int fmt)
{
  IIO_DEBUG ("rbi fmt = %d\n", fmt);
  // these functions can be defined in separate, independent files
  switch (fmt)
    {
    case IIO_FORMAT_QNM:
      return read_beheaded_qnm (x, f, h, hn);
    case IIO_FORMAT_RIM:
      return read_beheaded_rim (x, f, h, hn);
    case IIO_FORMAT_PFM:
      return read_beheaded_pfm (x, f, h, hn);
    case IIO_FORMAT_FLO:
      return read_beheaded_flo (x, f, h, hn);
    case IIO_FORMAT_JUV:
      return read_beheaded_juv (x, f, h, hn);
    case IIO_FORMAT_LUM:
      return read_beheaded_lum (x, f, h, hn);
    case IIO_FORMAT_PCM:
      return read_beheaded_pcm (x, f, h, hn);
    case IIO_FORMAT_BMP:
      return read_beheaded_bmp (x, f, h, hn);

#ifdef I_CAN_HAS_LIBPNG
    case IIO_FORMAT_PNG:
      return read_beheaded_png (x, f, h, hn);
#endif

#ifdef I_CAN_HAS_LIBJPEG
    case IIO_FORMAT_JPEG:
      return read_beheaded_jpeg (x, f, h, hn);
#endif

#ifdef I_CAN_HAS_LIBTIFF
    case IIO_FORMAT_TIFF:
      return read_beheaded_tiff (x, f, h, hn);
#endif

#ifdef I_CAN_HAS_LIBEXR
    case IIO_FORMAT_EXR:
      return read_beheaded_exr (x, f, h, hn);
#endif

      /*
         case IIO_FORMAT_JP2:   return read_beheaded_jp2 (x, f, h, hn);
         case IIO_FORMAT_VTK:   return read_beheaded_vtk (x, f, h, hn);
         case IIO_FORMAT_CIMG:  return read_beheaded_cimg(x, f, h, hn);
         case IIO_FORMAT_PAU:   return read_beheaded_pau (x, f, h, hn);
         case IIO_FORMAT_DICOM: return read_beheaded_dicom(x, f, h, hn);
         case IIO_FORMAT_NIFTI: return read_beheaded_nifti(x, f, h, hn);
         case IIO_FORMAT_PCX:   return read_beheaded_pcx (x, f, h, hn);
         case IIO_FORMAT_GIF:   return read_beheaded_gif (x, f, h, hn);
         case IIO_FORMAT_XPM:   return read_beheaded_xpm (x, f, h, hn);
         case IIO_FORMAT_RAFA:   return read_beheaded_rafa (x, f, h, hn);
       */

//#ifdef I_CAN_HAS_WHATEVER
    case IIO_FORMAT_UNRECOGNIZED:
      return read_beheaded_whatever (x, f, h, hn);
//#else
//      case IIO_FORMAT_UNRECOGNIZED: return -2;
//#endif

    default:
      return -17;
    }
}





// general image reader {{{1



//
// This function is the core of the library.
// Everything passes through here.
//
static int
read_image_f (struct iio_image *x, FILE * f)
{
  int bufmax = 0x100, nbuf, format;
  char buf[bufmax];
  format = guess_format (f, buf, &nbuf, bufmax);
  IIO_DEBUG ("iio file format guess: %s {%d}\n", iio_strfmt (format), nbuf);
  assert (nbuf > 0);
  return read_beheaded_image (x, f, buf, nbuf, format);
}

static int
read_image (struct iio_image *x, const char *fname)
{
#ifndef IIO_ABORT_ON_ERROR
  if (setjmp (global_jump_buffer))
    {
      IIO_DEBUG ("SOME ERROR HAPPENED AND WAS HANDLED\n");
      return 1;
    }
  //if (iio_single_jmpstuff(false, true)) {
  //      IIO_DEBUG("SOME ERROR HAPPENED AND WAS HANDLED\n");
  //      exit(42);
  //}
#endif //IIO_ABORT_ON_ERROR
  FILE *f = xfopen (fname, "r");
  int r = read_image_f (x, f);
  fclose (f);

  IIO_DEBUG ("READ IMAGE return value = %d\n", r);
  IIO_DEBUG ("READ IMAGE dimension = %d\n", x->dimension);
  switch (x->dimension)
    {
    case 1:
      IIO_DEBUG ("READ IMAGE sizes = %d\n", x->sizes[0]);
      break;
    case 2:
      IIO_DEBUG ("READ IMAGE sizes = %d x %d\n", x->sizes[0], x->sizes[1]);
      break;
    case 3:
      IIO_DEBUG ("READ IMAGE sizes = %d x %d x %d\n", x->sizes[0],
                 x->sizes[1], x->sizes[2]);
      break;
    case 4:
      IIO_DEBUG ("READ IMAGE sizes = %d x %d x %d x %d\n", x->sizes[0],
                 x->sizes[1], x->sizes[2], x->sizes[3]);
      break;
    default:
      error ("caca [dimension = %d]", x->dimension);
    }
  //FORI(x->dimension) IIO_DEBUG(" %d", x->sizes[i]);
  //IIO_DEBUG("\n");
  IIO_DEBUG ("READ IMAGE pixel_dimension = %d\n", x->pixel_dimension);
  IIO_DEBUG ("READ IMAGE type = %s\n", iio_strtyp (x->type));
  //IIO_DEBUG("READ IMAGE meta = %d\n", x->meta);
  //IIO_DEBUG("READ IMAGE format = %d\n", x->format);
  IIO_DEBUG ("READ IMAGE contiguous_data = %d\n", x->contiguous_data);

  return r;
}


static void iio_save_image_default (const char *filename,
                                    struct iio_image *x);



// API (input) {{{1

static void *
rerror (const char *fmt, ...)
{
#ifdef IIO_ABORT_ON_ERROR
  va_list argp;
  va_start (argp, fmt);
  error (fmt, argp);
  va_end (argp);
#endif
  return NULL;
}

// 2D only
static void *
iio_read_image (const char *fname, int *w, int *h, int desired_sample_type)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      //error("non 2d image");
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, desired_sample_type);
  return x->data;
}

// API 2D
float *
iio_read_image_float_vec (const char *fname, int *w, int *h, int *pd)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      //error("non 2d image");
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  *pd = x->pixel_dimension;
  iio_convert_samples (x, IIO_TYPE_FLOAT);
  return x->data;
}

// API 2D
float *
iio_read_image_float_split (const char *fname, int *w, int *h, int *pd)
{
  float *r = iio_read_image_float_vec (fname, w, h, pd);
  float *rbroken = xmalloc (*w ** h ** pd * sizeof *rbroken);
  break_pixels_float (rbroken, r, *w ** h, *pd);
  xfree (r);
  return rbroken;
}

// API 2D
float *
iio_read_image_float_rgb (const char *fname, int *w, int *h)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  if (x->pixel_dimension != 3)
    {
      iio_hacky_colorize (x, 3);
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_FLOAT);
  return x->data;
}

// API 2D
double *
iio_read_image_double_vec (const char *fname, int *w, int *h, int *pd)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      //error("non 2d image");
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  *pd = x->pixel_dimension;
  iio_convert_samples (x, IIO_TYPE_DOUBLE);
  return x->data;
}

// API 2D
uint8_t *
iio_read_image_uint8_vec (const char *fname, int *w, int *h, int *pd)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  *pd = x->pixel_dimension;
  iio_convert_samples (x, IIO_TYPE_UINT8);
  return x->data;
}

// API 2D
uint16_t *
iio_read_image_uint16_vec (const char *fname, int *w, int *h, int *pd)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  *pd = x->pixel_dimension;
  iio_convert_samples (x, IIO_TYPE_UINT16);
  return x->data;
}

// API 2D
uint8_t (*iio_read_image_uint8_rgb (const char *fname, int *w, int *h))[3]
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      error ("non 2d image");
    }
  if (x->pixel_dimension != 3)
    error ("non-color image");
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_UINT8);
  return x->data;
}

// API 2D
uint8_t (**iio_read_image_uint8_matrix_rgb (const char *fnam, int *w, int *h))
  [3]
{
  struct iio_image x[1];
  int r = read_image (x, fnam);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  if (x->pixel_dimension != 3)
    {
      iio_hacky_colorize (x, 3);
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_UINT8);
  return wrap_2dmatrix_around_data (x->data, *w, *h, 3);
}

// API 2D
float (**iio_read_image_float_matrix_rgb (const char *fnam, int *w, int *h))
  [3]
{
  struct iio_image x[1];
  int r = read_image (x, fnam);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  if (x->pixel_dimension != 3)
    {
      iio_hacky_colorize (x, 3);
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_FLOAT);
  return wrap_2dmatrix_around_data (x->data, *w, *h, 3 * sizeof (float));
}

// API 2D
uint8_t ***
iio_read_image_uint8_matrix_vec (const char *fname, int *w, int *h, int *pd)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  *pd = x->pixel_dimension;
  fprintf (stderr, "matrix_vec pd = %d\n", *pd);
  iio_convert_samples (x, IIO_TYPE_UINT8);
  return wrap_2dmatrix_around_data (x->data, *w, *h, *pd);
}

// API 2D
void *
iio_read_image_float_matrix_vec (const char *fname, int *w, int *h, int *pd)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  *w = x->sizes[0];
  *h = x->sizes[1];
  *pd = x->pixel_dimension;
  iio_convert_samples (x, IIO_TYPE_FLOAT);
  return wrap_2dmatrix_around_data (x->data, *w, *h, *pd * sizeof (float));
}

// API 2D
uint8_t **
iio_read_image_uint8_matrix (const char *fname, int *w, int *h)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  if (x->pixel_dimension == 3)
    iio_hacky_uncolorize (x);
  if (x->pixel_dimension != 1)
    error ("non-scalar image");
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_UINT8);
  return wrap_2dmatrix_around_data (x->data, *w, *h, 1);
  //return x->data;

  // WRONG!:
  //uint8_t *f = iio_read_image_uint8(fname, w, h);
  //uint8_t **a = wrap_2dmatrix_around_data(f, *w, *h, sizeof*f);
  //return a;
}

float **
iio_read_image_float_matrix (const char *fname, int *w, int *h)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  if (x->pixel_dimension == 3)
    iio_hacky_uncolorize (x);
  if (x->pixel_dimension != 1)
    return rerror ("non-scalar image");
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_FLOAT);
  return wrap_2dmatrix_around_data (x->data, *w, *h, sizeof (float));
}

// API nd general
void *
iio_read_nd_image_as_stored (char *fname,
                             int *dimension, int *sizes,
                             int *samples_per_pixel, int *sample_size,
                             bool * ieefp_samples, bool * signed_samples)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("so much fail");
  *dimension = x->dimension;
  FORI (x->dimension) sizes[i] = x->sizes[i];
  *samples_per_pixel = x->pixel_dimension;
  iio_type_unid (sample_size, ieefp_samples, signed_samples, x->type);
  return x->data;
}

// API nd general
void *
iio_read_nd_image_as_desired (char *fname,
                              int *dimension, int *sizes,
                              int *samples_per_pixel, int desired_sample_size,
                              bool desired_ieeefp_samples,
                              bool desired_signed_samples)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("so much fail");
  int desired_type = iio_type_id (desired_sample_size,
                                  desired_ieeefp_samples,
                                  desired_signed_samples);
  iio_convert_samples (x, desired_type);
  *dimension = x->dimension;
  FORI (x->dimension) sizes[i] = x->sizes[i];
  *samples_per_pixel = x->pixel_dimension;
  return x->data;
}

//// API 2D general
//void *iio_read_image_numbers_as_they_are_stored(char *fname, int *w, int *h,
//              int *samples_per_pixel, int *sample_size,
//              bool *ieeefp_samples, bool *signed_samples)
//{
//}

// API 2D
float *
iio_read_image_float (const char *fname, int *w, int *h)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  if (x->pixel_dimension == 3)
    iio_hacky_uncolorize (x);
  if (x->pixel_dimension == 4)
    iio_hacky_uncolorizea (x);
  if (x->pixel_dimension != 1)
    return rerror ("non-scalarizable image");
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_FLOAT);
  return x->data;
}

// API 2D
double *
iio_read_image_double (const char *fname, int *w, int *h)
{
  struct iio_image x[1];
  int r = read_image (x, fname);
  if (r)
    return rerror ("could not read image");
  if (x->dimension != 2)
    {
      x->dimension = 2;
      return rerror ("non 2d image");
    }
  if (x->pixel_dimension == 3)
    iio_hacky_uncolorize (x);
  if (x->pixel_dimension != 1)
    return rerror ("non-scalar image");
  *w = x->sizes[0];
  *h = x->sizes[1];
  iio_convert_samples (x, IIO_TYPE_DOUBLE);
  return x->data;
}

//// API 2D
//char *iio_read_image_char(const char *fname, int *w, int *h)
//{
//      return iio_read_image(fname, w, h, IIO_TYPE_CHAR);
//}

// API 2D
uint8_t *
iio_read_image_uint8 (const char *fname, int *w, int *h)
{
  return iio_read_image (fname, w, h, IIO_TYPE_UINT8);
}

// API 2D
//float **iio_read_image_float_matrix(const char *fname, int *w, int *h)
//{
//      float *f = iio_read_image_float(fname, w, h);
//      float **a = wrap_2dmatrix_around_data(f, *w, *h, sizeof*f);
//      return a;
//}




// API (output) {{{1

static bool
this_float_is_actually_a_byte (float x)
{
  return (x == floorf (x)) && (x >= 0) && (x < 256);
}

static bool
these_floats_are_actually_bytes (float *t, int n)
{
  IIO_DEBUG ("checking %d floats for byteness (%p)\n", n, (void *) t);
  FORI (n) if (!this_float_is_actually_a_byte (t[i]))
    return false;
  return true;
}

//// returns the un-prefixed string, or NULL
//static const char *hasprefix(const char *haystack, const char *needle)
//{
//      const char *r = strstr(haystack, needle);
//      if (r == haystack)
//              return r + strlen(needle);
//      else
//              return NULL;
//}
//
//// returns the first argument, or NULL
//static const char *hassufix(const char *haystack, const char *needle)
//{
//      const char *r =
//}

static bool
string_suffix (const char *s, const char *suf)
{
  int len_s = strlen (s);
  int len_suf = strlen (suf);
  if (len_s < len_suf)
    return false;
  return 0 == strcmp (suf, s + (len_s - len_suf));
}

// Note:
// This function was written without being designed.  See file "saving.txt" for
// an attempt at designing it.
static void
iio_save_image_default (const char *filename, struct iio_image *x)
{
  int typ = normalize_type (x->type);
  if (x->dimension != 2)
    error ("de moment només escrivim 2D");
  //static bool silly = true;
  if (string_suffix (filename, ".uv") && typ == IIO_TYPE_FLOAT
      && x->pixel_dimension == 2)
    {
      iio_save_image_as_juv (filename, x);
      return;
    }
  if (string_suffix (filename, ".flo") && typ == IIO_TYPE_FLOAT
      && x->pixel_dimension == 2)
    {
      iio_save_image_as_flo (filename, x);
      return;
    }
  if (string_suffix (filename, ".mw") && typ == IIO_TYPE_FLOAT
      && x->pixel_dimension == 1)
    {
      iio_save_image_as_rim_fimage (filename, x);
      return;
    }
  if (string_suffix (filename, ".mw") && typ == IIO_TYPE_UINT8
      && x->pixel_dimension == 1)
    {
      iio_save_image_as_rim_cimage (filename, x);
      return;
    }
  if (x->pixel_dimension != 1 && x->pixel_dimension != 3
      && x->pixel_dimension != 4 && x->pixel_dimension != 2)
    {
      iio_save_image_as_tiff_smarter (filename, x);
      return;
      //error("de moment només escrivim gris ó RGB");
    }
  if (typ != IIO_TYPE_FLOAT && typ != IIO_TYPE_UINT8 && typ != IIO_TYPE_INT16
      && typ != IIO_TYPE_INT8)
    error ("de moment només fem floats o bytes (got %d)", typ);
  int nsamp = iio_image_number_of_samples (x);
  if (typ == IIO_TYPE_FLOAT &&
      these_floats_are_actually_bytes (x->data, nsamp))
    {
      void *old_data = x->data;
      x->data = xmalloc (nsamp * sizeof (float));
      memcpy (x->data, old_data, nsamp * sizeof (float));
      iio_convert_samples (x, IIO_TYPE_UINT8);
      //silly=false;
      iio_save_image_default (filename, x);     // recursive call
      //silly=true;
      xfree (x->data);
      x->data = old_data;
      return;
    }
  if (true)
    {
      if (false
          || string_suffix (filename, ".tiff")
          || string_suffix (filename, ".tif")
          || string_suffix (filename, ".TIFF")
          || string_suffix (filename, ".TIF"))
        {
          iio_save_image_as_tiff_smarter (filename, x);
          return;
        }
    }
  if (true)
    {
      char *tiffname = strstr (filename, "TIFF:");
      if (tiffname == filename)
        {
          iio_save_image_as_tiff_smarter (filename + 5, x);
          return;
        }
    }
  if (true)
    {
      char *pngname = strstr (filename, "PNG:");
      if (pngname == filename)
        {
          if (typ == IIO_TYPE_FLOAT)
            {
              void *old_data = x->data;
              x->data = xmalloc (nsamp * sizeof (float));
              memcpy (x->data, old_data, nsamp * sizeof (float));
              iio_convert_samples (x, IIO_TYPE_UINT8);
              iio_save_image_default (filename, x);     //recursive
              xfree (x->data);
              x->data = old_data;
              return;
            }
          iio_save_image_as_png (filename + 4, x);
          return;
        }
    }
  if (true)
    {
      if (false
          || string_suffix (filename, ".png")
          || string_suffix (filename, ".PNG")
          || (typ == IIO_TYPE_UINT8 && x->pixel_dimension == 4)
          //      || (typ==IIO_TYPE_FLOAT&&x->pixel_dimension==4)
          || (typ == IIO_TYPE_UINT8 && x->pixel_dimension == 2)
          //      || (typ==IIO_TYPE_FLOAT&&x->pixel_dimension==2)
        )
        {
          if (typ == IIO_TYPE_FLOAT)
            {
              void *old_data = x->data;
              x->data = xmalloc (nsamp * sizeof (float));
              memcpy (x->data, old_data, nsamp * sizeof (float));
              iio_convert_samples (x, IIO_TYPE_UINT8);
              iio_save_image_default (filename, x);     //recursive
              xfree (x->data);
              x->data = old_data;
              return;
            }
          iio_save_image_as_png (filename, x);
          return;
        }
    }
  IIO_DEBUG ("SIDEF:\n");
#ifdef IIO_SHOW_DEBUG_MESSAGES
  iio_print_image_info (stderr, x);
#endif
  FILE *f = xfopen (filename, "w");
  if (x->pixel_dimension == 1 && typ == IIO_TYPE_FLOAT)
    {
      int m =
        these_floats_are_actually_bytes (x->data,
                                         x->sizes[0] *
                                         x->sizes[1]) ? 255 : 65535;
      fprintf (f, "P2\n%d %d\n%d\n", x->sizes[0], x->sizes[1], m);
      float *data = x->data;
      FORI (x->sizes[0] * x->sizes[1]) fprintf (f, "%a\n", data[i]);
    }
  else if (x->pixel_dimension == 3 && typ == IIO_TYPE_FLOAT)
    {
      int m =
        these_floats_are_actually_bytes (x->data,
                                         3 * x->sizes[0] *
                                         x->sizes[1]) ? 255 : 65535;
      float *data = x->data;
      fprintf (f, "P3\n%d %d\n%d\n", x->sizes[0], x->sizes[1], m);
      FORI (3 * x->sizes[0] * x->sizes[1])
      {
        fprintf (f, "%g\n", data[i]);
      }
    }
  else if (x->pixel_dimension == 3 && typ == IIO_TYPE_UINT8)
    {
      uint8_t *data = x->data;
      int w = x->sizes[0];
      int h = x->sizes[1];
      if (w * h <= 10000)
        {
          fprintf (f, "P3\n%d %d\n255\n", w, h);
          FORI (3 * w * h)
          {
            int datum = data[i];
            fprintf (f, "%d\n", datum);
          }
        }
      else
        {
          fprintf (f, "P6\n%d %d\n255\n", w, h);
          fwrite (data, 3 * w * h, 1, f);
        }
    }
  else if (x->pixel_dimension == 4 && typ == IIO_TYPE_UINT8)
    {
      fprintf (stderr, "IIO WARNING: assuming 4 chanels mean RGBA\n");
      uint8_t *data = x->data;
      fprintf (f, "P3\n%d %d\n255\n", x->sizes[0], x->sizes[1]);
      //error("write correctly here");
      FORI (4 * x->sizes[0] * x->sizes[1])
      {
        if (i % 4 == 3)
          continue;
        int datum = data[i];
        fprintf (f, "%d\n", datum);
      }
    }
  else if (x->pixel_dimension == 1 && typ == IIO_TYPE_UINT8)
    {
      uint8_t *data = x->data;
      int w = x->sizes[0];
      int h = x->sizes[1];
      if (w * h <= 10000)
        {
          fprintf (f, "P2\n%d %d\n255\n", w, h);
          FORI (w * h)
          {
            int datum = data[i];
            fprintf (f, "%d\n", datum);
          }
        }
      else
        {
          fprintf (f, "P5\n%d %d\n255\n", w, h);
          fwrite (data, w * h, 1, f);
        }
    }
  else
    iio_save_image_as_tiff_smarter (filename, x);
  //      error("\n\n\nThis particular data format can not yet be saved."
  //                      "\nPlease, ask enric.\n");
  xfclose (f);
}

void
iio_save_image_uint8_matrix_rgb (char *filename, uint8_t (**data)[3],
                                 int w, int h)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 3;
  x->type = IIO_TYPE_UINT8;
  x->data = data[0][0];
  iio_save_image_default (filename, x);
}

void
iio_save_image_uint8_matrix (char *filename, uint8_t ** data, int w, int h)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 1;
  x->type = IIO_TYPE_UINT8;
  x->data = data[0];
  iio_save_image_default (filename, x);
}

void
iio_save_image_float_vec (char *filename, float *data, int w, int h, int pd)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = pd;
  x->type = IIO_TYPE_FLOAT;
  x->data = data;
  x->contiguous_data = false;
  iio_save_image_default (filename, x);
}

void
iio_save_image_float_split (char *filename, float *data, int w, int h, int pd)
{
  float *rdata = xmalloc (w * h * pd * sizeof *rdata);
  recover_broken_pixels_float (rdata, data, w * h, pd);
  iio_save_image_float_vec (filename, rdata, w, h, pd);
  xfree (rdata);
}

void
iio_save_image_double_vec (char *filename, double *data, int w, int h, int pd)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = pd;
  x->type = IIO_TYPE_DOUBLE;
  x->data = data;
  x->contiguous_data = false;
  iio_save_image_default (filename, x);
}

void
iio_save_image_float (char *filename, float *data, int w, int h)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 1;
  x->type = IIO_TYPE_FLOAT;
  x->data = data;
  x->contiguous_data = false;
  iio_save_image_default (filename, x);
}

void
iio_save_image_double (char *filename, double *data, int w, int h)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = 1;
  x->type = IIO_TYPE_DOUBLE;
  x->data = data;
  x->contiguous_data = false;
  iio_save_image_default (filename, x);
}

void
iio_save_image_uint8_vec (char *filename, uint8_t * data,
                          int w, int h, int pd)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = pd;
  x->type = IIO_TYPE_UINT8;
  x->data = data;
  x->contiguous_data = false;
  iio_save_image_default (filename, x);
}

void
iio_save_image_uint16_vec (char *filename, uint16_t * data,
                           int w, int h, int pd)
{
  struct iio_image x[1];
  x->dimension = 2;
  x->sizes[0] = w;
  x->sizes[1] = h;
  x->pixel_dimension = pd;
  x->type = IIO_TYPE_UINT16;
  x->data = data;
  x->contiguous_data = false;
  iio_save_image_default (filename, x);
}

// misc debugging stuff {{{1

//void try_iio(void);
//void try_iio(void)
//{
//#ifdef _XOPEN_SOURCE
//      IIO_DEBUG("IIO compiled with _XOPEN_SOURCE = %d\n",_XOPEN_SOURCE);
//#endif
//      int w, h;
//      float **t = iio_read_image_float_matrix("-", &w, &h);
//      printf("we read a %dx%d image!\n", w, h);
//      xfree(t);
//
//      /*
//      FILE *f = stdin;
//      struct iio_image x[1]; read_image_f(x, f);
//      printf("we read a %dx%d image!\n", x->sizes[0], x->sizes[1]);
//      iio_print_image_info(stdout, x);
//      xfree(x->data);
//      */
//      /*
//      FILE *f = stdin;
//      struct iio_image *x = read_whole_jpeg(f);
//      printf("we read a %dx%d jpg!\n", x->sizes[0], x->sizes[1]);
//      unsigned char *data = x->data;
//      FORJ(x->sizes[1]) FORI(x->sizes[0])
//              FORL(x->pixel_dimension)
//                      printf("%g%c", (float)data[(x->sizes[0]*j+i)*x->pixel_dimension+l],
//                                      l==x->pixel_dimension-1?'\n':' ');
//      xfree(x);
//      */
//      /*
//      FILE *f = stdin;
//      //struct iio_image x[1];
//      char buf[4];
//      FORI(4) buf[i] = fgetc(f);
//      struct iio_image *x = read_beheaded_image_png(f, buf, 4);
//      printf("we read a %dx%d png!\n", x->sizes[0], x->sizes[1]);
//      float *data = x->data;
//      //FORJ(x->sizes[1]) FORI(x->sizes[0])
//      //      FORL(x->pixel_dimension)
//      //              printf("%g%c", data[(x->sizes[0]*j+i)*x->pixel_dimension+l],
//      //                              l==x->pixel_dimension-1?'\n':' ');
//      xfree(x);
//      */
//}
// }}}1

// vim:set foldmethod=marker:
