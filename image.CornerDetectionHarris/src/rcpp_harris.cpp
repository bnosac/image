#include <Rcpp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h> 
#include <math.h>
#include <float.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "harris.h"
#include "gaussian.h"
#include "gradient.h"
#include "interpolation.h"


// [[Rcpp::export]]
SEXP detect_corners(Rcpp::NumericVector x, int nx, int ny, 
                    float k=0.060000,
                    float sigma_d=1.000000,
                    float sigma_i=2.500000, 
                    float threshold=130,
                    int gaussian=1,
                    int gradient=0,
                    int strategy=0, 
                    int Nselect=1,
                    int measure=0,
                    int Nscales=1,
                    int precision=1,
                    int cells=10,
                    int verbose=1) {
  std::vector<harris_corner> corners;
  float *I=new float[nx*ny];
  for(int i = 0; i < x.size(); i++) I[i] = (float)x[i];
  
  harris_scale(
    I, corners, Nscales, gaussian, gradient, measure, k, 
    sigma_d, sigma_i, threshold, strategy, cells, Nselect, 
    precision, nx, ny, verbose
  );
  
  unsigned int nr_corners = corners.size();
  std::vector<float> loc_x;
  std::vector<float> loc_y;
  std::vector<float> loc_strength;
  for (unsigned int i = 0; i < nr_corners; i++){
    loc_x.push_back(corners[i].x);
    loc_y.push_back(corners[i].y);
    loc_strength.push_back(corners[i].R);
  }
  
  Rcpp::List out = Rcpp::List::create(
    Rcpp::Named("x") = loc_x,
    Rcpp::Named("y") = loc_y,
    Rcpp::Named("strength") = loc_strength
  );
  return out;
}

