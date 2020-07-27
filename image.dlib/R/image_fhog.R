#' @title HOG (Histogram Of Gradients) feature extraction
#' @description Gets the histogram of oriented gradients (HOG) feature descriptor
#' \url{https://en.wikipedia.org/wiki/Histogram_of_oriented_gradients} of an image.
#' More information on the calculation, see the reference paper or
#' the dlib reference \url{http://dlib.net/dlib/image_transforms/fhog_abstract.h.html}
#' @param x a 3-dimensional array of integer pixel values where the first dimension is RGB, 2nd the width of the image and the 3rd the height of the image. See the example.
#' @param cell_size integer. HOG groups pixels in cells, based on which a histogram of the gradiens is computed. Give the cell size. Defaults to 8.
#' @param filter_rows_padding integer indicating how many extra rows and columns of zero padding along the borders to add, for more efficient convolution code
#' @param filter_cols_padding integer indicating how many extra rows and columns of zero padding along the borders to add, for more efficient convolution code
#' @references Object Detection with Discriminatively Trained Part Based Models by P. Felzenszwalb, R. Girshick, D. McAllester, D. Ramanan IEEE Transactions on Pattern Analysis and Machine Intelligence, Vol. 32, No. 9, Sep. 2010
#' @return a list with HOG features of the pixels mapped into HOG space. It contains:
#' \itemize{
#'  \item{hog_height: }{The size of the height of the image in HOG space}
#'  \item{hog_width: }{The size of the width of the image in HOG space}
#'  \item{hog_feature: }{The 31 dimensional FHOG vector, one for each pixel in HOG space. This is an array of dimension hog_height x hog_width x 31. The 31-dimensional vector describes the gradient structure within the cell.}
#'  \item{hog_cell_size: }{The HOG cell_size}
#'  \item{filter_rows_padding: }{The value of filter_rows_padding}
#'  \item{filter_cols_padding: }{The value of filter_cols_padding}
#' }
#' @export
#' @examples
#' library(magick)
#' f <- system.file("extdata", "cruise_boat.png", package = "image.dlib")
#' x <- image_read(f)
#' x
#' feats <- image_data(x, channels = "rgb")
#' feats <- as.integer(feats, transpose = FALSE)
#' feats <- image_fhog(feats, cell_size = 8)
#' str(feats)
#'
#' ## FHOG feature vector of pixel at HOG space position 1, 1
#' feats$fhog[1, 1, ]
#' ## FHOG feature vector of pixel at HOG space position 4 (height), 7 (width)
#' feats$fhog[4, 7, ]
image_fhog <- function(x, 
                       cell_size = 8L,
                       filter_rows_padding = 1L,
                       filter_cols_padding = 1L) {
  width  <- dim(x)[2]
  height <- dim(x)[3]
  
  out <- dlib_fhog(x, rows = height, cols = width,
            cell_size = as.integer(cell_size),
            filter_rows_padding = as.integer(filter_rows_padding),
            filter_cols_padding = as.integer(filter_cols_padding))
  out$fhog <- array(out$fhog, dim = c(out$hog_height, out$hog_width, 31))
  out
}
