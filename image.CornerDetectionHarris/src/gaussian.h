// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.


#ifndef _GAUSSIAN_H
#define _GAUSSIAN_H


#define STD_GAUSSIAN 0
#define FAST_GAUSSIAN 1
#define NO_GAUSSIAN 2

/**
 *
 * Convolution with a Gaussian 
 *
 */
void gaussian(
  float *I,    //input image
  float *Is,   //output image
  int   nx,    //image width
  int   ny,    //image height
  float sigma, //Gaussian sigma
  int   type=FAST_GAUSSIAN, //type of Gaussian convolution 
  int   K=3    //defines the number of iterations or window precision
);


#endif
