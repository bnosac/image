// Content of this file is added to each source to avoid R CMD check messages
#pragma once

#include <stdlib.h>
#include <string.h>
#include <Rcpp.h>
#include <stdexcept>

#define cerr Rcerr
#define cout Rcout
#define exit(status_code) exit_dlib(status_code)
#define abort() abort_dlib()

void exit_dlib(int error_code);
void abort_dlib();

namespace std {
  extern std::ostream Rcout;
  extern std::ostream Rcerr;
}
