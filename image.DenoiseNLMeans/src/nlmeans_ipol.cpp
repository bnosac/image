
/*
 * Copyright (c) 2009-2011, A. Buades <toni.buades@uib.es>
 * All rights reserved.
 *
 *
 * Patent warning:
 *
 * This file implements algorithms possibly linked to the patents
 *
 * # A. Buades, T. Coll and J.M. Morel, Image data processing method by
 * reducing image noise, and camera integrating means for implementing
 * said method, EP Patent 1,749,278 (Feb. 7, 2007).
 *
 * This file is made available for the exclusive aim of serving as
 * scientific tool to verify the soundness and completeness of the
 * algorithm description. Compilation, execution and redistribution
 * of this file may violate patents rights in certain countries.
 * The situation being different for every country and changing
 * over time, it is your responsibility to determine which patent
 * rights restrictions apply to you before you compile, use,
 * modify, or redistribute this file. A patent lawyer is qualified
 * to make this determination.
 * If and only if they don't conflict with any patent terms, you
 * can benefit from the following license terms attached to this
 * file.
 *
 * License:
 *
 * This program is provided for scientific and educational only:
 * you can use and/or modify it for these purposes, but you are
 * not allowed to redistribute this work or derivative works in
 * source or executable form. A license must be obtained from the
 * patent right holders for any other use.
 *
 *
 */




#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#include "libdenoising.h"
#include "io_png.h"

/**
 * @mainpage Non Local Means denoising
 *
 * README.txt:
 * @verbinclude README.txt
 */


/**
 * @file   nlmeansIpol.cpp
 * @brief  Main executable file
 *
 *
 *
 * @author Antoni Buades <toni.buades@uib.es>
 */






// usage: nlmeans_ipol image sigma noisy denoised

int main(int argc, char **argv) {


    if (argc < 5) {
        printf("usage: nlmeans_ipol image sigma noisy denoised \n");
        exit(-1);
    }


    // read input
    size_t nx,ny,nc;
    float *d_v = NULL;
    d_v = io_png_read_f32(argv[1], &nx, &ny, &nc);
    if (!d_v) {
        printf("error :: %s not found  or not a correct png image \n", argv[1]);
        exit(-1);
    }




    // variables
    int d_w = (int) nx;
    int d_h = (int) ny;
    int d_c = (int) nc;
    if (d_c == 2) {
        d_c = 1;    // we do not use the alpha channel
    }
    if (d_c > 3) {
        d_c = 3;    // we do not use the alpha channel
    }

    int d_wh = d_w * d_h;
    int d_whc = d_c * d_w * d_h;



    // test if image is really a color image even if it has more than one channel
    if (d_c > 1) {

        // dc equals 3
        int i=0;
        while (i < d_wh && d_v[i] == d_v[d_wh + i] && d_v[i] == d_v[2 * d_wh + i ])  {
            i++;
        }

        if (i == d_wh) d_c = 1;

    }



    // add noise
    float fSigma = atof(argv[2]);
    float *noisy = new float[d_whc];

    for (int i=0; i < d_c; i++) {
        fiAddNoise(&d_v[i * d_wh], &noisy[i * d_wh], fSigma, i, d_wh);
    }





    // denoise
    float **fpI = new float*[d_c];
    float **fpO = new float*[d_c];
    float *denoised = new float[d_whc];

    for (int ii=0; ii < d_c; ii++) {

        fpI[ii] = &noisy[ii * d_wh];
        fpO[ii] = &denoised[ii * d_wh];

    }




    int bloc, win;
    float fFiltPar;

    if (d_c == 1) {

        if (fSigma > 0.0f && fSigma <= 15.0f) {
            win = 1;
            bloc = 10;
            fFiltPar = 0.4f;

        } else if ( fSigma > 15.0f && fSigma <= 30.0f) {
            win = 2;
            bloc = 10;
            fFiltPar = 0.4f;

        } else if ( fSigma > 30.0f && fSigma <= 45.0f) {
            win = 3;
            bloc = 17;
            fFiltPar = 0.35f;

        } else if ( fSigma > 45.0f && fSigma <= 75.0f) {
            win = 4;
            bloc = 17;
            fFiltPar = 0.35f;

        } else if (fSigma <= 100.0f) {

            win = 5;
            bloc = 17;
            fFiltPar = 0.30f;

        } else {
            printf("error :: algorithm parametrized only for values of sigma less than 100.0\n");
            exit(-1);
        }





    } else {


        if (fSigma > 0.0f && fSigma <= 25.0f) {
            win = 1;
            bloc = 10;
            fFiltPar = 0.55f;

        } else if (fSigma > 25.0f && fSigma <= 55.0f) {
            win = 2;
            bloc = 17;
            fFiltPar = 0.4f;

        } else if (fSigma <= 100.0f) {
            win = 3;
            bloc = 17;
            fFiltPar = 0.35f;

        } else {
            printf("error :: algorithm parametrized only for values of sigma less than 100.0\n");
            exit(-1);
        }



    }



    nlmeans_ipol(win, bloc, fSigma, fFiltPar, fpI,  fpO, d_c, d_w, d_h);


    // save noisy and denoised images
    if (io_png_write_f32(argv[3], noisy, (size_t) d_w, (size_t) d_h, (size_t) d_c) != 0) {
        printf("... failed to save png image %s", argv[3]);
    }

    if (io_png_write_f32(argv[4], denoised, (size_t) d_w, (size_t) d_h, (size_t) d_c) != 0) {
        printf("... failed to save png image %s", argv[4]);
    }




}



