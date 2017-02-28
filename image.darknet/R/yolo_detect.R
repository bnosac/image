#' @title Object detection with YOLO v2 (You only look once)
#' @description Object detection with YOLO v2 (You only look once)
#' @references \url{https://pjreddie.com/publications}
#' @param file character string with the full path to the image file
#' @param object an object of class \code{darknet_model} as returned by \code{\link{image_darknet_model}}
#' @param threshold numeric, detection threshold
#' @param hier_threshold numeric, detection threshold
#' @return invisible(), a file called predictions.png is created in the working directory showing 
#' the objects found
#' @export
#' @seealso \code{\link{image_darknet_model}}
#' @examples 
#' ##
#' ## Define the model
#' ##
#' yolo_tiny_voc <- image_darknet_model(type = 'detect', 
#'  model = "tiny-yolo-voc.cfg", 
#'  weights = system.file(package="image.darknet", "models", "tiny-yolo-voc.weights"), 
#'  labels = system.file(package="image.darknet", "include", "darknet", "data", "voc.names"))
#' yolo_tiny_voc
#' 
#' ##
#' ## Find objects inside the image
#' ##
#' f <- system.file("include", "darknet", "data", "dog.jpg", package="image.darknet")
#' x <- image_darknet_detect(file = f, object = yolo_tiny_voc)
#' 
#' \dontrun{
#' ## For other models, see ?image_darknet_model
#' 
#' ## trained on COCO
#' weights <- file.path(system.file(package="image.darknet", "models"), "yolo.weights")
#' download.file(url = "http://pjreddie.com/media/files/yolo.weights", destfile = weights)
#' yolo_coco <- image_darknet_model(type = 'detect', 
#'  model = "yolo.cfg", 
#'  weights = system.file(package="image.darknet", "models", "yolo.weights"), 
#'  labels = system.file(package="image.darknet", "include", "darknet", "data", "coco.names"))
#' yolo_coco
#' 
#' f <- system.file("include", "darknet", "data", "dog.jpg", package="image.darknet")
#' x <- image_darknet_detect(file = f, object = yolo_coco)
#' }
image_darknet_detect <- function(file, object, threshold = 0.3, hier_threshold = 0.5) {
  stopifnot(file.exists(file))
  stopifnot(object$type == "detect")
  if(any(basename(file) == file)){
    stop("Give a full path to the file instead of a path inside the working directory")
  }
  threshold <- as.numeric(threshold)
  hier_threshold <- as.numeric(hier_threshold)
  result <- .Call("darknet_detect", 
                  object$cfgfile, 
                  object$weightfile, 
                  file, 
                  threshold, hier_threshold, 
                  object$labels,
                  system.file(package = "image.darknet", "include", "darknet"),
                  PACKAGE = "image.darknet")
  invisible()
}