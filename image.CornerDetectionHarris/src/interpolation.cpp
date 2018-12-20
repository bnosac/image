// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.


#include "interpolation.h"

#include <stdio.h>
#include <math.h>

#define MAX_ITERATIONS 20


/**
  *
  * QUADRATIC APPROXIMATION
  *
  * Approximate function as
  * 
  * f(x)=f + Df^T x + 1/2 x^T DDf x
  *
**/
bool quadratic_approximation(
  float *M,  //values of the interpolation function (9 values)
  float &x,  //corner x-position
  float &y,  //corner y-position
  float &Mo  //maximum of the interpolation function
)
{
  float fx  = 0.5*(M[5]-M[3]);
  float fy  = 0.5*(M[7]-M[1]);
  float fxx = (M[5]-2*M[4]+M[3]);
  float fyy = (M[7]-2*M[4]+M[1]);
  float fxy = 0.25*(M[0]-M[2]-M[6]+M[8]);
  
  float det = fxx*fyy-fxy*fxy;
  
  //check invertibility
  if(det*det<1E-6)
    return false;
  else
  {
    //compute the new position and value
    float dx = (fyy*fx-fxy*fy)/det;
    float dy = (fxx*fy-fxy*fx)/det;
    x-=dx; y-=dy;
    Mo=M[4]+fx*dx+fy*dy+0.5*(fxx*dx*dx+2*dx*dy*fxy+fyy*dy*dy);
    return true;
  }
}




/**
  *
  * QUARTIC INTERPOLATION
  *
  * Evaluate function in (x,y)
  * 
  * f(x)=a0 x^2y^2 + a1 x^2y + a2 xy^2 + a3 x^2 + a4 y^2 +
  *      a5 xy + a6 x + a7 y + a8
  *
**/
float f(
  float *a, //polynomial coefficients
  float x,  //x value
  float y   //y value
)
{
  return a[0]*x*x*y*y+a[1]*x*x*y+a[2]*x*y*y+
         a[3]*x*x+a[4]*y*y+a[5]*x*y+a[6]*x+a[7]*y+a[8];
}


/**
  *
  * Compute the coefficients of the interpolation function
  * 
  * f(x)=a0 x^2y^2 + a1 x^2y + a2 xy^2 + a3 x^2 + a4 y^2 +
  *      a5 xy + a6 x + a7 y + a8
  *
**/
void polynomial_coefficients(
  float *M, //values of the interpolation function
  float *a  //output polynomial coefficients
)
{
  a[0]=M[4]-0.5*(M[1]+M[3]+M[5]+M[7])+0.25*(M[0]+M[2]+M[6]+M[8]);
  a[1]=0.5*(M[1]-M[7])+0.25*(-M[0]-M[2]+M[6]+M[8]);
  a[2]=0.5*(M[3]-M[5])+0.25*(-M[0]+M[2]-M[6]+M[8]);
  a[3]=0.5*(M[3]+M[5])-M[4];
  a[4]=0.5*(M[1]+M[7])-M[4];
  a[5]=0.25*(M[0]-M[2]-M[6]+M[8]);
  a[6]=0.5*(M[5]-M[3]);
  a[7]=0.5*(M[7]-M[1]);
  a[8]=M[4];
}


/**
  *
  * Compute the Gradient of the interpolation function
  * 
  *
**/
void polynomial_gradient(
  float dx, //current x-position
  float dy, //current y-position
  float *a, //polynomial coefficients
  float *D  //output Hessian
)
{
  D[0]=2*a[0]*dx*dy*dy+2*a[1]*dx*dy+2*a[2]*dy*dy+2*a[3]*dx+a[5]*dy+a[6];
  D[1]=2*a[0]*dx*dx*dy+2*a[1]*dx*dx+2*a[2]*dx*dy+2*a[4]*dy+a[5]*dx+a[7];
}


/**
  *
  * Compute the Hessian of the interpolation function
  * 
  *
**/
void Hessian(
  float dx, //current x-position
  float dy, //current y-position
  float *a, //polynomial coefficients
  float *H  //output Hessian
)
{
  H[0]=2*a[0]*dy*dy+2*a[1]*dy+2*a[3];
  H[1]=4*a[0]*dx*dy+2*a[1]*dx+2*a[2]*dy+a[5];
  H[2]=2*a[0]*dx*dx+2*a[2]*dx+2*a[4];
}


/**
  *
  * Solve the system H^-1*D
  * 
  *
**/
bool solve(
  float *H, //Hessian
  float *D, //gradient
  float *b  //output increment
)
{
  float det=H[0]*H[2]-H[1]*H[1];
  
  if(det*det<1E-10)
    return false;
  else{
    b[0]=(D[0]*H[2]-D[1]*H[1])/det;
    b[1]=(D[1]*H[0]-D[0]*H[1])/det;
    return true;
  }
}


/**
  *
  * Apply Newton method to find maximum of the interpolation function
  *
**/
bool quartic_interpolation(
  float *M,  //values of the interpolation function (9 values)
  float &x,  //corner x-position
  float &y,  //corner y-position
  float &Mo, //maximum of the interpolation function
  float TOL  //stopping criterion threshold
)
{
  float D[2],b[2],H[3],a[9];
  
  float dx=0, dy=0;
  polynomial_coefficients(M,a);
  
  int i=0;
  do
  {
    //compute the gradient and Hessian in the current position
    polynomial_gradient(dx,dy,a,D);
    Hessian(dx,dy,a,H);
    
    //solve the system for estimating the next increment
    if(!solve(H,D,b))
      return false;
    
    //move the current position
    dx-=b[0];
    dy-=b[1];        
    
    i++;
  }
  while(D[0]*D[0]+D[1]*D[1]>TOL && i<MAX_ITERATIONS);
  
  //check that (dx, dy) are inside the allowed boundaries 
  if(dx>1 || dx<-1 || dy>1 || dy<-1 || isnan(dx) || isnan(dy))
    return false;    
  else
  {
    //compute the new position and value
    x+=dx; y+=dy; Mo=f(a, dx, dy);
    return true;
  }
}


