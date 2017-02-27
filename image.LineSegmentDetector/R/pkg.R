#' @title The image.LineSegmentDetector (LSD) package detects line segments in images
#' @description LSD is a linear-time Line Segment Detector giving subpixel accurate results.
#' It is designed to work on any digital image without parameter tuning.
#' It controls its own number of false detections: On average, one false alarm is allowed per image.
#' The method is based on Burns, Hanson, and Riseman's method, and uses an a-contrario validation approach according
#' to Desolneux, Moisan, and Morel's theory.
#' @references LSD: A Fast Line Segment Detector with a False Detection Control"
#' by Rafael Grompone von Gioi, Jeremie Jakubowicz, Jean-Michel Morel, and Gregory Randall, IEEE Transactions on Pattern Analysis and
#' Machine Intelligence, vol. 32, no. 4, pp. 722-732, April, 2010. \url{https://doi.org/10.5201/ipol.2012.gjmr-lsd}
#' @name image.LineSegmentDetector-package
#' @aliases image.LineSegmentDetector-package
#' @docType package
#' @seealso \link{image_line_segment_detector}
#' @importFrom Rcpp evalCpp
#' @importFrom sp Line Lines SpatialLines
#' @importFrom graphics plot
#' @useDynLib image.LineSegmentDetector
NULL
