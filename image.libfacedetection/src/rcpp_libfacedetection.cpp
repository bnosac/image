#include <Rcpp.h>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "facedetectcnn.h"
using namespace Rcpp;

#define DETECT_BUFFER_SIZE 0x20000

// [[Rcpp::export]]
Rcpp::List detect_faces(IntegerVector x, int width, int height, int step) {
  int nr;
  std::vector<int> rect_x;
  std::vector<int> rect_y;
  std::vector<int> rect_w;
  std::vector<int> rect_h;
  std::vector<int> rect_neighbors;
  std::vector<int> rect_angle;

  std::vector<unsigned char> image;
  for (int i = 0; i < x.size(); i++){
    image.push_back ((unsigned char)x[i]);
  }

  int * pResults = NULL; 
  //pBuffer is used in the detection functions.
  //If you call functions in multiple threads, please create one buffer for each thread!
  unsigned char * pBuffer = (unsigned char *)malloc(DETECT_BUFFER_SIZE);
  pResults = facedetect_cnn(pBuffer, image.data(), width, height, step);
  
  nr = (pResults ? *pResults : 0);
  for(int i = 0; i < nr; i++)
  {
    short * p = ((short*)(pResults+1))+142*i;
    int x = p[0];
    int y = p[1];
    int w = p[2];
    int h = p[3];
    int neighbors = p[4];
    int angle = p[5];
    
    rect_x.push_back (x);
    rect_y.push_back (y);
    rect_w.push_back (w);
    rect_h.push_back (h);
    rect_neighbors.push_back (neighbors);
    rect_angle.push_back (angle);
  }
  free(pBuffer);
  Rcpp::List output = Rcpp::List::create(
    Rcpp::Named("nr") = nr,
    Rcpp::Named("detections") = Rcpp::DataFrame::create(
      Rcpp::Named("x") = rect_x, 
      Rcpp::Named("y") = rect_y,
      Rcpp::Named("width") = rect_w,
      Rcpp::Named("height") = rect_h,
      Rcpp::Named("neighbours") = rect_neighbors,
      Rcpp::Named("angle") = rect_angle));
  return output;
}
