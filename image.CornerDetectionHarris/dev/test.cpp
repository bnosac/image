extern "C"
{
#include "iio.h"
}


void rgb_gray(
    float *rgb,  //input color image
    float *gray, //output grayscale image
    int   nx,    //number of columns
    int   ny,    //number of rows
    int   nz     //number of channels
)
{
  for(int i=0;i<nx*ny;i++)
    gray[i]=(0.2989*rgb[i*nz]+0.5870*rgb[i*nz+1]+0.1140*rgb[i*nz+2]);
}

// [[Rcpp::export]]
Rcpp::NumericVector check() {
  int nx, ny, nz;
  int x;
  int y;
  float *Ic=iio_read_image_float_vec("/home/jwijffels/image/test.png", &nx, &ny, &nz);
  float *I=new float[nx*ny];
  std::cout << "nx " << nx << " ny " << ny << "\n"; 
  //convert image to grayscale
  if(nz>1)
    rgb_gray(Ic, I, nx, ny, nz);
  else
    for(int i=0;i<nx*ny;i++)
      I[i]=Ic[i];
  
  x = 0;
  y = 0;
  std::cout << "greyscale: x=0, y=0: " << I[(x + y*ny)] << "\n";  
  x = 1;
  y = 0;
  std::cout << "greyscale: x=1, y=0: " << I[(x + y*ny)] << "\n";  
  x = 0;
  y = 1;
  std::cout << "greyscale: x=0, y=1: " << I[(x + y*ny)] << "\n"; 
  x = 0;
  y = ny-1;
  std::cout << "greyscale: x=0, y=ny-1: " << I[(x + y*ny)] << "\n"; 
  Rcpp::NumericMatrix out(ny, nx);
  for(int i_x = 0; i_x < nx; i_x++){
    for(int i_y = 0; i_y < ny; i_y++){
      out(i_y, i_x) = I[(i_x + i_y*ny)]; 
    }
  }
  Rcpp::NumericVector result(nx * ny);
  for(int i = 0; i < nx * ny; i++){
    result[i] = I[i]; 
  }
  return result;
}

