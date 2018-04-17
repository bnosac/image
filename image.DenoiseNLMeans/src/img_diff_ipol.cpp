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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>


#include "io_png.h"






int main(int argc, char **argv) {


    if (argc < 5) {
        printf("usage: img_diff_ipol image1 image2 sigma difference\n");
        return EXIT_FAILURE;
    }




    // read input1
    size_t nx,ny,nc;
    float *d_v = NULL;
    d_v = io_png_read_f32(argv[1], &nx, &ny, &nc);
    if (!d_v) {
        printf("error :: %s not found  or not a correct png image \n", argv[1]);
        return EXIT_FAILURE;
    }

    if (nc == 2) {
        nc = 1;    // we do not use the alpha channel
    }
    if (nc > 3) {
        nc = 3;    // we do not use the alpha channel
    }

    // test if image is really a color image even if it has more than one channel
    if (nc > 1) {
        int nxy = nx * ny;
        // nc equals 3
        int i=0;
        while (i < nxy && d_v[i] == d_v[nxy + i] && d_v[i] == d_v[2 * nxy + i ])  {
            i++;
        }
        if (i == nxy) nc = 1;
    }



    // read input2
    size_t nx2,ny2,nc2;
    float *d_v2 = NULL;
    d_v2 = io_png_read_f32(argv[2], &nx2, &ny2, &nc2);
    if (!d_v2) {
        printf("error :: %s not found  or not a correct png image \n", argv[2]);
        return EXIT_FAILURE;
    }


    if (nc2 == 2) {
        nc2 = 1;    // we do not use the alpha channel
    }
    if (nc2 > 3) {
        nc2 = 3;    // we do not use the alpha channel
    }

    // test if image is really a color image even if it has more than one channel
    if (nc2 > 1) {
        int nxy2 = nx2 * ny2;
        // nc2 equals 3
        int i=0;
        while (i < nxy2 && d_v[i] == d_v[nxy2 + i] && d_v[i] == d_v[2 * nxy2 + i ])  {
            i++;
        }
        if (i == nxy2) nc2 = 1;
    }


    // test if same size
    if (nc != nc2 || nx != nx2 || ny != ny2) {
        printf("error :: input images of different size or number of channels \n");
        return EXIT_FAILURE;
    }




    // variables
    int d_w = (int) nx;
    int d_h = (int) ny;
    int d_c = (int) nc;

    int d_whc = d_c * d_w * d_h;



    // comput difference and convert from [-4 sigma, 4 sigma] to [0,255]
    float fSigma = atof(argv[3]);
    fSigma *= 4.0f;

    float *difference = new float[d_whc];

    for (int ii=0; ii < d_whc ;  ii++) {

        float fValue = d_v[ ii] - d_v2[ii];

        fValue =  (fValue + fSigma) * 255.0f / (2.0f * fSigma);

        if (fValue < 0.0) fValue = 0.0f;
        if (fValue > 255.0) fValue = 255.0f;

        difference[ii] = fValue;

    }


    // save noisy and denoised images
    if (io_png_write_f32(argv[4], difference, (size_t) d_w, (size_t) d_h, (size_t) d_c) != 0) {
        printf("... failed to save png image %s", argv[4]);
    }

    return EXIT_SUCCESS;
}
