#include <Rcpp.h>
using namespace Rcpp;
// [[Rcpp::plugins(cpp11)]]
// [[Rcpp::depends(dlib)]]
#include <dlib/pixel.h>
#include <dlib/array2d.h>
#include <dlib/image_loader/image_loader.h>
#include <dlib/image_keypoint.h>
using namespace std;
using namespace dlib;

// [[Rcpp::export]]
List dlib_surf_points(const std::string file_name,
                      long max_points = 10000,
                      double detection_threshold = 30.0) {
  array2d<rgb_pixel> img;
  load_bmp(img, file_name.c_str());
  std::vector<surf_point> sp = get_surf_points(img, max_points, detection_threshold);
  NumericVector ip_center_x(sp.size());
  NumericVector ip_center_y(sp.size());
  NumericVector ip_angle(sp.size());
  NumericVector ip_scale(sp.size());
  NumericVector ip_score(sp.size());
  NumericVector ip_laplacian(sp.size());
  NumericMatrix ip_surf(sp.size(), 64);

  for(int i=0; i<sp.size(); i++){
    ip_center_x[i] = sp[i].p.center(0);
    ip_center_y[i] = sp[i].p.center(1);
    ip_angle[i] = sp[i].angle;
    ip_scale[i] = sp[i].p.scale;
    ip_score[i] = sp[i].p.score;
    ip_laplacian[i] = sp[i].p.laplacian;
    for(int j=0; j<64; j++){
      ip_surf[i, j] = sp[i].des(j, 1);
    }

  }
  return List::create(_["points"] = sp.size(),
                      _["x"] = ip_center_x,
                      _["y"] = ip_center_y,
                      _["angle"] = ip_angle,
                      _["pyramid_scale"] = ip_scale,
                      _["score"] = ip_score,
                      _["laplacian"] = ip_laplacian,
                      _["surf"] = ip_surf);
}

