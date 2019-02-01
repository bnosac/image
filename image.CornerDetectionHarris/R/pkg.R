#' @importFrom Rcpp evalCpp
#' @useDynLib image.CornerDetectionHarris
NULL



#' @title Find Corners using Harris Corner Detection
#' @description An implementation of the Harris Corner Detection algorithm explained at \url{https://doi.org/10.5201/ipol.2018.229}. 
#' @param x an object of class magick-image or a greyscale matrix of image pixel values in the 0-255 range where values start at the top left corner.
#' @param k Harris' K parameter. Defaults to 0.06.
#' @param sigma_d Gaussian standard deviation for derivation. Defaults to 1.
#' @param sigma_i Gaussian standard deviation for integration. Defaults to 2.5.
#' @param threshold threshold for eliminating low values. Defaults to 130.
#' @param gaussian smoothing, either one of 'precise Gaussian', 'fast Gaussian' or 'no Gaussian'. Defaults to 'fast Gaussian'.
#' @param gradient calculation of gradient, either one of 'central differences' or 'Sobel operator'. Defaults to 'central differences'.
#' @param strategy strategy for selecting the output corners, either one of 'all corners', 'sort all corners', 'N corners', 'distributed N corners'. Defaults to 'all corners'.
#' @param Nselect number of output corners. Defaults to 1.
#' @param measure either one of 'Harris', 'Shi-Tomasi' or 'Harmonic Mean'. Defaults to 'Harris'.
#' @param Nscales number of scales for filtering out corners. Defaults to 1.
#' @param precision subpixel accuracy, either one of 'no subpixel', 'quadratic approximation', 'quartic interpolation'. Defaults to 'quadratic approximation'
#' @param cells regions for output corners (1x1, 2x2, ..., NxN). Defaults to 10.
#' @param verbose logical, indicating to show the trace of different substeps
#' @return as list of the relevant points with the x/y locations as well as the strenght. Note y values start at the top left corner of the image.
#' @export
#' @examples
#' library(magick)
#' path <- system.file(package="image.CornerDetectionHarris", 
#'                     "extdata", "building.png")
#' x <- image_read(path)
#' points <- image_harris(x)
#' points
#' 
#' img <- image_draw(x)
#' points(points$x, points$y, col = "red", pch = 20)
#' dev.off()
#' img <- image_draw(x)
#' points(points$x, points$y, 
#'        col = "red", pch = 20, cex = 5 * points$strength / max(points$strength))
#' dev.off()
#' 
#' ## Or pass on a greyscale matrix starting at top left
#' mat <- magick::image_data(x, channels = "gray")
#' mat <- as.integer(mat)
#' mat <- mat[, , 1]
#' mat <- t(mat)
#' points <- image_harris(mat)
#' img <- image_draw(x)
#' points(points$x, points$y, col = "red", pch = 20)
#' dev.off()
image_harris <- function(x, 
                         k = 0.060000, 
                         sigma_d = 1.000000, 
                         sigma_i = 2.500000, 
                         threshold = 130, 
                         gaussian = c('fast Gaussian', 'precise Gaussian', 'no Gaussian'), 
                         gradient = c('central differences', 'Sobel operator'), 
                         strategy = c('all corners', 'sort all corners', 'N corners', 'distributed N corners'), 
                         Nselect = 1L, 
                         measure = c('Harris', 'Shi-Tomasi', 'Harmonic Mean'), 
                         Nscales = 1L, 
                         precision = c('quadratic approximation', 'quartic interpolation', 'no subpixel'), 
                         cells = 10L, 
                         verbose = FALSE) {
  gaussian <- which(gaussian %in% match.arg(gaussian)) -1L
  gradient <- which(gradient %in% match.arg(gradient)) -1L
  strategy <- which(strategy %in% match.arg(strategy)) -1L
  measure <- which(measure %in% match.arg(measure)) -1L
  precision <- which(precision %in% match.arg(precision)) -1L
  
  if(inherits(x, "magick-image")){
    if(!requireNamespace("magick", quietly = TRUE)){
      stop("image_harris requires the magick package, which you can install from cran with install.packages('magick')")
    }
    meta <- magick::image_info(x)
    if(nrow(meta) != 1){
      stop("image_harris requires a magick-image containing 1 image only")
    }
    w <- meta$width
    h <- meta$height
    
    x <- magick::image_data(x, channels = "gray")
    x <- as.integer(x)
    x <- x[, , 1]
    x <- t(x)
  }else if(inherits(x, "matrix")){
    #w <- ncol(x)
    #h <- nrow(x)
    w <- nrow(x)
    h <- ncol(x)
  }else{
    stop("x is not a matrix nor a magick-image")
  }
  corners <- detect_corners(x, w, h, 
                            k = k, sigma_d = sigma_d, sigma_i = sigma_i, 
                            threshold = threshold, gaussian = gaussian, gradient = gradient, strategy = strategy, 
                            Nselect = Nselect, measure = measure, Nscales = Nscales, precision = precision, 
                            cells = cells, verbose = verbose)
  class(corners) <- "image.harris"
  corners
}


#' @export
print.image.harris <- function(x, ...){
  cat("Harris Corner Detector", sep = "\n")
  cat(sprintf("  found %s corners", length(x$x)), sep = "\n")
}


