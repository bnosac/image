#include <Rcpp.h>
using namespace Rcpp;

extern "C" {
#include "smooth_contours.h"
}

// [[Rcpp::export]]
List detect_contours(NumericVector image, int X, int Y, double Q = 2.0)
{
  double * x;          /* x[n] y[n] coordinates of result contour point n */
  double * y;
  int * curve_limits;  /* limits of the curves in the x[] and y[] */
  int N,M;         /* result: N contour points, forming M curves */
  //double Q = 2.0; /* default Q=2        */
  //double W = 1.3; /* PDF line width 1.3 */
  
  smooth_contours(&x, &y, &N, &curve_limits, &M, image.begin(), X, Y, Q);
  
  NumericVector contour_x(N);
  for(int i = 0; i < N; i++) contour_x[i] = x[i];
  NumericVector contour_y(N);
  for(int i = 0; i < N; i++) contour_y[i] = y[i];
  NumericVector curvelimits(M);
  for(int i = 0; i < M; i++) curvelimits[i] = curve_limits[i];
  
  List z = List::create(contour_x, contour_y, curvelimits, M, N);
  return z;
}



