#' @importFrom Rcpp evalCpp
#' @importFrom graphics text rect
#' @useDynLib image.libfacedetection
NULL




#' @title Detect faces in images using the libfacedetection CNN
#' @description Detect faces in images using using a convolutional neural network available from \url{https://github.com/ShiqiYu/libfacedetection}. 
#' The function can be used to detect faces of minimal size 48x48 pixels.
#' @param x an object of class magick-image with rgb colors. Or an rgb integer array with pixel values in the 0-255 range.
#' @return A list with elements nr and detections.\cr
#' Where nr indicates the number of faces found and the data frame detections 
#' indicates the locations of these. This data.frame has columns x, y, width and height 
#' as well as columns neighbours and angle. The values of x and y are the top left of the start of the box.
#' @export
#' @examples
#' library(magick)
#' path <- system.file(package="image.libfacedetection", "images", "handshake.jpg")
#' x <- image_read(path)
#' x
#' faces <- image_detect_faces(x)
#' faces
#' plot(faces, x, border = "red", lwd = 7, col = "white")
#' 
#' 
#' ##
#' ## You can also directly pass on the RGB array in BGR format 
#' ## without the need of having magick
#' ##
#' tensor <- image_data(x, channels = "rgb")
#' tensor <- as.integer(tensor)
#' faces  <- image_detect_faces(tensor)
#' str(faces)
#' plot(faces, x)
image_detect_faces <- function(x) {
  if(inherits(x, "magick-image")){
    if(!requireNamespace("magick", quietly = TRUE)){
      stop("image_detect_faces requires the magick package, which you can install from cran with install.packages('magick')")
    }
    meta <- magick::image_info(x)
    if(nrow(meta) != 1){
      stop("image_detect_faces requires a magick-image containing 1 image only")
    }
    w <- meta$width
    h <- meta$height
    
    x <- magick::image_data(x, channels = "rgb")
    x <- as.integer(x)
    x <- aperm(x, c(3, 2, 1))
    faces <- detect_faces(x, width = w, height = h, step = 1*w*3)
  }else if(inherits(x, "array")){
    w <- ncol(x)
    h <- nrow(x)
    stopifnot(length(dim(x)) == 3 && dim(x)[3] == 3)
    x <- aperm(x, c(3, 2, 1))
    faces <- detect_faces(x, width = w, height = h, step = 1*w*3)
  }else{
    stop("x is not an array nor a magick-image")
  }
  class(faces) <- "libfacedetection"
  faces
}

#' @title Plot detected faces
#' @description Plot functionality for bounding boxes detected with \code{\link{image_detect_faces}}
#' @param x object of class \code{libfacedetection} as returned by \code{\link{image_detect_faces}}
#' @param image object of class \code{magick-image} which was used to construct \code{x}
#' @param border color of the border of the box. Defaults to red. Passed on to \code{\link[graphics]{rect}}
#' @param lwd line width of the border of the box. Defaults to 5. Passed on to \code{\link[graphics]{rect}}
#' @param only_box logical indicating to draw only the box and not the text on top of it. Defaults to FALSE.
#' @param col color of the text on the box. Defaults to red. Passed on to \code{\link[graphics]{text}}
#' @param cex character expension factor of the text on the box. Defaults to 2. Passed on to \code{\link[graphics]{text}}
#' @param ... other parameters passed on to \code{\link[graphics]{rect}}
#' @export
#' @return an object of class \code{magick-image}
#' @examples
#' library(magick)
#' path <- system.file(package="image.libfacedetection", "images", "handshake.jpg")
#' x <- image_read(path)
#' x
#' faces <- image_detect_faces(x)
#' faces
#' plot(faces, x, border = "red", lwd = 7, col = "white")
#' 
#' ## show one detected face
#' face <- head(faces$detections, 1)
#' image_crop(x, geometry_area(x = face$x, y = face$y, 
#'                             width = face$width, height = face$height))
#' ## show all detected faces
#' boxcontent <- lapply(seq_len(faces$nr), FUN=function(i){
#'   face <- faces$detections[i, ]
#'   image_crop(x, geometry_area(x = face$x, y = face$y, 
#'                               width = face$width, height = face$height))
#' })
#' boxcontent <- do.call(c, boxcontent)
#' boxcontent
plot.libfacedetection <- function(x, image, border = "red", lwd = 5, only_box = FALSE, col = "red", cex = 2, ...){
  stopifnot(inherits(image, "magick-image") && nrow(magick::image_info(image)) == 1)
  faces <- x$detections
  img <- magick::image_draw(image)
  lapply(seq_along(faces$x), FUN = function(i){
    face <- lapply(faces, FUN=function(x) x[i])
    graphics::rect(xleft = face$x, xright = face$x + face$width,
         ytop = face$y, ybottom = face$y + face$height, border = border, lwd = lwd, ...)
    if(!only_box){
      graphics::text(x = face$x, y = face$y, adj = 0.5, labels = face$neighbours, col = col, cex = cex)  
    }
  })
  img
}
