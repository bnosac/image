/*----------------------------------------------------------------------------

  Smooth Contours: an unsupervised method for detecting smooth contours
  in digital images. This code is part of the following publication and
  was subject to peer review:

    "Unsupervised Smooth Contour Detection"
    by Rafael Grompone von Gioi and Gregory Randall,
    Image Processing On Line, 2016.
    http://dx.doi.org/10.5201/ipol.2016.175

  Copyright (c) 2016 rafael grompone von gioi <grompone@gmail.com>,
                     Gregory Randall <randall@fing.edu.uy>

  Smooth Contours is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

/*----------------------------------------------------------------------------*/
#ifndef FALSE
#define FALSE 0
#endif /* !FALSE */

#ifndef TRUE
#define TRUE 1
#endif /* !TRUE */

/*----------------------------------------------------------------------------*/
/* PI */
#ifndef M_PI
#define M_PI   3.14159265358979323846
#endif /* !M_PI */

/*----------------------------------------------------------------------------*/
#define MIN(a,b) ( (a)<(b) ? (a) : (b) )

/*----------------------------------------------------------------------------*/
/* structure to store an element of the lateral regions of pixels to an arc
   operator. for each pixel it stores its value, the lateral distance to the
   arc defining the operator, and to which of the two lateral regions belongs
 */
struct region
{
  double val;  /* pixel value */
  double w;    /* absolute value of the lateral distance to the arc */
  int reg;     /* number of the lateral region to which it belongs: 1 or 2 */
};

/*----------------------------------------------------------------------------*/
/* structure to store a parameterization of an arc of circle
 */
struct arc_of_circle
{
  int is_line_segment;
  double a,b,c; /* a*x+b*y+c = 0 -> line through the arc endpoints */
  double d;     /* b*x-a*y+d = 0 -> orthogonal to previous through midpoint */
  double len;   /* arc length */
  double xc,yc,radius;     /* center and radius of circle containing the arc */
  double ang_ref,ang_span;
  int dir;
  int bbx0,bby0,bbx1,bby1; /* bounding box */
};

/*----------------------------------------------------------------------------*/
/* fatal error, print a message to standard error and exit
 */
static void error(char * msg)
{
  //fprintf(stderr,"error: %s\n",msg);
  //exit(EXIT_FAILURE);
}

/*----------------------------------------------------------------------------*/
/* memory allocation, print an error and exit if fail
 */
static void * xmalloc(size_t size)
{
  void * p;
  if( size == 0 ) error("xmalloc: zero size");
  p = malloc(size);
  if( p == NULL ) error("xmalloc: out of memory");
  return p;
}

/*----------------------------------------------------------------------------*/
/* compute a > b considering the rounding errors due to the representation
   of double numbers
 */
static int greater(double a, double b)
{
  if( a <= b ) return FALSE;  /* trivial case, return as soon as possible */

  if( (a-b) < 1000 * DBL_EPSILON ) return FALSE;

  return TRUE; /* greater */
}

/*----------------------------------------------------------------------------*/
/* Euclidean distance between x1,y1 and x2,y2
 */
static double dist(double x1, double y1, double x2, double y2)
{
  return sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
}

/*----------------------------------------------------------------------------*/
/* compute the error function erf using Winitzki's approximation.

   the formula is given in

     "A handy approximation for the error function and its inverse"
     by Sergei Winitzki, February 6, 2008,
     http://sites.google.com/site/winitzki/sergei-winitzkis-files/erf-approx.pdf

   the error function is defined as

     erf(x) = 2/sqrt(pi) \int_0^x e^{-t^2} dt

   and the approximation is

     erf(x) ~ (1 - exp(-x^2 * (4/pi + axˆ2) / (1 + ax^2) ) )^1/2

   with

     a = 8/3pi * (pi - 3) / (4 - pi)
 */
static double erf_winitzki(double x)
{
  static double a = 8.0 / 3.0 / M_PI * (M_PI - 3.0) / (4.0 - M_PI);
  if( x < 0.0 ) return -erf_winitzki(-x);
  return sqrt( 1.0 - exp(-x*x * (4.0/M_PI + a*x*x) / (1.0 + a*x*x)) );
}

/*----------------------------------------------------------------------------*/
/* compute a Gaussian kernel of length n, standard deviation sigma,
   and centered at value mean.

   for example, if mean=0.5, the Gaussian will be centered in the middle point
   between values kernel[0] and kernel[1].

   kernel must be allocated to a size n.
 */
static void gaussian_kernel(double * kernel, int n, double sigma, double mean)
{
  double sum = 0.0;
  double val;
  int i;

  /* check input */
  if( kernel == NULL ) error("gaussian_kernel: kernel not allocated");
  if( sigma <= 0.0 ) error("gaussian_kernel: sigma must be positive");

  /* compute Gaussian kernel */
  for(i=0; i<n; i++)
    {
      val = ( (double) i - mean ) / sigma;
      kernel[i] = exp( -0.5 * val * val );
      sum += kernel[i];
    }

  /* normalization */
  if( sum > 0.0 ) for(i=0; i<n; i++) kernel[i] /= sum;
}

/*----------------------------------------------------------------------------*/
/* filter an image with a Gaussian kernel of parameter sigma. return a pointer
   to a newly allocated filtered image, of the same size as the input image.
 */
static double * gaussian_filter(double * image, int X, int Y, double sigma)
{
  int x,y,offset,i,j,nx2,ny2,n;
  double * kernel;
  double * tmp;
  double * out;
  double val,prec;

  /* check input */
  if( sigma <= 0.0 ) error("gaussian_filter: sigma must be positive");
  if( image == NULL || X < 1 || Y < 1 ) error("gaussian_filter: invalid image");

  /* get memory */
  tmp = (double *) xmalloc( X * Y * sizeof(double) );
  out = (double *) xmalloc( X * Y * sizeof(double) );

  /* compute gaussian kernel */
  /*
     The size of the kernel is selected to guarantee that the first discarded
     term is at least 10^prec times smaller than the central value. For that,
     the half size of the kernel must be larger than x, with
       e^(-x^2/2sigma^2) = 1/10^prec
     Then,
       x = sigma * sqrt( 2 * prec * ln(10) )
   */
  prec = 3.0;
  offset = (int) ceil( sigma * sqrt( 2.0 * prec * log(10.0) ) );
  n = 1 + 2 * offset; /* kernel size */
  kernel = (double *) xmalloc( n * sizeof(double) );
  gaussian_kernel(kernel, n, sigma, (double) offset);

  /* auxiliary variables for the double of the image size */
  nx2 = 2*X;
  ny2 = 2*Y;

  /* x axis convolution */
  for(x=0; x<X; x++)
    for(y=0; y<Y; y++)
      {
        val = 0.0;
        for(i=0; i<n; i++)
          {
            j = x - offset + i;

            /* symmetry boundary condition */
            while(j<0) j += nx2;
            while(j>=nx2) j -= nx2;
            if( j >= X ) j = nx2-1-j;

            val += image[j+y*X] * kernel[i];
          }
        tmp[x+y*X] = val;
      }

  /* y axis convolution */
  for(x=0; x<X; x++)
    for(y=0; y<Y; y++)
      {
        val = 0.0;
        for(i=0; i<n; i++)
          {
            j = y - offset + i;

            /* symmetry boundary condition */
            while(j<0) j += ny2;
            while(j>=ny2) j -= ny2;
            if( j >= Y ) j = ny2-1-j;

            val += tmp[x+j*X] * kernel[i];
          }
        out[x+y*X] = val;
      }

  /* free memory */
  free( (void *) kernel );
  free( (void *) tmp );

  return out;
}

/*----------------------------------------------------------------------------*/
/* non oriented angle difference, returns a value in [0,pi]
 */
static double diff_0_pi(double a, double b)
{
  a -= b;
  while( a <= -M_PI ) a += 2.0 * M_PI;
  while( a >   M_PI ) a -= 2.0 * M_PI;
  if( a < 0.0 ) a = -a;
  return a;
}

/*----------------------------------------------------------------------------*/
/* return a score for chaining pixels 'from' to 'to', favoring closet point:
   = 0.0 invalid chaining
   > 0.0 valid forward chaining; the larger the value, the better the chaining
   < 0.0 valid backward chaining; the smaller the value, the better the chaining

   input:
     from, to       the two pixel IDs to evaluate their potential chaining
     Ex[i], Ey[i]   the sub-pixel position of point i, if i is an edge point;
                    they take values -1,-1 if i is not an edge point
     Gx[i], Gy[i]   the image gradient at pixel i
     X, Y           the size of the image
 */
static double chain( int from, int to, double * Ex, double * Ey,
                     double * Gx, double * Gy, int X, int Y )
{
  double dx,dy;

  /* check input */
  if( Ex == NULL || Ey == NULL || Gx == NULL || Gy == NULL )
    error("chain: invalid input");
  if( from < 0 || to < 0 || from >= X*Y || to >= X*Y )
    error("chain: one of the points is out the image");

  /* check that the points are different and valid edge points,
     otherwise return invalid chaining */
  if( from == to ) return 0.0; /* same pixel */
  if( Ex[from] < 0.0 || Ey[from] < 0.0 || Ex[to] < 0.0 || Ey[to] < 0.0 )
    return 0.0; /* one of them is not an edge point */

  /* in a good chaining, the gradient should be roughly orthogonal
     to the line joining the two points to be chained:

             Gx,Gy
            |                        ------> dx,dy
            |               thus
       from x-------x to             ---> Gy,-Gx  (orthogonal to the gradient)

     when Gy * dx - Gx * dy > 0, it corresponds to a forward chaining,
     when Gy * dx - Gx * dy < 0, it corresponds to a backward chaining.
     (this choice is arbitrary)

     first check that the gradient at both points to be chained agree
     in one direction, otherwise return invalid chaining.
   */
  dx = Ex[to] - Ex[from];
  dy = Ey[to] - Ey[from];
  if( (Gy[from] * dx - Gx[from] * dy) * (Gy[to] * dx - Gx[to] * dy) <= 0.0 )
    return 0.0;

  /* return the chaining score: positive for forward chaining,
     negative for backwards. the score is the inverse of the distance
     to the chaining point, to give preference to closer points */
  if( (Gy[from] * dx - Gx[from] * dy) >= 0.0 )
    return  1.0 / dist(Ex[from],Ey[from],Ex[to],Ey[to]); /* forward chaining  */
  else
    return -1.0 / dist(Ex[from],Ey[from],Ex[to],Ey[to]); /* backward chaining */
}

/*----------------------------------------------------------------------------*/
/* compute the image gradient, giving its x and y components as well as the
   modulus. Gx, Gy, and modG must be already allocated.
 */
static void compute_gradient( double * Gx, double * Gy, double * modG,
                              double * image, int X, int Y )
{
  int x,y;

  /* check input */
  if( Gx == NULL || Gy == NULL || modG == NULL || image == NULL )
    error("compute_gradient: invalid input");

  /* approximate image gradient using centered differences */
  for(x=1; x<(X-1); x++)
    for(y=1; y<(Y-1); y++)
      {
        Gx[x+y*X]   = image[(x+1)+y*X] - image[(x-1)+y*X];
        Gy[x+y*X]   = image[x+(y+1)*X] - image[x+(y-1)*X];
        modG[x+y*X] = sqrt( Gx[x+y*X] * Gx[x+y*X] + Gy[x+y*X] * Gy[x+y*X] );
      }
}

/*----------------------------------------------------------------------------*/
/* compute sub-pixel edge points using adapted Canny and Devernay methods.

   input: Gx, Gy, and modG are the x and y components and modulus of the image
          gradient, respectively. X,Y is the image size.

   output: Ex and Ey will have the x and y sub-pixel coordinates of the edge
           points found, or -1 and -1 when not an edge point. Ex and Ey must be
           already allocated.

   a modified Canny non maximal suppression [1] is used to select edge points,
   and a modified Devernay sub-pixel correction [2] is used to improve the
   position accuracy. in both cases, the modification boils down to using only
   vertical or horizontal non maximal suppression and sub-pixel correction.
   no threshold is used on the gradient.

   [1] J.F. Canny, "A computational approach to edge detection",
       IEEE Transactions on Pattern Analysis and Machine Intelligence,
       vol.8, no.6, pp.679-698, 1986.

   [2] F. Devernay, "A Non-Maxima Suppression Method for Edge Detection
       with Sub-Pixel Accuracy", Rapport de recherche 2724, INRIA, Nov. 1995.

   the reason for this modification is that Devernay correction is inconsistent
   for some configurations at 45, 225, -45 or -225 degree. in edges that should
   go exactly in the middle of a pixels like (5 pixels drawn):

                     ___
                    |
                 ___|
                |
             ___|
            |
         ___|
        |
     ___|

   the correction terms of both sides of the perfect edge are not compatible,
   leading to edge points with "oscillations" like:

                       .
                   .

                .
            .

         .
     .

   but the Devernay correction works very well and is very consistent when used
   to interpolate only along horizontal or vertical direction. this modified
   version requires that a pixel, to be an edge point, must be a local maximum
   horizontally or vertically, depending on the gradient orientation: if the
   x component of the gradient is larger than the y component, Gx > Gy, this
   means that the gradient is roughly horizontal and a horizontal maximum is
   required to be an edge point.

   when the pixel is both, a horizontal and vertical maximum, the direction
   with largest contrast is used for the interpolation. this usually leads to
   a more accurate estimation of the position.

   using only horizontal or vertical interpolation may lead occasionally to
   some missing edge points for edges at 45, 225, -45 or -225 degree. these
   does not happen very often and seems to be an acceptable price to pay for
   removing the "oscillations" in slanted edges. moreover, as it is very rare
   to have two consecutive missing points, using a chaining strategy that looks
   slightly farther away for neighbors, the chains of edge points are recovered
   well enough. this is done in the function chain_edge_points() below.
 */
static void compute_edge_points( double * Ex, double * Ey, double * modG,
                                 double * Gx, double * Gy, int X, int Y )
{
  int x,y,i;

  /* check input */
  if( Ex == NULL || Ey == NULL || modG == NULL || Gx == NULL || Gy == NULL )
    error("compute_edge_points: invalid input");

  /* initialize Ex and Ey as non edge points for all pixels */
  for(i=0; i<X*Y; i++) Ex[i] = Ey[i] = -1.0;

  /* explore pixels inside a 2 pixel margin (so modG[x,y +/- 1,1] is defined) */
  for(x=2; x<(X-2); x++)
    for(y=2; y<(Y-2); y++)
      {
        int Dx = 0;                     /* interpolation will be along Dx,Dy, */
        int Dy = 0;                     /*   which will be selected below     */
        double mod = modG[x+y*X];       /* modG at pixel */
        double L = modG[x-1+y*X];       /* modG at pixel on the left */
        double R = modG[x+1+y*X];       /* modG at pixel on the right */
        double U = modG[x+(y+1)*X];     /* modG at pixel up */
        double D = modG[x+(y-1)*X];     /* modG at pixel below */
        double gx = fabs( Gx[x+y*X] );  /* absolute value of Gx */
        double gy = fabs( Gy[x+y*X] );  /* absolute value of Gy */

        /* local horizontal and/or vertical maxima of the gradient modulus */
        /* it can happen that two neighbor pixels have equal value and are both
           maxima, for example when the edge is exactly between both pixels. in
           such cases, as an arbitrary convention, the edge is marked on the
           left one when an horizontal max or below when a vertical max. for
           this the conditions are L < mod >= R and D < mod >= U,
           respectively. the comparisons are done using the function greater()
           instead of the operators > or >= so numbers differing only due to
           rounding errors are considered equal */
        int lHm = greater(mod,L) && !greater(R,mod); /* local horizontal max */
        int lVm = greater(mod,D) && !greater(U,mod); /* local vertical max */

        /* if both horizontal and vertical max, interpolate along direction of
           max contrast; otherwise, requires max along approx gradient dir */
        if( lHm && lVm && MIN(L,R) < MIN(U,D) ) Dx = 1; /* both */
        else if( lHm && lVm ) Dy = 1;      /* both but more contrast in vert. */
        else if( lHm && gx >= gy ) Dx = 1; /* only horizontal maximum */
        else if( lVm && gx <= gy ) Dy = 1; /* only vertical maximum */

        /* Devernay sub-pixel correction [2]

           the edge point position is selected as the one of the maximum of a
           quadratic interpolation of the magnitude of the gradient along a
           unidimensional direction. the pixel must be a local maximum. so we
           have the values:
                                                   . b
                                            a .    |
             x = -1, |Gx| = a                 |    |    . c
             x =  0, |Gx| = b                 |    |    |
             x =  1, |Gx| = c               ------------------> x
                                             -1    0    1

           the x position of the maximum of the parabola passing through
           (-1,a), (0,b), and (1,c) is

             offset = (a - c) / 2(a - 2b + c)

           and because b >= a and b >= c, -0.5 <= offset <= 0.5
         */
        if( Dx > 0 || Dy > 0 )
          {
            /* offset value is in [-0.5, 0.5] */
            double a = modG[ x-Dx + (y-Dy) * X ];
            double b = modG[ x    +  y     * X ];
            double c = modG[ x+Dx + (y+Dy) * X ];
            double offset = 0.5 * (a - c) / (a - b - b + c);

            /* store edge point */
            Ex[x+y*X] = x + offset * Dx;
            Ey[x+y*X] = y + offset * Dy;
          }
      }
}

/*----------------------------------------------------------------------------*/
/* chain edge points

   input: Ex and Ey are the sub-pixel coordinates when an edge point is present
          or -1,-1 otherwise. Gx, Gy and modG are the x and y components and the
          modulus of the image gradient, respectively. X,Y is the image size.

   output: next and prev will contain the number of next and previous edge
           points in the chain. when not chained on one of the directions, the
           corresponding value is set to -1. next and prev must be allocated
           before calling.
 */
static void chain_edge_points( int * next, int * prev, double * Ex, double * Ey,
                               double * Gx, double * Gy, int X, int Y )
{
  int x,y,i,j,alt;

  /* check input */
  if( next==NULL || prev==NULL || Ex==NULL || Ey==NULL || Gx==NULL || Gy==NULL )
    error("chain_edge_points: invalid input");

  /* initialize next and prev as non linked */
  for(i=0; i<X*Y; i++) next[i] = prev[i] = -1;

  /* try each point to make local chains */
  for(x=2; x<(X-2); x++)   /* 2 pixel margin so the tested neighbors in image */
    for(y=2; y<(Y-2); y++)
      if( Ex[x+y*X] >= 0.0 && Ey[x+y*X] >= 0.0 ) /* must be an edge point */
        {
          int from = x+y*X;    /* edge point to be chained */
          double fwd_s = 0.0;  /* score of best forward chaining */
          double bck_s = 0.0;  /* score of best backward chaining */
          int fwd = -1;        /* edge point of best forward chaining */
          int bck = -1;        /* edge point of best backward chaining */

          /* try all neighbors two pixels apart or less.

             the detection of edge points occasionally fails to detect some
             edge points at 45, 225, -45 or -225 degree. as explained before,
             this is small price to pay for getting accurate detections.

             looking for candidates for chaining two pixels apart, in most
             such cases, is enough to obtain good chains of edge points that
             accurately describes the edge.
           */
          for(i=-2; i<=2; i++)
            for(j=-2; j<=2; j++)
              {
                int to = x+i + (y+j)*X; /* candidate edge point to be chained */
                double s = chain(from,to,Ex,Ey,Gx,Gy,X,Y);  /* score from-to */

                if( s > fwd_s ) /* a better forward chaining found */
                  {
                    fwd_s = s;  /* set the new best forward chaining */
                    fwd = to;
                  }
                if( s < bck_s ) /* a better backward chaining found */
                  {
                    bck_s = s;  /* set the new best backward chaining */
                    bck = to;
                  }
              }

          /* before making the new chain, check whether the target was
             already chained and in that case, whether the alternative
             chaining is better than the proposed one.

                            x alt                        x alt
                             \                          /
                              \                        /
                from x---------x fwd              bck x---------x from

             we know that the best forward chain starting at from is from-fwd.
             but it is possible that there is an alternative chaining arriving
             at fwd that is better, such that alt-fwd is to be preferred to
             from-fwd. an analogous situation is possible in backward chaining,
             where an alternative link bck-alt may be better than bck-from.

             before making the new link, check if fwd/bck are already chained,
             and in such case compare the scores of the proposed chaining to
             the existing one, and keep only the best of the two.

             there is an undesirable aspect of this procedure: the result may
             depend on the order of exploration. consider the following
             configuration:

                 a x-------x b
                          /
                         /
                      c x---x d    with score(a-b) < score(c-b) < score(c-d)

             let us consider two possible orders of exploration.

             order: a,b,c
             we will first chain a-b when exploring a. when analyzing the
             backward links of b, we will prefer c-b, and a-b will be unlinked.
             finally, when exploring c, c-d will be preferred and c-b will be
             unlinked. the result is just c-d.

             order: c,b,a
             we will first chain c-d when exploring c. then, when exploring
             the backward connections of b, c-b will be the preferred link;
             but because c-d was already done and has a better score, c-b
             cannot be linked. finally, when exploring a, the link a-b will
             be created because there is no better backward linking of b.
             the result is c-d and a-b.

             we did not found yet a simple algorithm to solve this problem. by
             simple, we mean an algorithm without two passes or the need to
             re-evaluate the chaining of points where one link is cut.

             for most edge points, there is only one possible chaining and this
             problem does not arise. but it does happen and a better solution
             is desirable.
           */
          if( fwd >= 0 && next[from] != fwd &&
              ((alt=prev[fwd]) < 0 || chain(alt,fwd,Ex,Ey,Gx,Gy,X,Y) < fwd_s) )
            {
              if( next[from] >= 0 )     /* remove previous from-x link if one */
                prev[next[from]] = -1;  /* only prev requires explicit reset  */
              next[from] = fwd;         /* set next of from-fwd link          */
              if( alt >= 0 )            /* remove alt-fwd link if one         */
                next[alt] = -1;         /* only next requires explicit reset  */
              prev[fwd] = from;         /* set prev of from-fwd link          */
            }
          if( bck >= 0 && prev[from] != bck &&
              ((alt=next[bck]) < 0 || chain(alt,bck,Ex,Ey,Gx,Gy,X,Y) > bck_s ) )
            {
              if( alt >= 0 )            /* remove bck-alt link if one         */
                prev[alt] = -1;         /* only prev requires explicit reset  */
              next[bck] = from;         /* set next of bck-from link          */
              if( prev[from] >= 0 )     /* remove previous x-from link if one */
                next[prev[from]] = -1;  /* only next requires explicit reset  */
              prev[from] = bck;         /* set prev of bck-from link          */
            }
        }
}

/*----------------------------------------------------------------------------*/
/* simplify a chain i-j-k-l-m into i-j-l-m when the latter is more regular

   next and prev contain the number of next and previous edge points in the
   chain or -1 when not chained. Ex and Ey are the sub-pixel coordinates when
   an edge point is present or -1,-1 otherwise. Ex and Ey are the sub-pixel
   coordinates when an edge point is present or -1,-1 otherwise.

   this function modifies next and prev when simplifications are found.
 */
static void simplify_chains( int * next, int * prev, double * Ex, double * Ey,
                             int X, int Y )
{
  int i,j,k,l,m;

  /* check input */
  if( next == NULL || prev == NULL || Ex == NULL || Ey == NULL )
    error("simplify_chains: invalid input");

  /* look for chains of 5 consecutive chained edge points, i-j-k-l-m:

             x k
            / \
       x---x   x---x
       i   j   l   m

     we will consider to link j directly to l, leaving k out of the chain.
     an additional requirement is that the distance of the potential link
     j-l is less than 2.

     the latter is to avoid a general regularization of the chain. in normal
     linking along edges, the mean distance between edge points is in the
     interval from 1 (when horizontal or vertical) to sqrt(2) (45 degree).
     so, the mean distance between edge points at two chaining steps is
     in [2, 2*sqrt(2)]. restricting the simplifications to cases where the
     two step distance is less than 2, avoid regularizing normal chaining
     and concentrates on its objective of simplifying complex corners.

     to state the conditions to simplify the chain, let us define the following
     angles:

       A is angle(i-j,l-m)               x---x   x---x
                                         i   j   l   m

                                               x
                                              /
       B is angle(i-j,j-k)               x---x
                                         i   j

                                               x k
                                                \
       C is angle(k-l,l-m)                       x---x
                                                 l   m

     the condition to simplify the chain is A < B and A < C
     or equivalently A < MIN(B,C).

                               x k
                              / \
     to sum up, a chain  x---x   x---x  becomes  x---x---x---x  when the latter
                         i   j   l   m           i   j   l   m

     is more regular.
   */
  for(i=0; i<X*Y; i++) /* next[i]>=0 -> edge point, no explicit test needed */
    if( (j=next[i])>=0 && (k=next[j])>=0 && (l=next[k])>=0 && (m=next[l])>=0 &&
        dist(Ex[j],Ey[j],Ex[l],Ey[l]) < 2.0 )
      {
        double a = atan2( Ey[j] - Ey[i], Ex[j] - Ex[i] );  /* angle(i-j) */
        double b = atan2( Ey[k] - Ey[j], Ex[k] - Ex[j] );  /* angle(j-k) */
        double c = atan2( Ey[l] - Ey[k], Ex[l] - Ex[k] );  /* angle(k-l) */
        double d = atan2( Ey[m] - Ey[l], Ex[m] - Ex[l] );  /* angle(l-m) */

        /* if A < MIN(B,C) => simplify: link j-l, unlink j-k and k-l */
        if( diff_0_pi(a,d) < MIN( diff_0_pi(a,b), diff_0_pi(c,d) ) )
          {
            next[j] = l;
            prev[l] = j;
            next[k] = -1;
            prev[k] = -1;
          }
      }
}

/*----------------------------------------------------------------------------*/
/* create a list of chained edge points composed of 3 lists
   x, y and curve_limits; it also computes N (the number of edge points) and
   M (the number of curves).

   x[i] and y[i] (0<=i<N) store the sub-pixel coordinates of the edge points.
   curve_limits[j] (0<=j<=M) stores the limits of each chain in lists x and y.

   x, y, and curve_limits will be allocated.

   example:

     curve number k (0<=k<M) consists of the edge points x[i],y[i]
     for i determined by curve_limits[k] <= i < curve_limits[k+1].

     curve k is closed if x[curve_limits[k]] == x[curve_limits[k+1] - 1] and
                          y[curve_limits[k]] == y[curve_limits[k+1] - 1].
 */
static void list_chained_edge_points( double ** x, double ** y, int * N,
                                      int ** curve_limits, int * M,
                                      int * next, int * prev,
                                      double * Ex, double * Ey, int X, int Y )
{
  int i,k,n;

  /* initialize output: x, y, curve_limits, N, and M

     there cannot be more than X*Y edge points to be put in the output list:
     edge points must be local maxima of gradient modulus, so at most half of
     the pixels could be so. when a closed curve is found, one edge point will
     be put twice to the output. even if all possible edge points (half of the
     pixels in the image) would form one pixel closed curves (which is not
     possible) that would lead to output X*Y edge points.

     for the same reason, there cannot be more than X*Y curves: the worst case
     is when all possible edge points (half of the pixels in the image) would
     form one pixel chains. in that case (which is not possible) one would need
     a size for curve_limits of X*Y/2+1. so X*Y is enough.

     (curve_limits requires one more item than the number of curves.
      a simplest example is when only one chain of length 3 is present:
      curve_limits[0] = 0, curve_limits[1] = 3.)
   */
  *x = (double *) xmalloc( X * Y * sizeof(double) );
  *y = (double *) xmalloc( X * Y * sizeof(double) );
  *curve_limits = (int *) xmalloc( X * Y * sizeof(int) );
  *N = 0;
  *M = 0;

  /* copy chained edge points to output */
  for(i=0; i<X*Y; i++)   /* prev[i]>=0 or next[i]>=0 implies edge point */
    if( prev[i] >= 0 || next[i] >= 0 )
      {
        /* a new chain found, set chain starting index to the current point
           and then increase the curve counter */
        (*curve_limits)[*M] = *N;
        ++(*M);

        /* set k to the begining of the chain, or to i if closed curve */
        for(k=i; (n=prev[k])>=0 && n!=i; k=n);

        /* follow the chain of edge points starting on k */
        do
          {
            /* store the current point coordinates in the output lists */
            (*x)[*N] = Ex[k];
            (*y)[*N] = Ey[k];
            ++(*N);

            n = next[k];   /* save the id of the next point in the chain */

            next[k] = -1;  /* unlink chains from k so it is not used again */
            prev[k] = -1;

            /* for closed curves, the initial point is included again as
               the last point of the chain. actually, testing if the first
               and last points are equal is the only way to know that it is
               a closed curve.

               to understand that this code actually repeats the first point,
               consider a closed chain as follows:  a--b
                                                    |  |
                                                    d--c

               let us say that the algorithm starts by point a. it will store
               the coordinates of point a and then unlink a-b. then, will store
               point b and unlink b-c, and so on. but the link d-a is still
               there. (point a is no longer pointing backwards to d, because
               both links are removed at each step. but d is indeed still
               pointing to a.) so it will arrive at point a again and store its
               coordinates again as last point. there, it cannot continue
               because the link a-b was removed, there would be no next point,
               k would be -1 and the curve is finished.
             */

            k = n;  /* set the current point to the next in the chain */
          }
        while( k >= 0 ); /* continue while there is a next point in the chain */
      }
  (*curve_limits)[*M] = *N; /* store end of the last chain */
}

/*----------------------------------------------------------------------------*/
/* chained, sub-pixel edge detector. based on a modified Canny non-maximal
   suppression and a modified Devernay sub-pixel correction.

   the input image is assumed to be blurred as desired.
   x, y, and curve_limits will be allocated.

   the output are the chained edge points given as 3 lists x, y and
   curve_limits, as well as the number N of edge points, and the number M of
   curves.

   x[i] and y[i] (0<=i<N) store the sub-pixel coordinates of the edge points.
   curve_limits[j] (0<=j<=M) stores the limits of each chain in lists x and y.

   example:

     curve number k (0<=k<M) consists of the edge points x[i],y[i]
     for i determined by curve_limits[k] <= i < curve_limits[k+1].

     curve k is closed if x[curve_limits[k]] == x[curve_limits[k+1] - 1] and
                          y[curve_limits[k]] == y[curve_limits[k+1] - 1].
 */
static void chained_subpixel_edge_points( double ** x, double ** y, int * N,
                                          int ** curve_limits, int * M,
                                          double * image, int X, int Y )
{
  double * Gx   = (double *) xmalloc( X * Y * sizeof(double) );     /* grad_x */
  double * Gy   = (double *) xmalloc( X * Y * sizeof(double) );     /* grad_y */
  double * modG = (double *) xmalloc( X * Y * sizeof(double) );     /* |grad| */
  double * Ex   = (double *) xmalloc( X * Y * sizeof(double) );     /* edge_x */
  double * Ey   = (double *) xmalloc( X * Y * sizeof(double) );     /* edge_y */
  int * next = (int *) xmalloc( X * Y * sizeof(int) ); /* next point in chain */
  int * prev = (int *) xmalloc( X * Y * sizeof(int) ); /* prev point in chain */

  compute_gradient(Gx,Gy,modG,image,X,Y);

  compute_edge_points(Ex,Ey,modG,Gx,Gy,X,Y);

  chain_edge_points(next,prev,Ex,Ey,Gx,Gy,X,Y);

  simplify_chains(next,prev,Ex,Ey,X,Y);

  list_chained_edge_points(x,y,N,curve_limits,M,next,prev,Ex,Ey,X,Y);

  /* free memory */
  free( (void *) Gx );
  free( (void *) Gy );
  free( (void *) modG );
  free( (void *) Ex );
  free( (void *) Ey );
  free( (void *) next );
  free( (void *) prev );
}

/*----------------------------------------------------------------------------*/
/* compute the center and radius of the circle through 3 non-aligned points

   this function will set the values xc, yc, radius of the arc_of_circle
   pointed by arc.
 */
static void circle_through( struct arc_of_circle * arc, double x1, double y1,
                            double x2, double y2, double x3, double y3 )
{
  double h,k,den,xxyy1,xxyy2,xxyy3;

  /* from http://mathforum.org/library/drmath/view/55239.html

     Date: 05/25/2000 at 10:45:58
     From: Doctor Rob
     Subject: Re: finding the coordinates of the center of a circle

     Thanks for writing to Ask Dr. Math, Alison.

     Let (h,k) be the coordinates of the center of the circle, and r its
     radius. Then the equation of the circle is:

          (x-h)^2 + (y-k)^2 = r^2

     Since the three points all lie on the circle, their coordinates will
     satisfy this equation. That gives you three equations:

          (x1-h)^2 + (y1-k)^2 = r^2
          (x2-h)^2 + (y2-k)^2 = r^2
          (x3-h)^2 + (y3-k)^2 = r^2

     in the three unknowns h, k, and r. To solve these, subtract the first
     from the other two. That will eliminate r, h^2, and k^2 from the last
     two equations, leaving you with two simultaneous linear equations in
     the two unknowns h and k. Solve these, and you'll have the coordinates
     (h,k) of the center of the circle. Finally, set:

          r = sqrt[(x1-h)^2+(y1-k)^2]

     and you'll have everything you need to know about the circle.

     This can all be done symbolically, of course, but you'll get some
     pretty complicated expressions for h and k. The simplest forms of
     these involve determinants, if you know what they are:

              |x1^2+y1^2  y1  1|        |x1  x1^2+y1^2  1|
              |x2^2+y2^2  y2  1|        |x2  x2^2+y2^2  1|
              |x3^2+y3^2  y3  1|        |x3  x3^2+y3^2  1|
          h = ------------------,   k = ------------------
                  |x1  y1  1|               |x1  y1  1|
                2*|x2  y2  1|             2*|x2  y2  1|
                  |x3  y3  1|               |x3  y3  1|
   */

  den = x1*y2 + y1*x3 + x2*y3 - x3*y2 - x2*y1 - x1*y3; /* denominator */
  if( den == 0.0 ) error("the 3 points are aligned");

  xxyy1 = x1*x1 + y1*y1;
  xxyy2 = x2*x2 + y2*y2;
  xxyy3 = x3*x3 + y3*y3;

  h = xxyy1*y2 + xxyy3*y1 + xxyy2*y3 - xxyy3*y2 - xxyy2*y1 - xxyy1*y3;
  h /= 2.0 * den;

  k = x1*xxyy2 + x3*xxyy1 + x2*xxyy3 - x3*xxyy2 - x2*xxyy1 - x1*xxyy3;
  k /= 2.0 * den;

  arc->xc = h;
  arc->yc = k;
  arc->radius = dist(x1,y1,h,k);
}

/*----------------------------------------------------------------------------*/
/* oriented difference of angles modulo 2pi
 */
static double diff_0_2pi(double a, double b)
{
  a -= b;
  while( a < 0.0      ) a += 2.0*M_PI;
  while( a > 2.0*M_PI ) a -= 2.0*M_PI;
  return a;
}

/*----------------------------------------------------------------------------*/
/* initialize an arc structure from point i to point k in the list x[],y[] and
   with width max_w. return TRUE if the arc is smooth up to precision sigma,
   FALSE otherwise
 */
static int smooth_segment( struct arc_of_circle * arc, double * x, double * y,
                           int i, int k, double sigma, double max_w,
                           int X, int Y )
{
  int l;
  int j = i + (k-i) / 2; /* middle point */
  double ang_i,ang_j,ang_k;

  /* full circles are not handled as one arc by the validation step.

     this is to avoid considering the special case of closed circles. in
     practice, the impact is minor, it only means that full circles will not be
     validated all together. but usually most of the partial arcs of the circle
     will be validated, so the result would be a curve following the full
     circle. the real risk is of losing small circles, which may have the limit
     size where removing even one pixel leads to a reject.
   */
  if( x[i]==x[k] && y[i]==y[k] ) return FALSE; /* exactly the same x,y values
                                                  are repeated in closed curves,
                                                  so in this case there is no
                                                  risk in using the operator ==
                                                  to compare doubles */

  /* compute the parameters of the line through x[i],y[i] and x[k],y[k],
     the orthogonal line to the previous, and len as a line segment */
  arc->len = dist(x[i],y[i],x[k],y[k]); /* if arc  will be re-computed later */
  arc->a = -(y[k] - y[i]) / arc->len;
  arc->b =  (x[k] - x[i]) / arc->len;
  arc->c = -arc->a*x[i] - arc->b*y[i];
  arc->d = -arc->b*0.5*(x[i]+x[k]) + arc->a*0.5*(y[i]+y[k]);

  /* initialize bounding box */
  arc->bbx0 = arc->bbx1 = (int) x[i];
  arc->bby0 = arc->bby1 = (int) y[i];

  /* line segment or arc? decide evaluating the middle point */
  if( fabs( arc->a*x[j] + arc->b*y[j] + arc->c ) < sigma )
    {
      /* line segment */
      arc->is_line_segment = TRUE;

      /* check that all points are not farther than sigma from the line */
      for(l=i; l<=k; l++)
        {
          /* update bounding box */
          if( x[l] < (double) arc->bbx0 ) arc->bbx0 = (int) x[l];
          if( y[l] < (double) arc->bby0 ) arc->bby0 = (int) y[l];
          if( x[l] > (double) arc->bbx1 ) arc->bbx1 = (int) x[l];
          if( y[l] > (double) arc->bby1 ) arc->bby1 = (int) y[l];

          if( fabs(arc->a*x[l] + arc->b*y[l] + arc->c) > sigma )
            return FALSE; /* not smooth */
        }
    }
  else
    {
      /* arc */
      arc->is_line_segment = FALSE;

      /* compute circle center and radius */
      circle_through(arc,x[i],y[i],x[j],y[j],x[k],y[k]);

      /* arc direction */
      ang_i = atan2(y[i] - arc->yc, x[i] - arc->xc);
      ang_j = atan2(y[j] - arc->yc, x[j] - arc->xc);
      ang_k = atan2(y[k] - arc->yc, x[k] - arc->xc);
      if( diff_0_2pi(ang_j,ang_i) < diff_0_2pi(ang_k,ang_i) )
        {
          arc->dir =  1;  /* direction of turn of the arc */
          arc->ang_span = diff_0_2pi(ang_k,ang_i); /* span */
          arc->ang_ref = ang_i;
        }
      else
        {
          arc->dir = -1;  /* direction of turn of the arc */
          arc->ang_span = diff_0_2pi(ang_i,ang_k); /* span */
          arc->ang_ref = ang_k;
        }

      /* compute arc length */
      arc->len = arc->radius * arc->ang_span;

      /* check that all points are not farther than sigma from the arc */
      for(l=i; l<=k; l++)
        {
          /* update bounding box */
          if( x[l] < (double) arc->bbx0 ) arc->bbx0 = (int) x[l];
          if( y[l] < (double) arc->bby0 ) arc->bby0 = (int) y[l];
          if( x[l] > (double) arc->bbx1 ) arc->bbx1 = (int) x[l];
          if( y[l] > (double) arc->bby1 ) arc->bby1 = (int) y[l];

          if( fabs(dist(x[l],y[l],arc->xc,arc->yc) - arc->radius) > sigma )
            return FALSE; /* not smooth */
        }
    }

  /* correct bounding box */
  arc->bbx0 -= max_w;      /* add the width of the max lateral regions */
  arc->bby0 -= max_w;
  arc->bbx1 += max_w + 1;
  arc->bby1 += max_w + 1;
  if( arc->bbx0 < 0 ) arc->bbx0 = 0; /* keep the bounding box inside image */
  if( arc->bby0 < 0 ) arc->bby0 = 0;
  if( arc->bbx1 > X ) arc->bbx1 = X;
  if( arc->bby1 > Y ) arc->bby1 = Y;

  return TRUE; /* is smooth */
}

/*----------------------------------------------------------------------------*/
/* get the lateral regions to an arc operator: the sub-case of a line segment

   input: pointer to an image of size X,Y, a pointer to the arc operator,
          and the width of lateral region w.

   output: the array pointed by reg will be filled with the elements of the
           regions. its size is returned in n.
 */
static void get_region_line( int * n, struct region * reg,
                             double * image, int X, int Y,
                             struct arc_of_circle * arc, double w )
{
  int x,y;

  /* check input */
  if( image==NULL || X<=0 || Y<=0 ) error("get_region_line: invalid image");
  if( arc==NULL ) error("get_region_line: invalid arc");

  /* count points */
  *n = 0;
  for(x=arc->bbx0; x<arc->bbx1; x++)
    for(y=arc->bby0; y<arc->bby1; y++)
      {
        double d_lat = arc->a*x + arc->b*y + arc->c;
        double d_lon = arc->b*x - arc->a*y + arc->d;

        if( fabs(d_lon) <= 0.5*arc->len && fabs(d_lat) <= w )
          {
            if( d_lat < 0.0 ) reg[*n].reg = 1;
            else              reg[*n].reg = 2;

            reg[*n].val = image[x+y*X];
            reg[*n].w = fabs(d_lat);
            (*n)++;
          }
      }
}

/*----------------------------------------------------------------------------*/
/* get the lateral regions to an arc operator: the sub-case of an arc of circle

   input: pointer to an image of size X,Y, a pointer to the arc operator,
          and the width of lateral region w.

   output: the array pointed by reg will be filled with the elements of the
           regions. its size is returned in n.
 */
static void get_region_arc( int * n, struct region * reg,
                            double * image, int X, int Y,
                            struct arc_of_circle * arc, double w )
{
  int x,y;

  /* check input */
  if( image==NULL || X<=0 || Y<=0 ) error("get_region_arc: invalid image");
  if( arc==NULL || arc->is_line_segment ) error("get_region_arc: invalid arc");

  /* count points */
  *n = 0;
  for(x=arc->bbx0; x<arc->bbx1; x++)
    for(y=arc->bby0; y<arc->bby1; y++)
      {
        double r = dist(arc->xc, arc->yc, (double) x, (double) y);
        double offset = r - arc->radius;
        double ang = atan2(y - arc->yc, x - arc->xc);
        double ang_diff = diff_0_2pi(ang,arc->ang_ref);

        /* is the point in the angle sector? */
        if( ang_diff >= 0.0 && ang_diff <= arc->ang_span && fabs(offset) <= w )
          {
            if( (offset < 0.0 && arc->dir < 0) ||
                (offset > 0.0 && arc->dir > 0)    ) reg[*n].reg = 1;
            else                                    reg[*n].reg = 2;

            reg[*n].val = image[x+y*X];
            reg[*n].w = fabs(offset);
            (*n)++;
          }
      }
}

/*----------------------------------------------------------------------------*/
/* comparison function for type 'struct region' to be used with qsort
 */
static int comp_region(const void * a, const void * b)
{
  if( ( (struct region *) a)->val < ( (struct region *) b)->val ) return -1;
  if( ( (struct region *) a)->val > ( (struct region *) b)->val ) return  1;
  return  0;
}

/*----------------------------------------------------------------------------*/
/* get the lateral regions to an arc operator, apply the correcting term
   for pixel quantization, and sort its elements according to pixel value

   input: pointer to an image of size X,Y, a pointer to the arc operator,
          the width of lateral region w, and the pixel quantization step Q.

   output: the array pointed by reg will be filled with the elements of the
           regions. its size returned in n.
 */
static void get_region( int * n, struct region * reg,
                        double * image, int X, int Y,
                        struct arc_of_circle * arc, double w, double Q )
{
  double q_offset = 0.616793 * Q;
  int i;

  /* collect pixel values */
  if( arc->is_line_segment ) get_region_line(n,reg,image,X,Y,arc,w);
  else                       get_region_arc(n,reg,image,X,Y,arc,w);

  /* compensate pixel values quantization */
  for(i=0; i<*n; i++)
    if( reg[i].reg == 1 )
      reg[i].val += q_offset;

  /* sort region by value */
  qsort( (void *) reg, (size_t) *n, sizeof(struct region), &comp_region );
}

/*----------------------------------------------------------------------------*/
/* compute arc NFA using Mann-Whitney U test
   http://en.wikipedia.org/wiki/Mann%E2%80%93Whitney_U_test

   input: X,Y is the image size, arc is a pointer to the arc, w is the width
          of the lateral regions of the current arc operator, reg is a pointer
          to the lateral regions of size n, W is the number of width used in
          the method, and gap is the size in pixel units of the space to
          separate both regions.

   return: log(NFA)
 */
static double arc_log_nfa( int X, int Y, struct arc_of_circle * arc, double w,
                           int n, struct region * reg, int W, double gap )
{
  int n1,n2,i,rank,sum_tied_ranks,num_tied,num_tied_r2;
  double tie_val,adjusted_rank,sum_rank_r2,u,m,s,z,pvalue;

  /* Number of Tests: NT = sqrt(XY) * XY * (4 + pi^2)/3 * l^2 * num_width

     XY                 : number of centers
     sqrt(XY)           : number of arc lengths
     pi*l               : number of orientations of arcs of length l
     (4/pi + pi)/3 * l  : number of arcs of a given length, center, orientation;
                          this includes from full circles of perimeter l,
                          to straight line segments of length l.
     num_width          : number of arc width considered (counting with or
                          without central gap as different)
   */
  double logNT = 1.5 * log10( (double) X ) + 1.5 * log10( (double) Y )
               + log10(4.6232) + 2.0 * log10( (double) arc->len )
               + log10( (double) W );

  /* compute Mann-Whitney U statistic.

     the elements in reg are assumed to be ordered by increasing values.

     the assignation of the initial rank, the assignation of the adjusted rank,
     and the sum of ranks in region 2, all is done in one single loop.

     for this there are variables to keep the value of the current tied pixel
     group, its size, and how many belong to region 2. each new value evaluated
     can have the same value as in the current tied group or a larger one. if
     it is the same, it belongs to the tied group and the variables are
     updated. if it is larger, that means that the previous tied group is
     completed, its adjusted rank can be assigned, and the corresponding
     quantity added to the sum of ranks in region 2. the variables are updated
     to reflect the new tied group including initially only the new pixel.
   */
  n1 = n2 = 0;
  sum_rank_r2 = 0.0;
  rank = 0;
  tie_val = reg[0].val;  /* set tied pixel group for the first pixel in reg */
  sum_tied_ranks = num_tied = num_tied_r2 = 0;
  for(i=0; i<n; i++)
    if( reg[i].w > 0.5*gap && reg[i].w <= w ) /* evaluate only pixels inside
                                                 the current width */
      {
        if( greater(reg[i].val, tie_val) ) /* a new tied pixel group */
          {
            /* compute the adjusted rank and assign the rank values of region 2
               in the last tied pixel group */
            if( num_tied > 0 && num_tied_r2 > 0 )
              {
                adjusted_rank = (double) sum_tied_ranks / (double) num_tied;
                sum_rank_r2 += (double) num_tied_r2 * adjusted_rank;
              }

            /* initialize new tied pixel group */
            tie_val = reg[i].val;
            sum_tied_ranks = num_tied = num_tied_r2 = 0;
          }

        ++rank; /* rank in the ordering among the pixels in the region */
        sum_tied_ranks += rank;
        ++num_tied;

        /* count pixels in region 1 and 2 */
        if( reg[i].reg == 1 ) ++n1;
        else
          {
            ++n2;
            ++num_tied_r2;
          }
      }
  if( num_tied > 0 && num_tied_r2 > 0 ) /* assign ranks of last tied group */
    sum_rank_r2 += (double) num_tied_r2 * sum_tied_ranks / num_tied;
  u = sum_rank_r2 - 0.5 * n2 * (n2 + 1.0); /* compute u value */

  /* compute z, a version of u with standard normal distribution, N(0,1) */
  m = 0.5 * n1 * n2;
  s = sqrt( n1 * n2 * (n1+n2+1.0) / 12.0 );
  if( n1 > 0 && n2 > 0 && s > 0.0 ) z = (u - m) / s;
  else return logNT; /* one of the regions has no pixel => not meaningful */

  /* compute the p-value using the standard error function */
  pvalue = 0.5 * ( 1.0 - erf_winitzki(z/sqrt(2.0)) );
  if( pvalue <= 0.0 ) /* the p-value should always be larger than zero.
                         this condition reveals a numeric overflow,
                         due to a very very small p-value. then the arc
                         is meaningful => return a negative log10(NFA) */
    return (double) DBL_MIN_10_EXP; /* minimal negative exponent in doubles */

  /* return the log10(NFA) */
  return logNT + log10(pvalue);
}

/*----------------------------------------------------------------------------*/
/* copy the meaningful curves from structures xx,yy,curve,NN,MM
   to structures x,y,curve_limits,N,M

   input:
     xx,yy        : list of point coordinates
     curve        : curve limits in lists xx,yy
     NN           : number of points
     MM           : number of curves
     meaningful   : status of each point (TRUE=meaningful; FALSE otherwise)

   output:
     x,y          : list of point coordinates
     curve_limits : curve limits in lists x,y
     N            : number of points
     M            : number of curves

   the output arrays x,y and curve_limits will be allocated.
 */
static void keep_meaningful_curves( double ** x, double ** y, int * N,
                                    int ** curve_limits, int * M,
                                    double * xx, double * yy, int NN,
                                    int * curve, int MM, int * meaningful )
{
  int c,i;

  *N = *M = 0;     /* initialize N and M to zero point, zero curve */

  /* if the input is empty, return NULL pointers */
  if( NN <= 0 )
    {
      *x = *y = NULL;
      *curve_limits = NULL;
      return;
    }

  /* allocate memory
     worst case: each point is kept (n points so 2n coordinates)
                 and each one is one chain (n+1 limits) */
  *x = (double *) xmalloc( NN * sizeof(double) );
  *y = (double *) xmalloc( NN * sizeof(double) );
  *curve_limits = (int *) xmalloc( (NN+1) * sizeof(int) );

  /* create list of meaningful curves */
  (*curve_limits)[0] = 0; /* this is always right but needs to be initialized */
  for(c=0; c<MM; c++) /* iterate on curves */
    {
      int in_chain = FALSE; /* not in a chain when starting the curve */

      /* iterate the points of the curve */
      for(i=curve[c]; i<curve[c+1]; i++)
        if( meaningful[i] ) /* meaningful point, add it */
          {
            if( !in_chain ) /* new chain */
              {
                in_chain = TRUE;
                ++(*M); /* increase chain counter */
              }
            (*x)[*N] = xx[i]; /* store point coordinates */
            (*y)[*N] = yy[i];
            ++(*N);           /* increase point counter */
            (*curve_limits)[*M] = *N; /* curve_limits[M] should contain first
                                         point of following chain, which is N
                                         because is was already increased */
          }
        else in_chain = FALSE; /* not meaningful, not in chain */
    }
}

/*----------------------------------------------------------------------------*/
/* compute the minimal arc length that may lead to a validated detection
   for the given image size

   input: X,Y is the size of the image, max_w is the maximal width of the
          lateral regions, W is the number of widths for the lateral regions
          used in the method, and log_eps is the threshold applied to log(NFA)

   return: the minimal arc length to be evaluated
 */
static int compute_min_length(int X, int Y, double max_w, int W, double log_eps)
{
  double l_nfa;
  int min_l;

  /* evaluate the most favorable case for each length starting in one.
     stop when NFA <= epsilon is possible */
  for(l_nfa=DBL_MAX, min_l=1; l_nfa>=log_eps; min_l++)
    {
      /* most meaningful arc when all points are in the right order.
         that is, u gets its maximal possible value of n*n.
         then, z = (n*n - 0.5*n*n) / sqrt(.) = 0.5*n*n / sqrt(.) */
      double n = min_l * max_w;
      double z = 0.5*n*n / sqrt(n*n*(n+n+1.0)/12.0);

      /* Number of Tests: NT = sqrt(XY) * XY * (4 + pi^2)/3 * l^2 * num_width

         XY                : number of centers
         sqrt(XY)          : number of arc lengths
         pi*l              : number of orientations of arcs of length l
         (4/pi + pi)/3 * l : num. of arcs of a given length, center, orientation
                             this includes from full circles of perimeter l,
                             to straight line segments of length l
         num_width         : number of arc width considered (counting with or
                             without central gap as different)
       */
      double logNT = 1.5 * log10( (double) X ) + 1.5 * log10( (double) Y )
                   + log10(4.6232) + 2.0 * log10( (double) min_l )
                   + log10( (double) W );

      l_nfa = logNT + log10( 0.5 * (1.0 - erf_winitzki(z/sqrt(2.0))) );
    }

  return min_l;
}

/*----------------------------------------------------------------------------*/
/* Smooth Contours is an algorithm for detecting smooth contours on digital
   images. The output contours are given as chained sub-pixel edge points.

   Input:

     image        : the input image
     X,Y          : the size of the input image
     Q            : the pixel quantization step

   Output:

     x,y          : lists of sub-pixel coordinates of edge points
     curve_limits : the limits of each curve in lists x and y
     N            : number of edge points
     M            : number of curves

   The input is a XxY graylevel image given as a pointer to an array of doubles
   such that image[x+y*X] is the value at coordinates x,y
   (for 0 <= x < X and 0 <= y < Y).

   The output are the chained edge points given as 3 allocated lists: x, y and
   curve_limits. Also the numbers N (size of lists x and y) and M (number of
   curves).

   x[i] and y[i] (0<=i<N) store the sub-pixel coordinates of edge points.
   curve_limits[j] (0<=j<=M) stores the limits of each chain in lists x and y.

   example:

     curve number k (0<=k<M) consists of the edge points x[i],y[i]
     for i determined by curve_limits[k] <= i < curve_limits[k+1].

     curve k is closed if x[curve_limits[k]] == x[curve_limits[k+1] - 1] and
                          y[curve_limits[k]] == y[curve_limits[k+1] - 1].
 */
void smooth_contours( double ** x, double ** y, int * N,
                      int ** curve_limits, int * M,
                      double * image, int X, int Y, double Q )
{
  double dog_rate = 1.6;    /* DoG sigma rate to approx. Laplacian of Gaussian
                               optimal value 1.6 [Marr-Hildreth 1980] */
  double sigma_step = 0.8;  /* sigma to sampling step rate in Gaussian sampling
                               optimal value 0.8 [Morel-Yu 2011] */
  double log_eps = 0.0;     /* log10(epsilon), where epsilon is the mean number
                               of false detections one can afford per image */
  int num_w = 3;            /* number of arc widths to be tested */
  double fac_w = sqrt(2.0); /* arc width factor  */
  double min_w = sqrt(2.0); /* minimal arc width */
  int W = 2 * num_w;        /* there are two operators per width:
                               with and without central gap */

  double max_w = min_w * pow( fac_w, (double) num_w-1.0 );
  double sigma = sigma_step * sqrt( dog_rate * dog_rate - 1.0 );
  struct region * reg = xmalloc( X * Y * sizeof(struct region) );
  double * diff = (double *) xmalloc( X * Y * sizeof(double) );
  int * meaningful = (int *) xmalloc( X * Y * sizeof(int) );
  int * used = (int *) xmalloc( X * Y * sizeof(int) );
  double * gauss;
  double * xx;
  double * yy;
  int * curve;
  struct arc_of_circle arc;
  arc.dir  = 0;
  int i,k,n,reg_n,c,NN,MM,min_l;
  double w;

  /* compute minimal arc length that may become meaningful */
  min_l = compute_min_length(X,Y,max_w,W,log_eps);

  /* filter the input image by a Gaussian filter and compute difference image */
  gauss = gaussian_filter(image, X, Y, sigma);
  for(n=0; n<X*Y; n++) diff[n] = image[n] - gauss[n];

  /* compute chained edge points */
  chained_subpixel_edge_points(&xx, &yy, &NN, &curve, &MM, gauss, X, Y);

  /* initialize all edge points as not meaningful and not used */
  for(n=0; n<NN; n++) meaningful[n] = used[n] = FALSE;

  /* a-contrario validation of curve segments approximated by local arcs */
  for(c=0; c<MM; c++)                         /* iterate on curves */
    for(i=curve[c]; i<curve[c+1]; i++)        /* first point of curve segment */
      for(k=curve[c+1]-1; (k-i)>=min_l; k--)  /* last point of curve segment  */
        if( !used[i] || !used[k] )   /* test segment only if one end not used */
          if( smooth_segment(&arc,xx,yy,i,k,sigma,max_w,X,Y) )
            {
              /* heuristic to speed up: mark as used center part of segment */
              for(n=i+3; n<=(k-3); n++) used[n] = TRUE;

              /* get pixel values in lateral regions to the arc. to speed-up
                 it is done only once, thus outside w-loop and arc_log_nfa() */
              get_region(&reg_n, reg, diff, X, Y, &arc, max_w, Q);
              if( reg_n <= 0 ) continue; /* empty region */

              /* lateral width loop. two gaps are tried for each: 0.0 and 1.0 */
              for(w=min_w; w<=max_w; w*=fac_w)
                if( arc_log_nfa(X,Y,&arc,w,reg_n,reg,W,0.0) < log_eps ||
                    arc_log_nfa(X,Y,&arc,w,reg_n,reg,W,1.0) < log_eps )
                  {
                    for(n=i; n<=k; n++) meaningful[n] = used[n] = TRUE;
                    break; /* arc already meaningful, no need to test other w */
                  }
            }

  keep_meaningful_curves(x,y,N,curve_limits,M,xx,yy,NN,curve,MM,meaningful);

  /* free memory */
  free( (void *) gauss );
  free( (void *) diff );
  free( (void *) meaningful );
  free( (void *) used );
  free( (void *) xx );
  free( (void *) yy );
  free( (void *) curve );
  free( (void *) reg );
}
/*----------------------------------------------------------------------------*/
