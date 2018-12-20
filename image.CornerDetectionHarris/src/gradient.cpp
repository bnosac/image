// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.


#include "gradient.h"

/**
  *
  * Function to compute the gradient with central differences
  *
**/
void central_differences(
  float *I,  //input image
  float *dx, //computed x derivative
  float *dy, //computed y derivative
  int   nx,  //image width
  int   ny   //image height
)
{
    //compute gradient in the center body
    #pragma omp parallel for
    for(int i=1; i<ny-1; i++)
    {
        for(int j=1; j<nx-1; j++)
        {
            int p = i*nx+j;
            dx[p] = 0.5*(I[p+1]-I[p-1]);
            dy[p] = 0.5*(I[p+nx]-I[p-nx]);
        }
    }

    //copy first and last rows
    for(int i=1; i<nx-1; i++)
    {
      dx[i]=dx[i+nx];
      dx[nx*(ny-1)+i]=dx[nx*(ny-2)+i];
      dy[i]=dy[i+nx];
      dy[nx*(ny-1)+i]=dy[nx*(ny-2)+i];
    }

    //copy first and last columns
    for(int i=0; i<ny; i++)
    {
      dx[i*nx]=dx[i*nx+1];
      dx[(i+1)*nx-1]=dx[(i+1)*nx-2];
      dy[i*nx]=dy[i*nx+1];
      dy[(i+1)*nx-1]=dy[(i+1)*nx-2];
    }
}

/**
  *
  * Function to compute the gradient using the Sobel operator
  *
**/
void sobel_operator(
  float *I,  //input image
  float *dx, //computed x derivative
  float *dy, //computed y derivative
  int   nx,  //image width
  int   ny   //image height
)
{
    //compute gradient in the center body
    #pragma omp parallel for
    for(int i=1; i<ny-1; i++)
    {
        for(int j=1; j<nx-1; j++)
        {
            int p = i*nx+j;
            dx[p] = 1./4.*(I[p+1]-I[p-1])+
                    1./8.*(I[p-nx+1]+I[p+nx+1]-
                           I[p-nx-1]-I[p+nx-1]);
            dy[p] = 1./4.*(I[p+nx]-I[p-nx])+
                    1./8.*(I[p+nx+1]+I[p+nx-1]-
                           I[p-nx+1]-I[p-nx-1]);
        }
    }

    //copy first and last rows
    for(int i=1; i<nx-1; i++)
    {
      dx[i]=dx[i+nx];
      dx[nx*(ny-1)+i]=dx[nx*(ny-2)+i];
      dy[i]=dy[i+nx];
      dy[nx*(ny-1)+i]=dy[nx*(ny-2)+i];
    }

    //copy first and last columns
    for(int i=0; i<ny; i++)
    {
      dx[i*nx]=dx[i*nx+1];
      dx[(i+1)*nx-1]=dx[(i+1)*nx-2];
      dy[i*nx]=dy[i*nx+1];
      dy[(i+1)*nx-1]=dy[(i+1)*nx-2];
    }
}



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
)
{
  if(type==SOBEL_OPERATOR)
    sobel_operator(I, Ix, Iy, nx, ny);
  else
    central_differences(I, Ix, Iy, nx, ny);
}
