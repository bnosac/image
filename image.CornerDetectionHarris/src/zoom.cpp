// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.


/**
  *
  * Boundary condition
  *
**/
int bc(int x, int nx)
{
  if(x<0) x=0;
  else if (x>=nx) x=nx-1;
  
  return x;
}


/**
  *
  * Cubic interpolation in one dimension
  *
**/
static double cubic_interpolation(
  double v[4],  //interpolation points
  double x      //point to be interpolated
)
{
  return  v[1] + 0.5 * x * (v[2] - v[0] +
          x * (2.0 *  v[0] - 5.0 * v[1] + 4.0 * v[2] - v[3] +
          x * (3.0 * (v[1] - v[2]) + v[3] - v[0])));
}


/**
  *
  * Bicubic interpolation
  *
**/
static double bicubic_interpolation (
  double p[4][4], //array containing the interpolation points
  double x,       //x position to be interpolated
  double y        //y position to be interpolated
)
{
  double v[4];
  v[0] = cubic_interpolation(p[0], y);
  v[1] = cubic_interpolation(p[1], y);
  v[2] = cubic_interpolation(p[2], y);
  v[3] = cubic_interpolation(p[3], y);
  return cubic_interpolation(v, x);
}

/**
  *
  * Compute the bicubic interpolation of a point in an image.
  *
**/
float bicubic_interpolation_at(
  float *input, //image to be interpolated
  float  uu,    //x component 
  float  vv,    //y component 
  int    nx,    //image width
  int    ny     //image height
)
{
  //apply boundary conditions
  int x   = bc((int) uu, nx);
  int y   = bc((int) vv, ny);
  int mx  = bc((int) uu - 1, nx);
  int my  = bc((int) vv - 1, ny);
  int dx  = bc((int) uu + 1, nx);
  int dy  = bc((int) vv + 1, ny);
  int ddx = bc((int) uu + 2, nx);
  int ddy = bc((int) vv + 2, ny);

  //obtain the interpolation points of the image
  float p11 = input[mx  + nx * my];
  float p12 = input[x   + nx * my];
  float p13 = input[dx  + nx * my];
  float p14 = input[ddx + nx * my];

  float p21 = input[mx  + nx * y];
  float p22 = input[x   + nx * y];
  float p23 = input[dx  + nx * y];
  float p24 = input[ddx + nx * y];

  float p31 = input[mx  + nx * dy];
  float p32 = input[x   + nx * dy];
  float p33 = input[dx  + nx * dy];
  float p34 = input[ddx + nx * dy];

  float p41 = input[mx  + nx * ddy];
  float p42 = input[x   + nx * ddy];
  float p43 = input[dx  + nx * ddy];
  float p44 = input[ddx + nx * ddy];

  //create array
  double pol[4][4] = {
	  {p11, p21, p31, p41},
	  {p12, p22, p32, p42},
	  {p13, p23, p33, p43},
	  {p14, p24, p34, p44}
  };

  //return interpolation
  return bicubic_interpolation(pol, uu-x, vv-y);
}


/**
  *
  * Zoom out an image by a factor of 2
  *
**/
float *zoom_out(
  float *I, //input image
  int nx,   //image width
  int ny    //image height
)
{
  int nxx=nx/2, nyy=ny/2;
  float *Iz=new float[nxx*nyy];
  
  //zoom out the image using bicubic interpolation
  #pragma omp parallel for
  for (int i1=0; i1<nyy; i1++)
    for (int j1=0; j1<nxx; j1++)
      Iz[i1*nxx+j1]=bicubic_interpolation_at(I, j1*2, i1*2, nx, ny);

  return Iz;
}

