// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2018, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// All rights reserved.



#ifndef _ZOOM_H
#define _ZOOM_H


/**
  *
  * Zoom out an image by a factor of 2
  *
**/
float *zoom_out(
  float *I, //input image
  int nx,   //image width
  int ny    //image height
);


#endif
