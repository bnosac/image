#include <Rcpp.h>
#include <dlib/pixel.h>
#include <dlib/array2d.h>
#include <dlib/matrix.h>
//#include <dlib/image_loader/image_loader.h>
#include <dlib/image_transforms/fhog.h>
using namespace dlib;

// [[Rcpp::export]]
Rcpp::List dlib_fhog(std::vector<int> x, int rows, int cols, 
                     const int cell_size = 8,
                     const int filter_rows_padding = 1,
                     const int filter_cols_padding = 1) {
  // Import from file
  //array2d<rgb_pixel> img;
  //load_bmp(img, file.c_str());
  array2d<rgb_pixel> img;
  img.set_size(rows, cols);
  for(int r = 0; r < rows; r++){
    for(int c = 0; c < cols; c++){
      int index = c*3 + r*cols*3;
      assign_pixel(img[r][c], rgb_pixel(x[index], x[index + 1], x[index + 2]));
    }  
  }
  // Compute HOG features
  array2d<matrix<float,31,1> > hog;
  extract_fhog_features(img, hog, cell_size, filter_rows_padding, filter_cols_padding);

  Rcpp::NumericVector fhog(hog.nr() * hog.nc() * 31);
  int i = 0;
  for(int feat = 0; feat < 31; feat++){
    for(int x_i = 0; x_i < hog.nc(); x_i++){ // x is hog_width
      for(int y_i = 0; y_i < hog.nr(); y_i++){ // y is hog_height
        fhog[i] = hog[y_i][x_i](feat, 0);
        i = i + 1;
      }
    }
  }

  return Rcpp::List::create(Rcpp::Named("hog_height") = hog.nr(),
                            Rcpp::Named("hog_width") = hog.nc(),
                            Rcpp::Named("fhog") = fhog,
                            Rcpp::Named("hog_cell_size") = cell_size,
                            Rcpp::Named("filter_rows_padding") = filter_rows_padding,
                            Rcpp::Named("filter_cols_padding") = filter_cols_padding);
}
