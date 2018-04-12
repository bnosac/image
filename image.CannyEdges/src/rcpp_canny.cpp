#include <Rcpp.h>
using namespace Rcpp;

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "adsf.h"
#include "canny.h"
}

// Mirroring
int mirror(int x, int y, size_t nx, size_t ny) {
  int xt, yt;
  if (x < 0)
    xt = -x;
  else
    if (x > (int)nx -1)
      xt = 2*nx - 2 - x;
    else
      xt = x;
    if (y < 0)
      yt = -y;
    else
      if (y > (int)ny - 1)
        yt = 2*ny - 2 - y;
      else
        yt = y;
      return xt + nx*yt;

}

// extention of the border values
int extend(int x, int y, size_t nx, size_t ny) {
  int xt, yt;
  if (x < 0)
    xt = 0;
  else
    if (x > (int)nx -1)
      xt = nx - 1;
    else
      xt = x;
    if (y < 0)
      yt = 0;
    else
      if (y > (int)ny - 1)
        yt = ny - 1;
      else
        yt = y;
      return xt + nx*yt;
}

// out-of-image points value computation
int value(int x, int y, size_t nx, size_t ny) {
  // We use the same value as the border one
  return extend(x,y,nx,ny);
  //return mirror(x,y,nx,ny)
}

// Specific bilinear interpolation
double bilin(double* grad, double t, size_t x, size_t y, size_t nx, size_t ny, int dir) {
  double x1,x2,xt,y1,y2,yt;

  // Here are the points where we would like to evaluate grad
  xt = dir * cos(t);
  yt = dir * sin(t);
  // and the points where we are able to evaluate grad
  x1 = floor(xt), x2 = x1 + 1;
  y1 = floor(yt), y2 = y1 + 1;

  //Interpolation in the x direction
  // y = y1 :
  double gradx1 = (x2 - xt) * grad[value(x + x1, y + y1, nx, ny)]
  + (xt - x1) * grad[value(x + x2, y + y1, nx, ny)];
  // y = y2 :
  double gradx2 = (x2 - xt) * grad[value(x + x1, y + y2, nx, ny)]
  + (xt - x1) * grad[value(x + x2, y + y2, nx, ny)];
  // Interpolation in the y direction
  double gradxy = (y2 - yt) * gradx1 + (yt - y1) * gradx2;
  return gradxy;
}

//Computation of the maxima of the gradient
static void maxima(double* grad, double *theta, unsigned char *output, size_t nx, size_t ny, int low_thr,int high_thr) {
  for(size_t x = 0 ; x < nx ; x++) {
    for(size_t y = 0 ; y < ny ; y++) {
      double t = theta[y*nx + x];

      double prev = bilin(grad,t,x,y,nx,ny,-1);
      double next = bilin(grad,t,x,y,nx,ny,1);
      double now = grad[y*nx+x];

      if ((now <= prev) || (now <= next) || (now <= low_thr))
        // If it is not a local maximum or is below the low threshold, discard
        output[y*nx + x] = 0;
      else if (now >= high_thr)
        output[y*nx + x] = 2;
      else
        output[y*nx + x] = 1;
    }
  }
}

// static const char *help =
//   "canny usage:\n"
//   "\t-h | --help          Display this help message\n"
//   "\t-v | --version	Display the version of this program\n"
//   "\t-s | --sigma   DBLE  Gaussian filter's variance\n"
//   "\t-lt	          DBLE  Low threshold\n"
//   "\t-ht	          DBLE 	High threshold\n"
//   "\t-a |	          	triggers higher-order gradient\n"
//   "\t-i | --input   FILE  Input file\n"
//   "\t-o | --output  FILE  Output file\n"
//   ;


// [[Rcpp::export]]
List canny_edge_detector(IntegerVector image, int X, int Y,
                          double s = 2,
                          double low_thr = 3,
                          double high_thr = 10,
                          bool accGrad = false)
{
  size_t nx, ny;		// data size
  nx=X;
  ny=Y;
  //double s = 2; 		//s : sigma, filter variance
  //double low_thr=3, high_thr = 10;		// Thresholds
  //bool accGrad = false;

  //unsigned char input[image.size()];
  unsigned char *input = new unsigned char[image.size()];
  for(int i = 0; i < image.size(); i++) input[i] = (unsigned char)image[i];

  double* in = (double *) xmalloc(sizeof(double) * nx * ny);

  for(size_t d = 0 ; d < nx*ny ; d++)
    in[d] = (double) input[d];
  double* data = (double *) xmalloc(sizeof(double) * nx * ny);

  // Gaussian filtering
  gblur(data, in, nx, ny, 1, s);
  free(in);
  double* grad = (double *) xmalloc(sizeof(double) * nx * ny);
  double* theta = (double *) xmalloc(sizeof(double) * nx * ny);
  double hgrad,vgrad;

  // Gradient value :
  for(size_t x = 0 ; x < nx ; x++) {
    for(size_t y = 0 ; y < ny ; y++) {
      if(accGrad) {
        //horizontal gradient
        hgrad = 2*(data[value(x+1,y,nx,ny)] - data[value(x-1,y,nx,ny)]) +
          data[value(x+1,y+1,nx,ny)] - data[value(x-1,y+1,nx,ny)] +
          data[value(x+1,y-1,nx,ny)] - data[value(x-1,y-1,nx,ny)];
        //vertical gradient
        vgrad = 2*(data[value(x,y+1,nx,ny)] - data[value(x,y-1,nx,ny)]) +
          data[value(x+1,y+1,nx,ny)] - data[value(x+1,y-1,nx,ny)] +
          data[value(x-1,y+1,nx,ny)] - data[value(x-1,y-1,nx,ny)] ;
      }
      else {
        //horizontal gradient
        hgrad = data[value(x+1,y,nx,ny)] - data[value(x-1,y,nx,ny)];
        //vertical gradient
        vgrad = data[value(x,y+1,nx,ny)] - data[value(x,y-1,nx,ny)];
      }
      // Gradient norm and dirextion
      grad[y*nx+x] = hypot(hgrad,vgrad);
      theta[y*nx + x] = atan2(vgrad,hgrad);
    }
  }
  unsigned char *output = NULL;
  output = (unsigned char *) xmalloc(sizeof(unsigned char) * nx * ny);

  //Suppression of non-maxima
  maxima(grad,theta,output,nx,ny,low_thr,high_thr);

  int N = nx*ny;

  int* t = (int *) xmalloc(sizeof(int) * nx * ny);

  // Building the trees
  adsf_begin(t,N);
  for(size_t x = 0 ; x < nx ; x++)
    for(size_t y = 0 ; y < ny ; y++) {
      int d = x + nx * y;
      if(output[d]) {
        for(int ex = -1 ; ex < 2 ; ex++)
          for(int ey = -1 ; ey < 2 ; ey++) {
            int ed = value(x+ex,y+ey,nx,ny);
            // If the neighbour is marked, we make the union
            if(output[ed])
              adsf_union(t,N,d,ed);
          }
      }
    }

    // We mark every tree having a node higher than the high threshold
    for(int d = 0 ; d < N ; d++)
      if(output[d] == 2)
        output[adsf_find(t,N,d)] = 2;

      adsf_assert_consistency(t,N);

      // We remove every tree which root is not marked
      for(int d = 0 ; d < N ; d++)
        if(output[adsf_find(t,N,d)]  < 2)
          output[d] = 0;
        else
          // Very high value so it can be seen
          output[d] = (char) -1;
        free(t);

        free(grad);
        free(theta);
        free(data);





  NumericMatrix out_r(Dimension(nx, ny));
  int nonzero = 0;
  for(int i = 0; i < int(nx * ny); i++){
    out_r[i] = output[i];
    if(output[i] == 255){
      nonzero = nonzero + 1;
    }
  }
  free(output);
  delete[] input;
  List z = List::create(_["edges"] = out_r,
                        _["pixels_nonzero"] = nonzero,
                        _["nx"] = nx,
                        _["ny"] = ny,
                        _["s"] = s,
                        _["low_thr"] = low_thr,
                        _["high_thr"] = high_thr,
                        _["accGrad"] = accGrad);
  return z;
}



