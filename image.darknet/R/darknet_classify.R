#' @title Image classification with deep learning models AlexNet, Darknet, VGG-16, Extraction (GoogleNet) and Darknet19
#' @description Image classification with deep learning models AlexNet, Darknet, VGG-16, Extraction (GoogleNet) and Darknet19.
#' @param file character string with the full path to the image file
#' @param object an object of class \code{darknet_model} as returned by \code{\link{image_darknet_model}}
#' @param top integer indicating to return the top classifications only. Defaults to 5.
#' @return a list with elements 
#' \itemize{
#'  \item{file: }{the path to the file}
#'  \item{type: }{a data.frame with 2 columns called label and probability indicating the found class for that image and the probability of that class}
#' }
#' @export
#' @seealso \code{\link{image_darknet_model}}
#' @examples
#' ##
#' ## Define the model
#' ##
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "tiny.cfg")
#' weights <- system.file(package="image.darknet", "models", "tiny.weights")
#' f <- system.file(package="image.darknet", "include", "darknet", "data", "imagenet.shortnames.list")
#' labels <- readLines(f)
#' darknet_tiny <- image_darknet_model(type = 'classify', 
#'                                     model = model, weights = weights, labels = labels)
#' 
#' ##
#' ## Classify new images alongside the model
#' ##
#' f <- system.file("include", "darknet", "data", "dog.jpg", package="image.darknet")
#' x <- image_darknet_classify(file = f, object = darknet_tiny)
#' x
#' 
#' f <- system.file("include", "darknet", "data", "eagle.jpg", package="image.darknet")
#' image_darknet_classify(file = f, object = darknet_tiny)
#' 
#' \dontrun{
#' ## For other models, see ?image_darknet_model
#' 
#' ## darknet19
#' download.file(url = "http://pjreddie.com/media/files/darknet19.weights",
#'   destfile = system.file(package="image.darknet", "models", "darknet19.weights"))
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "darknet19.cfg")
#' weights <- system.file(package="image.darknet", "models", "darknet19.weights")
#' 
#' darknet19 <- image_darknet_model(type = 'classify', 
#'                                  model = model, weights = weights, labels = labels)
#' 
#' 
#' image_darknet_classify(file = f, object = darknet19)
#' }
image_darknet_classify <- function(file, object, top=5L) {
  stopifnot(file.exists(file))
  if(any(basename(file) == file)){
    stop("Give a full path to the file instead of a path inside the working directory")
  }
  top <- as.integer(top)
  result <- .Call("darknet_predict", object$datacfg, object$cfgfile, object$weightfile, 
                  file, top, object$labels, as.integer(object$resize), PACKAGE = "image.darknet")
  list(file = file, 
       type = data.frame(label = result[[2]], probability = result[[1]], stringsAsFactors = FALSE))
}



#' @title Specify a model to be used in classification or object detection
#' @description Specify a model to be used in classification or object detection.
#' For classification this consists of
#' \itemize{
#'  \item{model: }{the deep learning configuration file which was used to train the model}
#'  \item{weights: }{the trained weights of the deep learning model}
#'  \item{labels: }{a character vector of possible labels to classify an image - used during training}
#' }
#' @param type character string, either 'classify' for classification or 'detect' for object detection
#' @param model character string with the full path to the deep learning configuration file
#' @param weights character string with the full path to the trained deep learning weights
#' @param labels character vector of labels
#' @param resize logical indicating to resize the network. Defaults to TRUE. Set to FALSE for the Alexnet and VGG-16 model
#' @return an object of class darknet_model which is a list with these files
#' @export
#' @examples
#' ##
#' ## Define the model (structure of the deep learning model + the learned weights + the labels)
#' ##
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "tiny.cfg")
#' weights <- system.file(package="image.darknet", "models", "tiny.weights")
#' f <- system.file(package="image.darknet", "include", "darknet", "data", "imagenet.shortnames.list")
#' labels <- readLines(f)
#' 
#' darknet_tiny <- image_darknet_model(type = 'classify', 
#'                                     model = model, weights = weights, labels = labels)
#' darknet_tiny
#' 
#' ##
#' ## Other models which are trained already can be downloaded at: 
#' ## https://pjreddie.com/darknet/imagenet
#' ##
#' \dontrun{
#' ## AlexNet
#' download.file(url = "http://pjreddie.com/media/files/alexnet.weights",
#'   destfile = system.file(package="image.darknet", "models", "alexnet.weights"))
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "alexnet.cfg")
#' weights <- system.file(package="image.darknet", "models", "alexnet.weights")
#' 
#' alexnet <- image_darknet_model(type = 'classify', 
#'                                model = model, weights = weights, labels = labels, resize=FALSE)
#' alexnet
#' 
#' ## Darknet Reference
#' download.file(url = "http://pjreddie.com/media/files/darknet.weights",
#'   destfile = system.file(package="image.darknet", "models", "darknet.weights"))
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "darknet.cfg")
#' weights <- system.file(package="image.darknet", "models", "darknet.weights")
#' 
#' darknetref <- image_darknet_model(type = 'classify', 
#'                                   model = model, weights = weights, labels = labels)
#' darknetref
#' 
#' ## vgg-16
#' download.file(url = "http://pjreddie.com/media/files/16.weights",
#'   destfile = system.file(package="image.darknet", "models", "vgg-16.weights"))
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "vgg-16.cfg")
#' weights <- system.file(package="image.darknet", "models", "vgg-16.weights")
#' 
#' vgg16 <- image_darknet_model(type = 'classify', 
#'                              model = model, weights = weights, labels = labels, resize=FALSE)
#' vgg16
#' 
#' ## googlenet
#' download.file(url = "http://pjreddie.com/media/files/extraction.weights",
#'   destfile = system.file(package="image.darknet", "models", "extraction.weights"))
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "extraction.cfg")
#' weights <- system.file(package="image.darknet", "models", "extraction.weights")
#' 
#' googlenet <- image_darknet_model(type = 'classify', 
#'                                  model = model, weights = weights, labels = labels)
#' googlenet
#' 
#' ## darknet19
#' download.file(url = "http://pjreddie.com/media/files/darknet19.weights",
#'   destfile = system.file(package="image.darknet", "models", "darknet19.weights"))
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "darknet19.cfg")
#' weights <- system.file(package="image.darknet", "models", "darknet19.weights")
#' 
#' darknet19 <- image_darknet_model(type = 'classify', 
#'                                  model = model, weights = weights, labels = labels)
#' darknet19
#' 
#' ## Darknet19 448x448
#' download.file(url = "http://pjreddie.com/media/files/darknet19_448.weights",
#'   destfile = system.file(package="image.darknet", "models", "darknet19_448.weights"))
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "darknet19_448.cfg")
#' weights <- system.file(package="image.darknet", "models", "darknet19_448.weights")
#' 
#' darknet_19 <- image_darknet_model(type = 'classify', 
#'                                   model = model, weights = weights, labels = labels)
#' darknet_19
#' }
image_darknet_model <- function(type = c("classify", "detect"), model, weights, labels, resize=TRUE){
  stopifnot(file.exists(model))
  stopifnot(file.exists(weights))
  stopifnot(is.character(labels) & length(labels) > 1)
  type <- match.arg(type)
  out <- list()
  out$type <- type
  if(type == "classify"){
    out$datacfg <- "not_needed_any_more.data"
    out$cfgfile  <- model
    out$weightfile <- weights
    out$labels <- labels
    out$resize <- resize
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