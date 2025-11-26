#' @importFrom Rcpp evalCpp
#' @useDynLib image.Otsu
NULL



#' @title Image segmentation using Otsu
#' @description An implementation of the Otsu's image segmentation algorithm explained at \doi{10.5201/ipol.2016.158}. 
#' @param x an object of class magick-image or a greyscale matrix of image pixel values in the 0-255 range
#' @param threshold integer value in range of 0-255. To override the threshold. Defaults to 0 indicating not to override the threshold.
#' @return 
#' In case \code{x} is a matrix, a list with elements x (containing the thresholded image) and threshold is returned \cr
#' In case \code{x} is a \code{magick-image}, the thresholded \code{magick-image} is returned alongside
#' which also now has an attribute called threshold with the exact Otsu threshold value
#' @export
#' @examples
#' library(magick)
#' path <- system.file(package="image.Otsu", "extdata", "coins.jpeg")
#' x    <- image_read(path)
#' x
#' img  <- image_otsu(x)
#' img
#' attr(img, "threshold")
#' img  <- image_otsu(x, threshold = 180)
#' img
#'
#' img  <- image_data(x, channels = "gray")
#' img  <- as.integer(img, transpose = TRUE)
#' img  <- img[, , 1]
#' img  <- image_otsu(img)
#' str(img)
image_otsu <- function(x, threshold = 0) {
  threshold <- as.integer(threshold)
  stopifnot(threshold >= 0 & threshold <= 255)
  if(inherits(x, "magick-image")){
    if(!requireNamespace("magick", quietly = TRUE)){
      stop("image_otsu requires the magick package, which you can install from cran with install.packages('magick')")
    }
    meta <- magick::image_info(x)
    if(nrow(meta) != 1){
      stop("image_otsu requires a magick-image containing 1 image only")
    }
    w <- meta$width
    h <- meta$height
    
    img <- magick::image_data(x, channels = "gray")
    img <- as.integer(img)
    img <- img[, , 1]
    img <- otsu(img, w, h, threshold)
    img$x <- grDevices::as.raster(img$x, max = 255)
    img$x <- magick::image_read(img$x)
    attr(img$x, which = "threshold") <- img$threshold
    img <- img$x
  }else if(inherits(x, "matrix")){
    w <- ncol(x)
    h <- nrow(x)
    img <- otsu(x, w, h, threshold)
  }else{
    stop("x is not a matrix nor a magick-image")
  }
  img
}



