#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libdenoising.h"

#include <Rcpp.h>
using namespace Rcpp;


void fiAddNothing(float *u, float *v, int size) {
  for (int i=0; i< size; i++) {
    v[i] =  u[i];
  }
}

// [[Rcpp::plugins(cpp11)]]
// [[Rcpp::export]]
Rcpp::List rcpp_nlmeans(IntegerVector image, int width, int height, int channels, float sigma, bool args_auto = true, int win = 1, int bloc = 10, float fFiltPar = 0.4) {
  // read input
  size_t nx=width,ny=height,nc=channels;
  std::vector<float> _d_v = Rcpp::as<std::vector<float> >(image);
  float *d_v = &_d_v[0];
  
  // variables
  int d_w = (int) nx;
  int d_h = (int) ny;
  int d_c = (int) nc;
  if (d_c == 2) {
    d_c = 1;    // we do not use the alpha channel
  }
  if (d_c > 3) {
    d_c = 3;    // we do not use the alpha channel
  }
  
  int d_wh = d_w * d_h;
  int d_whc = d_c * d_w * d_h;
  
  
  
  // test if image is really a color image even if it has more than one channel
  if (d_c > 1) {
    
    // dc equals 3
    int i=0;
    while (i < d_wh && d_v[i] == d_v[d_wh + i] && d_v[i] == d_v[2 * d_wh + i ])  {
      i++;
    }
    
    if (i == d_wh) d_c = 1;
    
  }

  
  // denoise
  float fSigma = sigma;
  float *noisy = new float[d_whc];
  for (int i=0; i < d_c; i++) {
    fiAddNothing(&d_v[i * d_wh], &noisy[i * d_wh], d_wh);
  }
  // denoise
  float **fpI = new float*[d_c];
  float **fpO = new float*[d_c];
  float *denoised = new float[d_whc];
  
  for (int ii=0; ii < d_c; ii++) {
    
    fpI[ii] = &noisy[ii * d_wh];
    fpO[ii] = &denoised[ii * d_wh];
    
  }
  
  
  
  if(args_auto == true){
    //int bloc, win;
    //float fFiltPar;
    
    if (d_c == 1) {
      
      if (fSigma > 0.0f && fSigma <= 15.0f) {
        win = 1;
        bloc = 10;
        fFiltPar = 0.4f;
        
      } else if ( fSigma > 15.0f && fSigma <= 30.0f) {
        win = 2;
        bloc = 10;
        fFiltPar = 0.4f;
        
      } else if ( fSigma > 30.0f && fSigma <= 45.0f) {
        win = 3;
        bloc = 17;
        fFiltPar = 0.35f;
        
      } else if ( fSigma > 45.0f && fSigma <= 75.0f) {
        win = 4;
        bloc = 17;
        fFiltPar = 0.35f;
        
      } else if (fSigma <= 100.0f) {
        
        win = 5;
        bloc = 17;
        fFiltPar = 0.30f;
        
      } else {
        Rcpp::stop("error :: algorithm parametrized only for values of sigma less than 100.0");
      }
      
      
      
      
      
    } else {
      
      
      if (fSigma > 0.0f && fSigma <= 25.0f) {
        win = 1;
        bloc = 10;
        fFiltPar = 0.55f;
        
      } else if (fSigma > 25.0f && fSigma <= 55.0f) {
        win = 2;
        bloc = 17;
        fFiltPar = 0.4f;
        
      } else if (fSigma <= 100.0f) {
        win = 3;
        bloc = 17;
        fFiltPar = 0.35f;
        
      } else {
        Rcpp::stop("error :: algorithm parametrized only for values of sigma less than 100.0");
      }
      
      
      
    }
  }

  nlmeans_ipol(win, bloc, fSigma, fFiltPar, fpI,  fpO, d_c, d_w, d_h);
  
  std::vector<double> result_denoised;
  for(int i=0; i<(d_whc); i++){
    result_denoised.push_back(denoised[i]);
  }    
  Rcpp::List output = Rcpp::List::create(Rcpp::Named("width") = d_w,
                                         Rcpp::Named("height") = d_h,
                                         Rcpp::Named("channels") = d_c,
                                         Rcpp::Named("denoised") = result_denoised,
                                         Rcpp::Named("sigma") = fSigma,    // Noise parameter
                                         Rcpp::Named("filter") = fFiltPar, // Filtering parameter
                                         Rcpp::Named("window") = win,      // Half size of comparison window
                                         Rcpp::Named("bloc") = bloc);      // Half size of research window
  return output;
}


