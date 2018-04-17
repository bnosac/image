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


#include "libauxiliar.h"
#include "mt19937ar.h"

void fpClear(float *fpI,float fValue, int iLength) {
    for (int ii=0; ii < iLength; ii++) fpI[ii] = fValue;
}






// LUT tables
void  wxFillExpLut(float *lut, int size) {
    for (int i=0; i< size; i++)   lut[i]=   expf( - (float) i / LUTPRECISION);
}




float wxSLUT(float dif, float *lut) {

    if (dif >= (float) LUTMAXM1) return 0.0;

    int  x= (int) floor( (double) dif * (float) LUTPRECISION);

    float y1=lut[x];
    float y2=lut[x+1];

    return y1 + (y2-y1)*(dif*LUTPRECISION -  x);
}






float fiL2FloatDist(float *u0,float *u1,int i0,int j0,int i1,int j1,int radius,int width0, int width1) {

    float dist=0.0;
    for (int s=-radius; s<= radius; s++) {

        int l = (j0+s)*width0 + (i0-radius);
        float *ptr0 = &u0[l];

        l = (j1+s)*width1 + (i1-radius);
        float *ptr1 = &u1[l];

        for (int r=-radius; r<=radius; r++,ptr0++,ptr1++) {
            float dif = (*ptr0 - *ptr1);
            dist += (dif*dif);
        }

    }

    return dist;
}




float fiL2FloatDist(float **u0,float **u1,int i0,int j0,int i1,int j1,int radius,int channels, int width0, int width1) {

    float dif = 0.0f;


    for (int ii=0; ii < channels; ii++) {

        dif += fiL2FloatDist(u0[ii],u1[ii],i0,j0,i1,j1,radius,width0, width1);

    }


    return dif;
}




void fiAddNoise(float *u, float *v, float std, long int randinit, int size) {

    //srand48( (long int) time (NULL) + (long int) getpid()  + (long int) randinit);
    mt_init_genrand((unsigned long int) time (NULL) + (unsigned long int) getpid()  + (unsigned long int) randinit);

    for (int i=0; i< size; i++) {

        //double a = drand48();
        //double b = drand48();
        double a=mt_genrand_res53();
        double b=mt_genrand_res53();
        double z = (double)(std)*sqrt(-2.0*log(a))*cos(2.0*M_PI*b);

        v[i] =  u[i] + (float) z;

    }

}



