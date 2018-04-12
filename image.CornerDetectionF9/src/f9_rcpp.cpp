#include <Rcpp.h>
#include "f9.h"
#include <iostream>
using namespace Rcpp;


// [[Rcpp::export]]
List detect_corners(IntegerVector x, int width, int height, int bytes_per_row, bool suppress_non_max = false, unsigned char threshold = 4) {
  //unsigned char image_data[x.size()];
  unsigned char *image_data = new unsigned char[x.size()];
  for(int i = 0; i < x.size(); i++) image_data[i] = (unsigned char)x[i];
  const unsigned char * img;
  img = reinterpret_cast<const unsigned char*>(image_data);
  F9* f9detector = new F9();
  //int width = 512;
  //int height = 512;
  //int bytes_per_row = 512;
  //bool suppress_non_max = false;
  const std::vector<F9_CORNER> out = f9detector -> detectCorners(img,
                                                                 width,
                                                                 height,
                                                                 bytes_per_row,
                                                                 threshold,
                                                                 suppress_non_max);
  int N = out.size();
  NumericVector corners_x(N);
  NumericVector corners_y(N);
  for(int i = 0; i < N; i++){
    corners_x[i] = out[i].y;
    corners_y[i] = width - out[i].x;
  }
  delete[] image_data;
  List z = List::create(corners_x, corners_y);
  return z;
}
