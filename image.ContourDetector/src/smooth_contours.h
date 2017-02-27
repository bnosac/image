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
#ifndef SMOOTH_CONTOURS_HEADER
#define SMOOTH_CONTOURS_HEADER

/*----------------------------------------------------------------------------*/
#define SMOOTH_CONTOURS_VERSION "1.0 (August 26, 2016)"

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
                      double * image, int X, int Y, double Q );

#endif /* !SMOOTH_CONTOURS_HEADER */
/*----------------------------------------------------------------------------*/
