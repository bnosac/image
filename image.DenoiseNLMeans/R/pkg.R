#' @importFrom Rcpp evalCpp
#' @useDynLib image.DenoiseNLMeans
NULL


#' @title Image Denoising using Non-Local Means
#' @description Image Denoising using Non-Local Means
#' @param x file on disk in .png format
#' @param sigma sigma noise parameter (type numeric)
#' @param filter filtering parameter (type numeric)
#' @param win Half size of comparison window (type integer)
#' @param bloc Half size of research window (type integer)
#' @export
#' @return a list with elements file, channels, width, height with information on the input file and
#' next file_denoised, the path to the denoised file as well as the algorithm arguments sigma, filter, window and bloc.
#' For details on the algorithm, check the paper \url{http://www.ipol.im/pub/art/2011/bcm_nlm}.
#' @examples 
#' library(magick)
#' img <- system.file(package = "image.DenoiseNLMeans", "extdata", "img_garden.png")
#' x <- image_denoise_nlmeans(img, sigma = 1)
#' image_read(x$file_denoised)
#' 
#' x <- image_denoise_nlmeans(img, sigma = 10, win = 2, bloc = 10, filter = 0.4)
#' image_read(x$file_denoised)
image_denoise_nlmeans <- function(x, sigma = 10, win = 1, bloc = 10, filter = 0.4){
  stopifnot(length(x) == 1)
  stopifnot(file.exists(x))
  stopifnot(tools::file_ext(x) == "png")
  
  f <- tempfile(fileext = ".jpg")
  if(!missing(win) & !missing(bloc) & !missing(filter)){
    rcpp_nlmeans(x, f, as.numeric(sigma), args_auto=FALSE, 
                 win = as.integer(win), bloc = as.integer(bloc), fFiltPar = as.numeric(filter))
  }else{
    rcpp_nlmeans(x, f, as.numeric(sigma), args_auto=TRUE)
  }
}
