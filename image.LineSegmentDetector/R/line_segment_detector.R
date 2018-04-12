#' @title Line Segments Detection (LSD) in an image
#' @description LSD is a linear-time Line Segment Detector giving subpixel accurate results.
#' It is designed to work on any digital image without parameter tuning.
#' It controls its own number of false detections (NFA): On average, one false alarm is allowed per image.
#' The method is based on Burns, Hanson, and Riseman's method, and uses an a-contrario validation approach according
#' to Desolneux, Moisan, and Morel's theory.
#'
#' In general, no changes are needed on the arguments. The default arguments provide very good line detections for any
#' digital image without changing the parameters.
#' @param x a matrix of image pixel values in the 0-255 range.
#' @param scale Positive numeric value. When different from 1.0, LSD will scale the input image by 'scale' factor by Gaussian filtering,
#' before detecting line segments. Example: if scale=0.8, the input image will be subsampled
#' to 80 percent of its size, before the line segment detector is applied. Suggested value: 0.8
#' @param sigma_scale Positive numeric value. When scale != 1.0, the sigma of the Gaussian filter is:
#' sigma = sigma_scale / scale,   if scale <  1.0 and
#' sigma = sigma_scale,           if scale >= 1.0. Suggested value: 0.6
#' @param quant Positive numeric value. Bound to the quantization error on the gradient norm.
#' Example: if gray levels are quantized to integer steps,
#' the gradient (computed by finite differences) error due to quantization will be bounded by 2.0, as the
#' worst case is when the error are 1 and -1, that gives an error of 2.0. Suggested value: 2.0
#' @param ang_th Positive numeric value in 0-180 range. Gradient angle tolerance in the region growing algorithm, in degrees. Suggested value: 22.5
#' @param log_eps Detection threshold, accept if -log10(NFA) > log_eps. The larger the value, the more strict the detector is,
#' and will result in less detections. (Note that the 'minus sign' makes that this behavior is opposite to the one of NFA.).
#' Suggested value: 0.0.
#' @param density_th Positive numeric value in 0-1 range. Minimal proportion of 'supporting' points in a rectangle. Suggested value: 0.7.
#' @param n_bins Positive integer value. Number of bins used in the pseudo-ordering of gradient modulus. Suggested value: 1024
#' @param union Logical indicating if you need to post procces image by union close segments. Defaults to FALSE.
#' @param union_min_length Numeric value with minimum length of segment to union
#' @param union_max_distance Numeric value with maximum distance between two line which we would union
#' @param union_ang_th Numeric value with angle threshold in order to union
#' @param union_use_NFA Logical indicating to use NFA to union
#' @param union_log_eps Detection threshold to union
#' @return an object of class lsd which is a list with the following elements
#' \itemize{
#'  \item{n: }{The number of found line segments}
#'  \item{lines: }{A matrix with detected line segments with columns x1, y1, x2, y2, width, p, -log_nfa".
#'  Each row corresponds to a line where a line segment is given by the coordinates (x1, y1) to (x2, y2).
#'  Column width indicates the width of the line, p is the angle precision in (0, 1) given
#'  by angle_tolerance/180 degree, and NFA stands for the Number of False Alarms.
#'  The value -log10(NFA) is equivalent but more intuitive than NFA (Number of False Alarms):
#'  -1 corresponds to 10 mean false alarms, 0 corresponds to 1 mean false alarm,
#'  1 corresponds to 0.1 mean false alarms, 2 corresponds to 0.01 mean false alarms}
#'  \item{pixels: }{A matrix where each element correponds to a pixel where each pixel
#'  indicates the line segment to which it belongs. Unused pixels have the value '0',
#'  while the used ones have the number of the line segment, numbered 1,2,3,...,
#'  in the same order as in the lines matrix}
#' }
#' @references LSD: A Fast Line Segment Detector with a False Detection Control"
#' by Rafael Grompone von Gioi, Jeremie Jakubowicz, Jean-Michel Morel, and Gregory Randall, IEEE Transactions on Pattern Analysis and
#' Machine Intelligence, vol. 32, no. 4, pp. 722-732, April, 2010. \url{https://doi.org/10.5201/ipol.2012.gjmr-lsd}
#' @export
#' @examples
#' library(pixmap)
#' imagelocation <- system.file("extdata", "chairs.pgm", package="image.LineSegmentDetector")
#' image <- read.pnm(file = imagelocation, cellres = 1)
#' x <- image@grey * 255
#'
#' linesegments <- image_line_segment_detector(x)
#' linesegments
#' plot(image)
#' plot(linesegments, add = TRUE, col = "red")
#'
#' \dontrun{
#' imagelocation <- system.file("extdata", "le-piree.pgm", package="image.LineSegmentDetector")
#' image <- read.pnm(file = imagelocation, cellres = 1)
#' linesegments <- image_line_segment_detector(image@grey * 255)
#' plot(image)
#' plot(linesegments)
#'
#' ##
#' ## image_line_segment_detector expects a matrix as input
#' ##  if you have a jpg/png/... convert it to pgm first or take the r/g/b channel
#' f <- tempfile(fileext = ".pgm")
#' library(magick)
#' x <- image_read(system.file("extdata", "atomium.jpg", package="image.LineSegmentDetector"))
#' x <- image_convert(x, format = "pgm", depth = 8)
#' image_write(x, path = f, format = "pgm")
#'
#' image <- read.pnm(f, cellres = 1)
#' linesegments <- image_line_segment_detector(image@grey * 255)
#' plot(image)
#' plot(linesegments)
#' }
image_line_segment_detector <- function(x, scale = 0.8,
                                  sigma_scale = 0.6, quant = 2.0, ang_th = 22.5, log_eps = 0.0,
                                  density_th = 0.7, n_bins = 1024,
                                  union = FALSE, union_min_length = 5, union_max_distance = 5,
                                  union_ang_th=7, union_use_NFA=FALSE, union_log_eps = 0.0) {


  stopifnot(is.matrix(x))
  linesegments <- detect_line_segments(as.numeric(x),
                                       X=nrow(x),
                                       Y=ncol(x),
                                       scale = as.numeric(scale),
                                       sigma_scale = as.numeric(sigma_scale),
                                       quant = as.numeric(quant),
                                       ang_th = as.numeric(ang_th),
                                       log_eps = as.numeric(log_eps),
                                       need_to_union = as.logical(union),
                                       union_use_NFA = as.logical(union_use_NFA),
                                       union_ang_th = as.numeric(union_ang_th),
                                       union_log_eps = as.numeric(union_log_eps),
                                       length_threshold = as.numeric(union_min_length),
                                       dist_threshold = as.numeric(union_max_distance))
  names(linesegments) <- c("lines", "pixels")
  colnames(linesegments$lines) <- c("x1", "y1", "x2", "y2", "width", "p", "-log_nfa")
  linesegments$n <- nrow(linesegments$lines)
  class(linesegments) <- "lsd"
  linesegments
}

#' @export
print.lsd <- function(x, ...){
  cat("Line Segment Detector", sep = "\n")
  cat(sprintf("  found %s line segments", x$n), sep = "\n")
}


#' @title Plot the detected lines from the image_line_segment_detector
#' @description Plot the detected lines from the image_line_segment_detector
#' @param x an object of class lsd as returned by \code{\link{image_line_segment_detector}}
#' @param ... further arguments passed on to plot
#' @return invisibly a SpatialLines object with the lines
#' @export 
#' @method plot lsd
#' @examples 
#' library(pixmap)
#' imagelocation <- system.file("extdata", "le-piree.pgm", package="image.LineSegmentDetector")
#' image <- read.pnm(file = imagelocation, cellres = 1)
#' linesegments <- image_line_segment_detector(image@grey * 255)
#' plot(image)
#' plot(linesegments, add = TRUE, col = "red")
plot.lsd <- function(x, ...){
  requireNamespace("sp")
  out <- sp::SpatialLines(lapply(seq_len(x$n), FUN=function(i){
    l <- rbind(
      x$lines[i, c("x1", "y1")],
      x$lines[i, c("x2", "y2")])
    sp::Lines(sp::Line(l), ID = i)
  }))
  sp::plot(out, ...)
  invisible(out)
}
