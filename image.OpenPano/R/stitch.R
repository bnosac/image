#' @title Stitch images
#' @description Stitch images
#' @param x a character vector of paths to jpeg files to stitch
#' @param config the path to the config file of OpenPano
#' @param file path to the output file where the stitched image will be written unto. 
#' This should be a file with extension .jpg
#' @export
#' @return a list with elements x and file where \code{x} are the input files and \code{file}
#' is the stitched output file
#' @examples 
#' folder <- system.file(package = "image.OpenPano", "extdata")
#' images <- c(file.path(folder, "imga.jpg"), 
#'             file.path(folder, "imgb.jpg"),
#'             file.path(folder, "imgc.jpg"))
#' result <- image_stitch(images, file = "result_stitched.jpg")
#' 
#' library(magick)
#' image_read(images[1])
#' image_read(images[2])
#' image_read(images[3])
#' image_read("result_stitched.jpg")
#' 
#' file.remove("result_stitched.jpg")
image_stitch <- function(x, 
                         config = system.file(package = "image.OpenPano", "extdata", "config.cfg"),
                         file = tempfile(fileext = ".jpg")){
  stopifnot(all(file.exists(x)))
  stopifnot(all(tools::file_ext(x) == "jpg"))
  stopifnot(all(tools::file_ext(file) == "jpg"))
  openpano_stitch(x, config, file)
}
