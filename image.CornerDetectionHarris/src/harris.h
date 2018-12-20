// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.



#ifndef _HARRIS_H
#define _HARRIS_H

#include<vector>

//Measures for the corner strength function
#define HARRIS_MEASURE 0
#define SHI_TOMASI_MEASURE 1
#define HARMONIC_MEAN_MEASURE 2

//Strategies for selecting output corners
#define ALL_CORNERS 0
#define ALL_CORNERS_SORTED 1
#define N_CORNERS 2
#define DISTRIBUTED_N_CORNERS 3


/**
  *
  * Structure for handling corners
  *
**/
struct harris_corner{
  float x,y; //position of the corner
  float R;   //corner strength

  harris_corner(float xx, float yy, float RR)
  {
    x=xx; y=yy; R=RR;
  }  
  
  harris_corner()
  {
    x=0; y=0; R=0;
  }
};


/**
  *
  * Function for non-maximum suppression
  *
**/
void non_maximum_suppression(
  float *R,             //input data
  std::vector<harris_corner> &corners, //Harris' corners
  float Th,             //threshold for low values
  int   radius,         //window radius
  int   nx,             //number of columns of the image
  int   ny              //number of rows of the image
);


/**
  *
  * Function for selecting the output corners
  *
**/    
void select_output_corners(
  std::vector<harris_corner> &corners, //output selected corners
  int strategy, //strategy for the output corners
  int cells,    //number of regions in the image for distributed output
  int N,        //number of output corners
  int nx,       //number of columns of the image
  int ny        //number of rows of the image
);


/**
  *
  * Function for computing subpixel precision of maxima
  *
**/
void compute_subpixel_precision(
  float *R, //discriminant function
  std::vector<harris_corner> &corners, //selected corners
  int nx,   //number of columns of the image
  int type  //type of interpolation (quadratic or quartic)
);



/**
  *
  * Main function for computing Harris corners
  *
**/
void harris(
  float *I,        //input image
  std::vector<harris_corner> &corners, //output selected corners
  int   gauss,     //type of Gaussian 
  int   grad,      //type of gradient
  int   measure,   //measure for the discriminant function
  float k,         //Harris constant for the ....function
  float sigma_d,   //standard deviation for smoothing (image denoising)    
  float sigma_i,   //standard deviation for smoothing (pixel neighbourhood)
  float Th,        //threshold for eliminating low values
  int   strategy,  //strategy for the output corners
  int   cells,     //number of regions in the image for distributed output
  int   N,         //number of output corners
  int   precision, //type of subpixel precision approximation
  int   nx,        //number of columns of the image
  int   ny,        //number of rows of the image
  int   verbose    //activate verbose mode
);


/**
  *
  * Main function for computing Harris corners with scale test
  *
**/
void harris_scale(
  float *I,        //input image
  std::vector<harris_corner> &corners, //output selected corners
  int   Nscales,   //number of scales for checking the stability of corners
  int   gauss,     //type of Gaussian 
  int   grad,      //type of gradient
  int   measure,   //measure for the discriminant function
  float k,         //Harris constant for the ....function
  float sigma_d,   //standard deviation for smoothing (image denoising)    
  float sigma_i,   //standard deviation for smoothing (pixel neighbourhood)
  float Th,        //threshold for eliminating low values
  int   strategy,  //strategy for the output corners
  int   cells,     //number of regions in the image for distributed output
  int   N,         //number of output corners
  int   precision, //type of subpixel precision approximation
  int   nx,        //number of columns of the image
  int   ny,        //number of rows of the image
  int   verbose    //activate verbose mode
);

#endif
