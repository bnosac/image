#include <Rcpp.h>
using namespace Rcpp;
// [[Rcpp::plugins(cpp11)]]
// [[Rcpp::depends(dlib)]]
#include <dlib/pixel.h>
#include <dlib/array2d.h>
#include <dlib/matrix.h>
#include <dlib/image_loader/image_loader.h>
#include <dlib/image_transforms/fhog.h>
using namespace std;
using namespace dlib;

// [[Rcpp::export]]
List dlib_fhog(const std::string file,
               const int cell_size = 8,
               const int filter_rows_padding = 1,
               const int filter_cols_padding = 1) {
  // Import from file
  array2d<rgb_pixel> img;
  load_bmp(img, file.c_str());
  // Compute HOG features
  array2d<matrix<float,31,1> > hog;
  extract_fhog_features(img, hog, cell_size, filter_rows_padding, filter_cols_padding);

  NumericVector fhog(hog.nr() * hog.nc() * 31);
  int i = 0;
  for(int feat = 0; feat < 31; feat++){
    for(int x_i = 0; x_i < hog.nc(); x_i++){ // x is hog_width
      for(int y_i = 0; y_i < hog.nr(); y_i++){ // y is hog_height
        fhog[i] = hog[y_i][x_i](feat, 0);
        i = i + 1;
      }
    }
  }
  //cout << hog[5][4] << endl;

  return List::create(_["hog_height"] = hog.nr(),
                      _["hog_width"] = hog.nc(),
                      _["fhog"] = fhog,
                      _["hog_cell_size"] = cell_size,
                      _["filter_rows_padding"] = filter_rows_padding,
                      _["filter_cols_padding"] = filter_cols_padding);
}
