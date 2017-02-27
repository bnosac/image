#' @title Find Corners in Digital Images with FAST-9.
#' @description An implementation of the "FAST-9" corner detection algorithm explained at <http://www.edwardrosten.com/work/fast.html>. 
#' @param x a matrix of image pixel values in the 0-255 range.
#' @param threshold positive integer where threshold is the threshold below which differences in luminosity 
#' between adjacent pixels are ignored. Think of it as a smoothing parameter.
#' @param suppress_non_max logical
#' @return as list of the found corners with the x/y locations
#' @export
#' @examples
#' library(pixmap)
#' imagelocation <- system.file("extdata", "chairs.pgm", package="image.CornerDetectionF9")
#' image <- read.pnm(file = imagelocation, cellres = 1)
#' x <- image@grey * 255
#' corners <- image_detect_corners(x, 80)
#' plot(image)
#' points(corners$x, corners$y, col = "red", pch = 20, lwd = 0.5)
#' 
#' ##
#' ## image_detect_corners expects a matrix as input
#' ##  if you have a jpg/png/... convert it to pgm first or take the r/g/b channel
#' f <- tempfile(fileext = ".pgm")
#' library(magick)
#' x <- image_read(system.file("extdata", "hall.jpg", package="image.CornerDetectionF9"))
#' x <- image_convert(x, format = "pgm", depth = 8)
#' image_write(x, path = f, format = "pgm")
#'
#' image <- read.pnm(f, cellres = 1)
#' corners <- image_detect_corners(image@grey * 255, 80)
#' plot(image)
#' points(corners$x, corners$y, col = "red", pch = 20, lwd = 0.5)
image_detect_corners <- function(x, threshold = 50L, suppress_non_max = FALSE) {
  stopifnot(is.matrix(x))
  # bytes_per_row is the size of an image row, in bytes. For an 8-bit grayscale image it's usually equal to the image width, or, if the format has some alignment constraints, might be the closest next value that is a multiple of 4, 8 or whatever the format says rows should align with.
  corners <- detect_corners(as.integer(x),
                                       width=nrow(x),
                                       height=ncol(x),
                            bytes_per_row = nrow(x),
                            suppress_non_max = as.logical(suppress_non_max),
                            threshold = as.integer(threshold))
  names(corners) <- c("x", "y")
  class(corners) <- "image.corners"
  corners
}

#' @export
print.image.corners <- function(x, ...){
  cat("Corner Detector", sep = "\n")
  cat(sprintf("  found %s corners", length(x$x)), sep = "\n")
}

