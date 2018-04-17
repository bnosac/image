/*
 * Copyright (c) 2009-2011, A. Buades <toni.buades@uib.es>,
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _LIBAUXILIAR_H_
#define _LIBAUXILIAR_H_


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cmath>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cassert>




#define MAX(i,j) ( (i)<(j) ? (j):(i) )
#define MIN(i,j) ( (i)<(j) ? (i):(j) )


#define dTiny 1e-10
#define fTiny 0.00000001f
#define fLarge 100000000.0f


using namespace std;





///// LUT tables
#define LUTMAX 30.0
#define LUTMAXM1 29.0
#define LUTPRECISION 1000.0

void  wxFillExpLut(float *lut,int size);        // Fill exp(-x) lut

float wxSLUT(float dif,float *lut);                     // look at LUT

void fpClear(float *fpI,float fValue, int iLength);

float fiL2FloatDist ( float * u0, float *u1, int i0, int j0,int i1,int j1,int radius, int width0, int width1);

float fiL2FloatDist (float **u0, float **u1,int i0,int j0,int i1,int j1,int radius,int channels, int width0, int width1);



void fiAddNoise(float *u, float *v, float std, long int randinit, int size);





#endif