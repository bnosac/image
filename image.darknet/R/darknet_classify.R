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
#' weights <- file.path(system.file(package="image.darknet", "models"), "darknet19.weights")
#' download.file(url = "http://pjreddie.com/media/files/darknet19.weights", destfile = weights)
#' model <- system.file(package="image.darknet", "include", "darknet", "cfg", "darknet19.cfg")
#' 
#' darknet19 <- image_darknet_model(type = 'classify', 
#'                                  model = model, weights = weights, labels = labels)
#' image_darknet_classify(file = f, object = darknet19)
#' }
image_darknet_classify <- function(file, object, top=5L) {
  stopifnot(file.exists(file))
  stopifnot(object$type == "classify")
  if(any(basename(file) == file)){
    stop("Give a full path to the file instead of a path inside the working directory")
  }
  top <- as.integer(top)
  result <- .Call("darknet_predict", object$datacfg, object$cfgfile, object$weightfile, 
                  file, top, object$labels, as.integer(object$resize), PACKAGE = "image.darknet")
  list(file = file, 
       type = data.frame(label = result[[2]], probability = result[[1]], stringsAsFactors = FALSE))
}
