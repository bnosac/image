#' @title Specify a model to be used in classification or object detection
#' @description Specify a model to be used in classification or object detection.
#' For classification this consists of
#' \itemize{
#'  \item{model: }{the deep learning configuration file which was used to train the model}
#'  \item{weights: }{the trained weights of the deep learning model}
#'  \item{labels: }{a character vector of possible labels to classify an image - used during training}
#' }
#' Available models are 
#' \itemize{
#'  \item{classification: }{Alexnet, Darknet, VGG-16, Extraction(GoogleNet), Darknet19, Darknet19_448, Tiny Darknet trained on Imagenet}
#'  \item{detection: }{YOLO, YOLO tiny trained on VOC and on COCO}
#' }
#' @param type character string, either 'classify' for classification or 'detect' for object detection
#' @param model character string with the full path to the deep learning configuration file or
#' one of the following preset models: classification: tiny.cfg, alexnet.cfg, darknet.cfg, vgg-16.cfg, 
#' extraction.cfg, darknet19.cfg, darknet19_448.cfg which are available in the includ/darket/cfg folder
#' of this package
#' @param weights character string with the full path to the trained deep learning weights
#' @param labels character vector of labels
#' @param resize logical indicating to resize the network if type is 'classify'. 
#' Defaults to TRUE. Set to FALSE for the Alexnet and VGG-16 model
#' @return an object of class darknet_model which is a list with these files
#' @export
#' @examples
#' ##
#' ## Define the classification model 
#' ## (structure of the deep learning model + the learned weights + the labels)
#' ##
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "tiny.cfg")
#' weights <- system.file(package="image.darknet", "models", "tiny.weights")
#' f <- system.file(package="image.darknet", "include", "darknet", "data", "imagenet.shortnames.list")
#' labels <- readLines(f)
#' darknet_tiny <- image_darknet_model(type = 'classify', 
#'                                     model = "tiny.cfg", weights = weights, labels = labels)
#' darknet_tiny
#' \dontrun{
#' ##
#' ## Other CLASSIFICATION models which are trained already can be downloaded at: 
#' ## https://pjreddie.com/darknet/imagenet
#' ##
#' 
#' ## AlexNet
#' weights <- file.path(system.file(package="image.darknet", "models"), "alexnet.weights")
#' download.file(url = "http://pjreddie.com/media/files/alexnet.weights", destfile = weights)
#' alexnet <- image_darknet_model(type = 'classify', 
#'   model = "alexnet.cfg", weights = weights, labels = labels, resize=FALSE)
#' alexnet
#' 
#' ## Darknet Reference
#' weights <- file.path(system.file(package="image.darknet", "models"), "darknet.weights")
#' download.file(url = "http://pjreddie.com/media/files/darknet.weights", destfile = weights)
#' darknetref <- image_darknet_model(type = 'classify', 
#'   model = "darknet.cfg", weights = weights, labels = labels)
#' darknetref
#' 
#' ## vgg-16
#' weights <- file.path(system.file(package="image.darknet", "models"), "vgg-16.weights")
#' download.file(url = "http://pjreddie.com/media/files/vgg-16.weights", destfile = weights)
#' vgg16 <- image_darknet_model(type = 'classify', 
#'   model = "vgg-16.cfg", weights = weights, labels = labels, resize=FALSE)
#' vgg16
#' 
#' ## googlenet/extraction
#' weights <- file.path(system.file(package="image.darknet", "models"), "extraction.weights")
#' download.file(url = "http://pjreddie.com/media/files/extraction.weights", destfile = weights)
#' googlenet <- image_darknet_model(type = 'classify', 
#'   model = "extraction.cfg", weights = weights, labels = labels)
#' googlenet
#' 
#' ## darknet19
#' weights <- file.path(system.file(package="image.darknet", "models"), "darknet19.weights")
#' download.file(url = "http://pjreddie.com/media/files/darknet19.weights", destfile = weights)
#' darknet19 <- image_darknet_model(type = 'classify', 
#'   model = "darknet19.cfg", weights = weights, labels = labels)
#' darknet19
#' 
#' ## Darknet19 448x448
#' weights <- file.path(system.file(package="image.darknet", "models"), "darknet19_448.weights")
#' download.file(url = "http://pjreddie.com/media/files/darknet19_448.weights", destfile = weights)
#' darknet_19 <- image_darknet_model(type = 'classify', 
#'   model = "darknet19_448.cfg", weights = weights, labels = labels)
#' darknet_19
#' }
#' 
#' 
#' 
#' ##
#' ## Define the detection model (YOLO) 
#' ## (structure of the deep learning model + the learned weights + the labels)
#' ##
#' f <- system.file(package="image.darknet", "include", "darknet", "data", "voc.names")
#' labels <- readLines(f)
#' 
#' yolo_tiny_voc <- image_darknet_model(type = 'detect', 
#'  model = "tiny-yolo-voc.cfg", 
#'  weights = system.file(package="image.darknet", "models", "tiny-yolo-voc.weights"), 
#'  labels = labels)
#' yolo_tiny_voc
#' 
#' \dontrun{
#' ##
#' ## Other DETECTION models which are trained already can be downloaded at: 
#' ## https://pjreddie.com/darknet/yolo/
#' ##
#' weights <- file.path(system.file(package="image.darknet", "models"), "yolo-voc.weights")
#' download.file(url = "http://pjreddie.com/media/files/yolo-voc.weights", destfile = weights)
#' yolo_voc <- image_darknet_model(type = 'detect', 
#'  model = "yolo-voc.cfg", 
#'  weights = system.file(package="image.darknet", "models", "yolo-voc.weights"), 
#'  labels = labels)
#' yolo_voc
#' 
#' 
#' ## trained on COCO
#' f <- system.file(package="image.darknet", "include", "darknet", "data", "coco.names")
#' labels <- readLines(f)
#' 
#' weights <- file.path(system.file(package="image.darknet", "models"), "tiny-yolo.weights")
#' download.file(url = "http://pjreddie.com/media/files/tiny-yolo.weights", destfile = weights)
#' yolo_tiny_coco <- image_darknet_model(type = 'detect', 
#'  model = "tiny-yolo.cfg", 
#'  weights = system.file(package="image.darknet", "models", "tiny-yolo.weights"), 
#'  labels = labels)
#' yolo_tiny_coco
#' 
#' weights <- file.path(system.file(package="image.darknet", "models"), "yolo.weights")
#' download.file(url = "http://pjreddie.com/media/files/yolo.weights", destfile = weights)
#' yolo_coco <- image_darknet_model(type = 'detect', 
#'  model = "yolo.cfg", 
#'  weights = system.file(package="image.darknet", "models", "yolo.weights"), 
#'  labels = labels)
#' yolo_coco
#' }
image_darknet_model <- function(type = c("classify", "detect"), model, weights, labels, resize=TRUE){
  if(model %in% c("tiny.cfg", "alexnet.cfg", "darknet.cfg", "vgg-16.cfg", 
                  "extraction.cfg", "darknet19.cfg", "darknet19_448.cfg",
                  "yolo.cfg", "tiny-yolo.cfg", "yolo-voc", "tiny-yolo-voc.cfg",
                  "yolov2.cfg", "yolov2-voc.cfg", "yolov3.cfg", "yolov3-voc.cfg")){
    model <- system.file(package="image.darknet", "include", "darknet", "cfg", model)
  }
  stopifnot(file.exists(model))
  stopifnot(file.exists(weights))
  stopifnot(is.character(labels))
  if(length(labels) == 1){
    stopifnot(file.exists(labels))
    labels <- readLines(labels)
    stopifnot(length(labels) > 1)
  }
  type <- match.arg(type)
  out <- list()
  out$type <- type
  if(type == "classify"){
    out$datacfg <- "not_needed_any_more.data"
    out$cfgfile  <- model
    out$weightfile <- weights
    out$labels <- labels
    out$resize <- resize
  }else if(type == "detect"){
    out$datacfg <- "not_needed_any_more.data"
    out$cfgfile  <- model
    out$weightfile <- weights
    out$labels <- labels
  }
  class(out) <- "darknet_model"
  out
}


#' @export
print.darknet_model <- function(x, ...){
  cat("Darknet model", sep = "\n")
  cat(sprintf("  number of labels: %s", length(x$labels)), sep = "\n")
  cat(sprintf("  example labels: %s", paste(head(x$labels, 10), collapse = ", ")), sep = "\n\n")
  cat("Darknet model structure", sep = "\n\n")   
  cat(readLines(x$cfgfile), sep = "\n")   
}