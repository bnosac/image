#' @title Find SURF points in an image
#' @description SURF (speeded up robust features) points are blobs in a digital image
#' \url{https://en.wikipedia.org/wiki/Speeded_up_robust_features}. The function identifies
#' these points in the image and also provides a numeric descriptor of each point.
#'
#' This SURF descriptor is used on computer vision applications like object matching (by comparing the SURF
#' descriptor of several images using e.g. kNN - from the rflann package) or object recognition.
#' The SURF feature descriptor is based on the sum of the Haar wavelet response around the point of interest.
#' More information on the calculation, see the reference paper.
#' @param x a 3-dimensional array of integer pixel values where the first dimension is RGB, 2nd the width of the image and the 3rd the height of the image. See the example.
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
#' library(magick)
#' f <- system.file("extdata", "cruise_boat.png", package = "image.dlib")
#' x <- image_read(f)
#' x
#' surf_blobs <- image_data(x, channels = "rgb")
#' surf_blobs <- as.integer(surf_blobs, transpose = FALSE)
#' surf_blobs <- image_surf(surf_blobs, max_points = 10000, detection_threshold = 50)
#' str(surf_blobs)
#' 
#' library(magick)
#' plt <- image_draw(x)
#' points(surf_blobs$x, surf_blobs$y, col = "red", pch = 20)
#' dev.off()
#' plt
#' \dontrun{
#' ## Plot the points
#' "as.cimg.magick-image" <- function(x){
#'   out <- lapply(x, FUN=function(frame){
#'     frame <- as.integer(frame[[1]])[, , 1:3] # dropping the alpha channel
#'     dim(frame) <- append(dim(frame), 1, after = 2)
#'     frame
#'   })
#'   out$along <- 3
#'   out <- do.call(abind::abind, out)
#'   out <- imager::as.cimg(out)
#'   out <- imager::permute_axes(out, "yxzc")
#'   out
#' }
#' library(imager)
#' library(magick)
#' library(abind)
#' img <- image_read(path = f)
#' plot(as.cimg(img), main = "SURF points")
#' points(surf_blobs$x, surf_blobs$y, col = "red", pch = 20)
#' }
#' 
#' library(magick)
#' img1   <- image_scale(logo, "x300")
#' img2   <- image_flip(img1)
#' height <- image_info(img1)$height
#' sp1 <- image_surf(image_data(img1, channels = "rgb"), max_points = 50)
#' sp2 <- image_surf(image_data(img2, channels = "rgb"), max_points = 50)
#' 
#' ## Match surf points and plot matches
#' library(FNN)
#' k <- get.knnx(sp1$surf, sp2$surf, k = 1)
#' combined <- image_append(c(img1, img2), stack = TRUE)
#' plt <- image_draw(combined)
#' points(sp1$x, sp1$y, col = "red")
#' points(sp2$x, sp2$y + height, col = "blue")
#' for(i_from in head(order(k$nn.dist), 50)){
#'   i_to   <- k$nn.index[i_from]
#'   lines(x = c(sp1$x[i_to], sp2$x[i_from]), y = c(sp1$y[i_to], sp2$y[i_from] + height), col = "red")
#' }
#' dev.off()
#' plt
image_surf <- function(x, max_points = 1000, detection_threshold = 30) {
  width  <- dim(x)[2]
  height <- dim(x)[3]

  out <- dlib_surf_points(x, rows = height, cols = width, max_points=max_points, detection_threshold=detection_threshold)
  out$surf[is.nan(out$surf)] <- 0
  out
}

