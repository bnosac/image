#' @title The image.ContourDetector package detects contour lines in images 
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
#' @references Rafael Grompone von Gioi, and Gregory Randall, Unsupervised Smooth Contour Detection, 
#' Image Processing On Line, 6 (2016), pp. 233-267. \url{https://doi.org/10.5201/ipol.2016.175}
#' @name image.ContourDetector-package
#' @aliases image.ContourDetector-package
#' @docType package
#' @seealso \link{image_contour_detector}
#' @importFrom Rcpp evalCpp
#' @importFrom utils tail head
#' @importFrom sp Line Lines SpatialLines plot
#' @importFrom graphics plot
#' @useDynLib image.ContourDetector
NULL
