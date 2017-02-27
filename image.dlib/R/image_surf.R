#' @title Find SURF points in an image
#' @description SURF (speeded up robust features) points are blobs in a digital image
#' \url{https://en.wikipedia.org/wiki/Speeded_up_robust_features}. The function identifies
#' these points in the image and also provides a numeric descriptor of each point.
#'
#' This SURF descriptor is used on computer vision applications like object matching (by comparing the SURF
#' descriptor of several images using e.g. kNN - from the rflann package) or object recognition.
#' The SURF feature descriptor is based on the sum of the Haar wavelet response around the point of interest.
#' More information on the calculation, see the reference paper.
#' @param file a bmp file in bmp3 format with extension .bmp
#' @param max_points maximum number of surf points to find
#' @param detection_threshold detection threshold (the higher, the lower the number of SURF points found)
#' @references SURF: Speeded Up Robust Features By Herbert Bay, Tinne Tuytelaars, and Luc Van Gool
#' @return a list with SURF points found and the SURF descriptor. It contains:
#' \itemize{
#'  \item{points: }{The number of SURF points}
#'  \item{x: }{The x location of each SURF point}
#'  \item{y: }{The y location of each SURF point}
#'  \item{angle: }{The angle of each SURF point}
#'  \item{pyramid_scale: }{The pyramid scale of each SURF point}
#'  \item{score: }{The score of each SURF point}
#'  \item{laplacian: }{The laplacian at each SURF point}
#'  \item{surf: }{The SURF descriptor which is a matrix with 64 columns describing the point}
#' }
#' @export
#' @examples
#' f <- system.file("extdata", "cruise_boat.bmp", package="image.dlib")
#' surf_blobs <- image_surf(f, max_points = 10000, detection_threshold = 50)
#' str(surf_blobs)
#'
#' ## Plot the points
#' library(imager)
#' library(magick)
#' img <- image_read(path = f)
#' plot(as.cimg(img), main = "SURF points")
#' points(surf_blobs$x, surf_blobs$y, col = "red", pch = 20)
image_surf <- function(file, max_points = 1000, detection_threshold = 30) {
  stopifnot(tools::file_ext(file) == "bmp")
  dlib_surf_points(file, max_points=max_points, detection_threshold=detection_threshold)
}


cvt.frame <- function(f){
  f <- as.integer(f)
  d <- dim(f)
  dim(f) <- c(d[1:2],1,4)
  out <- imager::cimg(f)
  out <- imager::imrotate(out, 90)
  out <- imager::mirror(out, "x")
  out
}

#' @title Convert magick-image to cimg
#' @description Convert magick-image to cimg. Put in this package as long as it is not in the imager package
#' @param im an object of class magick-image from the magick package
#' @export
"as.cimg.magick-image" <- function(im){
  cc <- NULL
  out <- imager::map_il(seq_len(length(im)), ~ cvt.frame(im[[.]]))
  out <- imager::imappend(out, "z")
  alpha <- imager::imsub(out,cc==4)
  out <- imager::imsub(out,cc<=3)
  attr(out,"alpha") <- alpha
  out
}

