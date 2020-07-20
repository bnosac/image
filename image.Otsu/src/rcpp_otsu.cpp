/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Jan Wijffels, BNOSAC - jwijffels@bnosac.be
 * Copyright (c) 2015 Juan Pablo Balarini, Sergio Nesmachnow
 * jpbalarini@fing.edu.uy, sergion@fing.edu.uy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Rcpp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>


class GreyBox {
public:
  int width;
  int height;
  float *data;
  int get_width(void) {
    return width;
  }
  int get_height(void) {
    return height;
  }
  float *get_channel(int channel) {
    return data;
  }
  GreyBox(int w, int h, float *cells){
    width = w;
    height = h;
    data = cells;
  }
};


const int MAX_INTENSITY = 255;

/*! Computes image histogram
 \param input Input image
\param hist Image histogram that is returned
*/
void computeHistogram(GreyBox &input, unsigned *hist){
  // Compute number of pixels
  long int N = input.get_width() * input.get_height();
  int i = 0;
  
  // Initialize array
  for(i = 0; i <= MAX_INTENSITY; i++) hist[i] = 0;
  
  // Use first channel
  float *in = input.get_channel(0);
  // Iterate image
  for (i = 0; i < N; i++) {
    int value = (int) in[i];
    hist[value]++;
  }
  
  //printf("Total # of pixels: %ld\n", N);
}

/*! Segments the image using the computed threshold
\param input Input image
\param output Output segmented image
\param threshold Threshold used for segmentation
*/
void segmentImage(GreyBox &input, GreyBox *output, int threshold) {
  // Compute number of pixels
  long int N = input.get_width() * input.get_height();
  // Modify output image
  // Use first channel
  float *in = input.get_channel(0);
  float *out = output->get_channel(0);
  // Iterate image
  for (int i = 0; i < N; i++) {
    int value = (int) in[i];
    // Build the segmented image
    if (value > threshold){
      out[i] = 255.0f;
    }else{
      out[i] = 0.0f;
    }
  }
}

/*! Computes Otsus segmentation
\param input Input image
\param hist Image histogram
\param output Output segmented image
\param overrided_threshold Input param that overrides threshold
*/
int computeOtsusSegmentation(GreyBox &input, unsigned* hist, GreyBox *output, int overrided_threshold){
  // Compute number of pixels
  long int N = input.get_width() * input.get_height();
  int threshold = 0;
  
  if (overrided_threshold != 0){
    // If threshold was manually entered
    threshold = overrided_threshold;
  }else{
    // Compute threshold
    // Init variables
    float sum = 0;
    float sumB = 0;
    int q1 = 0;
    int q2 = 0;
    float varMax = 0;
    
    // Auxiliary value for computing m2
    for (int i = 0; i <= MAX_INTENSITY; i++){
      sum += i * ((int)hist[i]);
    }
    
    for (int i = 0 ; i <= MAX_INTENSITY ; i++) {
      // Update q1
      q1 += hist[i];
      if (q1 == 0)
        continue;
      // Update q2
      q2 = N - q1;
      
      if (q2 == 0)
        break;
      // Update m1 and m2
      sumB += (float) (i * ((int)hist[i]));
      float m1 = sumB / q1;
      float m2 = (sum - sumB) / q2;
      
      // Update the between class variance
      float varBetween = (float) q1 * (float) q2 * (m1 - m2) * (m1 - m2);
      
      // Update the threshold if necessary
      if (varBetween > varMax) {
        varMax = varBetween;
        threshold = i;
      }
    }
  }
  
  // Perform the segmentation
  segmentImage(input, output, threshold);
  return threshold;
}



// [[Rcpp::export]]
Rcpp::List otsu(Rcpp::NumericVector x, int width, int height, int threshold = 0) {
  float *input_data  = new float[width*height];
  float *output_data = new float[width*height];
  for(int i = 0; i < x.size(); i++){
    input_data[i] = (float)x[i];
  }
  GreyBox input(width, height, input_data);
  GreyBox *output = new GreyBox(width, height, output_data);
  unsigned hist[MAX_INTENSITY + 1];
  computeHistogram(input, hist);
  
  int thresh = computeOtsusSegmentation(input, hist, output, threshold);
  for(int i = 0; i < x.size(); i++){
    x[i] = (double)output->get_channel(0)[i];
  }
  Rcpp::List out = Rcpp::List::create(Rcpp::Named("x") = x, 
                                      Rcpp::Named("threshold") = thresh);
  return out;
}




