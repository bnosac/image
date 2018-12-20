// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.


#ifndef INTERPOLATION
#define INTERPOLATION

#define NO_INTERPOLATION        0
#define QUADRATIC_APPROXIMATION 1
#define QUARTIC_INTERPOLATION   2

/**
  *
  * Find maximum of a quadratic approximation function
  *
**/
bool quadratic_approximation(
  float *M,  //values of the interpolation function (9 values)
  float &x,  //corner x-position
  float &y,  //corner y-position
  float &Mo  //maximum of the interpolation function
);

/**
  *
  * Find maximum of a quartic interpolation function
  *
**/
bool quartic_interpolation(
  float *M,  //values of the interpolation function (9 values)
  float &x,  //corner x-position
  float &y,  //corner y-position
  float &Mo, //maximum of the interpolation function
  float TOL=1E-10  //stopping criterion threshold
);


#endif
