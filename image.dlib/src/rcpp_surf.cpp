#include <Rcpp.h>
#include <dlib/pixel.h>
#include <dlib/array2d.h>
//#include <dlib/image_loader/image_loader.h>
#include <dlib/image_keypoint.h>
using namespace std;
using namespace dlib;

// [[Rcpp::export]]
Rcpp::List dlib_surf_points(std::vector<int> x, int rows, int cols, 
                            long max_points = 10000, double detection_threshold = 30.0) {
  //array2d<rgb_pixel> img;
  //load_bmp(img, file_name.c_str());
  array2d<rgb_pixel> img;
  img.set_size(rows, cols);
  for(int r = 0; r < rows; r++){
    for(int c = 0; c < cols; c++){
      int index = c*3 + r*cols*3;
      assign_pixel(img[r][c], rgb_pixel(x[index], x[index + 1], x[index + 2]));
    }  
  }
  std::vector<surf_point> sp = get_surf_points(img, max_points, detection_threshold);
  Rcpp::NumericVector ip_center_x(sp.size());
  Rcpp::NumericVector ip_center_y(sp.size());
  Rcpp::NumericVector ip_angle(sp.size());
  Rcpp::NumericVector ip_scale(sp.size());
  Rcpp::NumericVector ip_score(sp.size());
  Rcpp::NumericVector ip_laplacian(sp.size());
  Rcpp::NumericMatrix ip_surf(sp.size(), 64);
  
  int points_nr;
  points_nr = sp.size();
  for(int i=0; i<points_nr; i++){
    ip_center_x[i] = sp[i].p.center(0);
    ip_center_y[i] = sp[i].p.center(1);
    ip_angle[i] = sp[i].angle;
    ip_scale[i] = sp[i].p.scale;
    ip_score[i] = sp[i].p.score;
    ip_laplacian[i] = sp[i].p.laplacian;
    for(int j=0; j<64; j++){
      ip_surf(i, j) = sp[i].des(j, 0);
    }

  }
  return Rcpp::List::create(Rcpp::Named("points") = sp.size(),
                            Rcpp::Named("x") = ip_center_x,
                            Rcpp::Named("y") = ip_center_y,
                            Rcpp::Named("angle") = ip_angle,
                            Rcpp::Named("pyramid_scale") = ip_scale,
                            Rcpp::Named("score") = ip_score,
                            Rcpp::Named("laplacian") = ip_laplacian,
                            Rcpp::Named("surf") = ip_surf);
}

