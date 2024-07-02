#' @title Unsupervised Smooth Contour Lines Detection in an image
#' @description Unsupervised Smooth Contour Detection.\cr
#' 
#' Following the a contrario approach, the starting point is defining the conditions where contours should not be detected: 
#' soft gradient regions contaminated by noise. To achieve this, low frequencies are removed from the input image. 
#' Then, contours are validated as the frontiers separating two adjacent regions, one with significantly larger values 
#' than the other. Significance is evaluated using the Mann-Whitney U test to determine whether the samples were drawn 
#' from the same distribution or not. This test makes no assumption on the distributions. 
#' The resulting algorithm is similar to the classic Marr-Hildreth edge detector, 
#' with the addition of the statistical validation step. Combined with heuristics based on the Canny and Devernay methods, 
#' an efficient algorithm is derived producing sub-pixel contours.
#' @param x a matrix of image pixel values in the 0-255 range.
#' @param Q numeric value with the pixel quantization step
#' @param ... further arguments passed on to \code{image_contour_detector.matrix}, \code{\link{image_contour_detector.RasterLayer}} or \code{\link{image_contour_detector.SpatRaster}}.
#' @return an object of class cld which is a list with the following elements
#' \itemize{
#'  \item{curves: }{The number of contour lines found}
#'  \item{contourpoints: }{The number of points defining the contour lines found}
#'  \item{data: }{A data.frame with columns 'x', 'y' and 'curve' giving the x/y locations for each contour curve}
#' }
#' @references Rafael Grompone von Gioi, and Gregory Randall, Unsupervised Smooth Contour Detection, 
#' Image Processing On Line, 6 (2016), pp. 233-267. \doi{10.5201/ipol.2016.175}
#' @export
#' @examples
#' \dontshow{
#' if(require(pixmap))
#' \{
#' }
#' library(pixmap)
#' imagelocation <- system.file("extdata", "image.pgm", package="image.ContourDetector")
#' image         <- read.pnm(file = imagelocation, cellres = 1)
#' \dontshow{
#' image <- image[1:100, 1:100] ## speedup to have CRAN R CMD check within 5 secs
#' }
#' x             <- image@grey * 255
#' contourlines  <- image_contour_detector(x, Q = 2)
#' contourlines
#' plot(image)
#' plot(contourlines, add = TRUE, col = "red")
#' \dontshow{
#' \}
#' # End of main if statement running only if the required packages are installed
#' }
#' 
#' ##
#' ## line_segment_detector expects a matrix as input
#' ##  if you have a jpg/png/... convert it to pgm first or take the r/g/b channel
#' \dontshow{
#' if(require(magick))
#' \{
#' }
#' library(magick)
#' x   <- image_read(system.file("extdata", "atomium.jpg", package="image.ContourDetector"))
#' x
#' mat <- image_data(x, channels = "gray")
#' mat <- as.integer(mat, transpose = TRUE)
#' mat <- drop(mat)
#' contourlines <- image_contour_detector(mat)
#' plot(contourlines)
#' \dontshow{
#' \}
#' # End of main if statement running only if the required packages are installed
#' }
#' 
#' ##
#' ##  working with a SpatRaster
#' ##
#' \dontshow{
#' if(require(terra))
#' \{
#' }
#' \donttest{
#' library(terra)
#' x   <- rast(system.file("extdata", "landscape.tif", package="image.ContourDetector"))
#' 
#' contourlines <- image_contour_detector(x)
#' image(x)
#' plot(contourlines, add = TRUE, col = "blue", lwd = 10)
#' }
#' \dontshow{
#' \}
#' # End of main if statement running only if the required packages are installed
#' }
image_contour_detector <- function(x, Q=2.0, ...){
  UseMethod("image_contour_detector")
}

#' @export
image_contour_detector.matrix <- function(x, Q=2.0, ...){
  stopifnot(is.matrix(x))
  contourlines <- detect_contours(x, 
                  X=nrow(x),
                  Y=ncol(x),
                  Q=Q)
  names(contourlines) <- c("x", "y", "curvelimits", "curves", "contourpoints")
  contourlines$curvelimits <- contourlines$curvelimits+1L
  from <- contourlines$curvelimits
  to <- c(tail(contourlines$curvelimits, contourlines$curves-1L)-1L, contourlines$contourpoints)
  curve <- unlist(mapply(seq_along(from), from, to, FUN=function(contourid, from, to) rep(contourid, to-from+1L), SIMPLIFY = FALSE))
  contourlines$data <- data.frame(x = contourlines$y, y = nrow(x) - contourlines$x, curve = curve)
  contourlines <- contourlines[c("curves", "contourpoints", "data")]
  class(contourlines) <- "cld"
  return(contourlines)
}

#' @title Unsupervised Smooth Contour Lines Detection for RasterLayer objects
#' @description Unsupervised Smooth Contour Detection
#' @param x a RasterLayer object
#' @param Q numeric value with the pixel quantization step
#' @param as_sf Boolean. Set to TRUE to export lines as sf spatial objects
#' @param ... further arguments passed on to \code{image_contour_detector.matrix}
#' @return an object of class cld which as described in \code{\link{image_contour_detector}}
#' @seealso \code{\link{image_contour_detector}}
#' @export
image_contour_detector.RasterLayer <- function(x, Q=2.0, as_sf=FALSE, ...){
  requireNamespace("raster")
  minX = raster::extent(x)[1]
  minY = raster::extent(x)[3]  
  resol = raster::res(x)[1]  
  x = raster::as.matrix(x)
  
  if( anyNA(x) ){
    x[is.na(x)] = 0
    warning("NA values found and set to 0") }

  contourlines = image_contour_detector.matrix(x, Q=Q, ...)
  
  contourlines$data$x = contourlines$data$x * resol + minX
  contourlines$data$y = contourlines$data$y * resol + minY
  return(contourlines)
}

#' @title Unsupervised Smooth Contour Lines Detection for SpatRaster objects
#' @param x a SpatRaster object
#' @param Q numeric value with the pixel quantization step
#' @param as_sf Boolean. Set to TRUE to export lines as sf spatial objects
#' @param ... further arguments passed on to \code{image_contour_detector.matrix}
#' @return an object of class cld which as described in \code{\link{image_contour_detector}}
#' @seealso \code{\link{image_contour_detector}}
#' @export
image_contour_detector.SpatRaster <- function(x, Q=2.0, as_sf=FALSE, ...){
  requireNamespace("terra")
  minX = terra::ext(x)[1]
  minY = terra::ext(x)[3]  
  resol = terra::res(x)[1]  
  xmat = terra::as.matrix(x, wide=TRUE)
  
  if( anyNA(xmat) ){
    x[is.na(xmat)] = 0
    warning("NA values found and set to 0") }

  contourlines = image_contour_detector.matrix(xmat, Q=Q, ...)
  
  contourlines$data$x = contourlines$data$x * resol + minX
  contourlines$data$y = contourlines$data$y * resol + minY

  # export object as sf
  if(isTRUE(as_sf)){
    requireNamespace("sf")
    
  # contourlines <- contourlines$data %>%
  # sf::st_as_sf(coords = c("x", "y"), crs = sf::st_crs(x) ) %>%
  # dplyr::group_by( curve ) %>%
  # dplyr::summarize(do_union=FALSE) %>%
  # sf::st_cast("LINESTRING")

    contourlines <- contourlines$data
    contourlines = sf::st_as_sf( contourlines, coords = c("x", "y"), crs = sf::st_crs(x) )
    
    out = list()
    
    for( i in unique(contourlines$curve) ){
      ss = subset(contourlines, curve == i )
      ss = sf::st_combine(ss)      
      out[[length(out)+1]] = sf::st_cast( st_sf(ss), "LINESTRING")
    }
    
    contourlines = do.call(rbind,out)    
    contourlines$length_m = sf::st_length(contourlines)    
  }  
  return(contourlines)
}
                         

#' @export
print.cld <- function(x, ...){
  cat("Contour Lines Detector", sep = "\n")
  cat(sprintf("  found %s contour lines", x$curves), sep = "\n")
}


#' @title Plot the detected contour lines from the image_contour_detector
#' @description Plot the detected contour lines from the image_contour_detector
#' @param x an object of class cld as returned by \code{\link{image_contour_detector}}
#' @param ... further arguments passed on to plot
#' @method plot cld
#' @return invisibly a SpatialLines object with the contour lines
#' @export 
#' @examples 
#' \dontshow{
#' if(require(pixmap))\{
#' }
#' library(pixmap)
#' imagelocation <- system.file("extdata", "image.pgm", package="image.ContourDetector")
#' image         <- read.pnm(file = imagelocation, cellres = 1)
#' contourlines  <- image_contour_detector(image@grey * 255)
#' plot(image)
#' plot(contourlines, add = TRUE, col = "red")
#' \dontshow{
#' \} # End of main if statement running only if the required packages are installed
#' }
plot.cld <- function(x, ...){
  requireNamespace("sp")
  l <- split(x$data, x$data$curve)
  out <- sp::SpatialLines(lapply(l, FUN=function(x){
    sp::Lines(sp::Line(x[, c("x", "y")]), ID = head(x$curve, 1))
  }))
  sp::plot(out, ...)
  invisible(out)
}
