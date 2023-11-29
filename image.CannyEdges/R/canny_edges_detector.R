#' @title Canny Edge Detector for Images
#' @description Canny Edge Detector for Images. See \url{https://en.wikipedia.org/wiki/Canny_edge_detector}.
#' Adapted from \url{https://github.com/Neseb/canny}.
#' @param x a matrix of image pixel values in the 0-255 range.
#' @param s sigma, the Gaussian filter variance. Defaults to 2.
#' @param low_thr lower threshold value of the algorithm. Defaults to 3.
#' @param high_thr upper threshold value of the algorithm. Defaults to 10
#' @param accGrad logical indicating to trigger higher-order gradient
#' @return a list with element edges which is a matrix with values 0 or 255 indicating
#' in the same dimension of \code{x}. Next to that
#' the list also contains the input parameters s, low_thr, high_thr and accGrad,
#' the number of rows (nx) and columns of the image (ny) and the number of pixels which
#' have value 255 (pixels_nonzero).
#' @export
#' @examples
#' if(requireNamespace("pixmap")){
#'
#' library(pixmap)
#' imagelocation <- system.file("extdata", "chairs.pgm", package="image.CannyEdges")
#' image <- read.pnm(file = imagelocation, cellres = 1)
#' x <- image@grey * 255
#'
#' edges <- image_canny_edge_detector(x)
#' edges
#' plot(edges)
#'
#' }
#'
#' \donttest{
#' if(requireNamespace("magick")){
#' ##
#' ## image_canny_edge_detector expects a matrix as input
#' ##  if you have a jpg/png/... convert it to pgm first or take the r/g/b channel
#' library(magick)
#' x <- image_read(system.file("extdata", "atomium.jpg", package="image.CannyEdges"))
#' x
#' image <- image_data(x, channels = "Gray")
#' image <- as.integer(image, transpose = TRUE)
#' edges <- image_canny_edge_detector(image)
#' edges
#' plot(edges)
#' }
#'
#' if(requireNamespace("pixmap") && requireNamespace("magick")){
#' ##
#' ## image_canny_edge_detector expects a matrix as input
#' ##  if you have a jpg/png/... convert it to pgm first or take the r/g/b channel
#' library(magick)
#' library(pixmap)
#' f <- tempfile(fileext = ".pgm")
#' x <- image_read(system.file("extdata", "atomium.jpg", package="image.CannyEdges"))
#' x <- image_convert(x, format = "pgm", depth = 8)
#' image_write(x, path = f, format = "pgm")
#'
#' image <- read.pnm(f, cellres = 1)
#' edges <- image_canny_edge_detector(image@grey * 255)
#' edges
#' plot(edges)
#'
#' file.remove(f)
#' }
#' }
image_canny_edge_detector <- function(x, s = 2, low_thr = 3, high_thr = 10, accGrad = TRUE) {
  x <- canny_edge_detector(as.integer(x), nrow(x), ncol(x), s, low_thr, high_thr, accGrad)
  class(x) <- "image_canny"
  x
}


#' @export
print.image_canny <- function(x, ...){
  cat("Canny edge detector", sep = "\n")
  cat(sprintf("  %s x %s matrix", x$nx, x$ny), sep = "\n")
  cat(sprintf("  number of pixels on edge %s", x$pixels_nonzero), sep = "\n")
  cat(sprintf("  sigma %s", x$s), sep = "\n")
  cat(sprintf("  low_thr %s", x$low_thr), sep = "\n")
  cat(sprintf("  high_thr %s", x$high_thr), sep = "\n")
  cat(sprintf("  accGrad %s", x$accGrad), sep = "\n")
}



#' @title Plot the result of the Canny Edge Detector
#' @description Plot the result of \code{\link{image_canny_edge_detector}}
#' @param x an object of class image_canny as returned by \code{\link{image_canny_edge_detector}}
#' @param ... further arguments passed on to plot, except type, xlab and ylab which are set inside the function
#' @method plot image_canny
#' @return invisible()
#' @export
#' @examples
#' library(pixmap)
#' imagelocation <- system.file("extdata", "chairs.pgm", package="image.CannyEdges")
#' image <- read.pnm(file = imagelocation, cellres = 1)
#' edges <- image_canny_edge_detector(image@grey * 255)
#' plot(edges)
plot.image_canny <- function(x, ...){
  ok <- grDevices::as.raster(x$edges, max = 255)
  plot(c(1, x$nx), c(1, x$ny), type = "n", xlab = "", ylab = "", ...)
  rasterImage(ok, 0, 0, x$nx, x$ny)
  invisible()
}
