// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.


#ifndef _GRADIENT_H
#define _GRADIENT_H


#define CENTRAL_DIFFERENCES 0
#define SOBEL_OPERATOR 1

/**
  *
  * Function to compute the gradient
  *
**/
void gradient(
  float *I,  //input image
  float *Ix, //computed x derivative
  float *Iy, //computed y derivative
  int   nx,  //image width
  int   ny,  //image height
  int   type //type of gradient
);

#endif
