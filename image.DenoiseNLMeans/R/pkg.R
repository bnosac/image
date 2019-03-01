#' @importFrom Rcpp evalCpp
#' @importFrom magick image_info image_data image_read image_comment
#' @importFrom grDevices as.raster
#' @useDynLib image.DenoiseNLMeans
NULL


#' @title Image Denoising using Non-Local Means
#' @description Image Denoising using Non-Local Means
#' @param image an object of class \code{magick-image} in 8-bit sRGB colorspace
#' @param sigma sigma noise parameter (type numeric)
#' @param filter filtering parameter (type numeric)
#' @param win Half size of comparison window (type integer)
#' @param bloc Half size of research window (type integer)
#' @export
#' @return an object of class \code{magick-image} with the denoised image, 
#' in the comments the algorithm arguments sigma, filter, window and bloc are provided.
#' @references For details on the algorithm, check the paper http://www.ipol.im/pub/art/2011/bcm_nlm.
#' @note Function is provided for scientific and education only. 
#' May be linked to the EP patent 1,749,278 by A. Buades, T. Coll and J.M. Morel.
#' @examples 
#' library(magick)
#' f <- system.file(package = "image.DenoiseNLMeans", "extdata", "img_garden.png")
#' img <- image_read(f)
#' img
#' denoised <- image_denoise_nlmeans(img)
#' denoised
#' x <- image_noise(img, noisetype = "Poisson")
#' x
#' denoised <- image_denoise_nlmeans(x, sigma = 40)
#' denoised
#' denoised <- image_denoise_nlmeans(x, sigma = 10, win = 2, bloc = 10, filter = 0.4)
#' denoised
#' image_comment(denoised)
image_denoise_nlmeans <- function(image, sigma = 10, win = 1, bloc = 10, filter = 0.4){
  stopifnot(inherits(image, "magick-image"))
  if(!missing(win) & !missing(bloc) & !missing(filter)){
    args_auto <- FALSE
  }else{
    args_auto <- TRUE
  }
  
  magick::image_apply(image, FUN=function(x){
    i <- magick::image_info(x)
    width <- i$width
    height <- i$height
    channels <- 3L
    d <- magick::image_data(x, channels = "rgb")
    d <- as.integer(d)
    d <- aperm(d, c(2, 1, 3))
    denoised <- rcpp_nlmeans(d, width = width, height = height,  channels = channels, 
                             as.numeric(sigma), args_auto = args_auto, 
                             win = as.integer(win), bloc = as.integer(bloc), fFiltPar = as.numeric(filter))
    
    ok <- denoised$denoised
    ok[ok > 255] <- 255
    ok <- array(ok, dim = c(width, height, channels))
    ok <- aperm(ok, perm = c(2, 1, 3))
    ok <- grDevices::as.raster(ok, max = 255)
    ok <- magick::image_read(ok)
    magick::image_comment(ok, sprintf("sigma=%s, filter=%s, window=%s, bloc=%s", denoised$sigma, denoised$filter, denoised$window, denoised$bloc))
    ok
  })
}

