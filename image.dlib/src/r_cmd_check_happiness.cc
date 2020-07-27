#include <stdlib.h>
#include <string.h>
#include <Rcpp.h>
#include <ostream>

void exit_dlib(int status_code) {
  if (status_code != EXIT_SUCCESS) {
    Rcpp::stop("Failure in dlib. Exit code: " + std::to_string(status_code));
  }
}
void abort_dlib() {
  Rcpp::stop("Unexpected dlib abort");
}

namespace std {
  std::ostream Rcout(Rcpp::Rcout.rdbuf());
  std::ostream Rcerr(Rcpp::Rcerr.rdbuf());
}
