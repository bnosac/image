to_grey <- function(r, g, b){
  0.2989*r+0.5870*g+0.1140*b
}
library(magick)
library(pixmap)
m <- matrix(c(0, 1/6, 2/6, 3/6, 4/6, 5/6), nrow = 2, ncol = 3)
x <- image_read(as.raster(m))
image_write(x, path = "test.png", format = "png")
d <- image.CornerDetectionHarris:::check()
d
f <- tempfile(fileext = ".pgm")
image_write(x, path = f, format = "pgm")
go <- image_read(f) %>% image_data %>% as.integer
go <- go[, , 1]
image_read(f) %>% image_data %>% as.integer %>% as.matrix %>% t %>% as.vector


out <- image_convert(x, colorspace = "Rec601Luma")
out <- image_data(x)
out <- as.integer(out)[, , 1]
as.vector(out)
to_grey(out[, , 1], out[, , 2], out[, , 3])

x <- image_read(path = "image.CornerDetectionHarris/dev/building.png")
x <- image_convert(x, colorspace = "Rec601Luma")
#x <- image_scale(x, "500!x500!")
x
img <- image_data(x)
img <- as.integer(img)
img <- aperm(img, c(2, 1, 3))
image_write(x, path = "test.png", format = "png")
m <- image.CornerDetectionHarris:::check()
x = 1; y=1;
to_grey(img[x, y, 1], img[x, y, 2], img[x, y, 3])
x = 2; y=1;
to_grey(img[x, y, 1], img[x, y, 2], img[x, y, 3])
x = 1; y=2;
to_grey(img[x, y, 1], img[x, y, 2], img[x, y, 3])
x = 1; y=1200;
to_grey(img[x, y, 1], img[x, y, 2], img[x, y, 3])


x <- image_read(path = "image.CornerDetectionHarris/dev/building.png")
x <- image_convert(x, colorspace = "Gray")
#x <- image_scale(x, "500!x500!")
x
img <- image_data(x)
img <- as.integer(img)
img <- to_grey(img[, , 1], img[, , 2], img[, , 3])
img <- as.vector(img[, , 1])
points <- image.CornerDetectionHarris:::detect_corners(img, 500, 500)

library(pixmap)
f <- tempfile(fileext = ".pgm")
image_write(x, path = f, format = "pgm")
image <- read.pnm(f, cellres = 1)
plot(image)
points(points$y, 500-points$x, col = "red", pch = 20, lwd = 0.5)

x <- image_read(path = "image.CornerDetectionHarris/dev/building.png")
x <- image_scale(x, "400!x300!")
x <- image_convert(x, colorspace = "Gray")
bitmap <- aperm(image_data(x, channels = 'gray'), c(2, 3, 1))
bitmap <- as.integer(bitmap,  transpose = TRUE)
points <- image.CornerDetectionHarris:::detect_corners(img, 400, 300)
plot(points$y, 500-points$x, col = "red", pch = 20, lwd = 0.5)
plot(points$x, points$y, col = "red", pch = 20, lwd = 0.5)

x <- image_read(path = "image.CornerDetectionHarris/dev/building.png")
x <- image_scale(x, "600!x400!")
f <- tempfile(fileext = ".pgm")
image_write(x, path = f, format = "pgm")
image <- read.pnm(f, cellres = 1)

img <- image_read(f)
go <- img %>% image_data %>% as.integer
go <- go[, , 1]
go <- as.vector(t(go))
points <- image.CornerDetectionHarris:::detect_corners(go, 800, 600)
plot(image)
points(points$x, 300-points$y, col = "red", pch = 20, lwd = 0.5)


xyz <- image_draw(x)
points(points$x, points$y, col = "red", pch = 20, lwd = 0.5)
dev.off()

#By default image_draw() sets all margins to 0 and uses graphics coordinates to match 
# image size in pixels (width x height) where (0,0) is the top left corner. 
# Note that this means the y axis increases from top to bottom which is the opposite of typical graphics coordinates. You can override all this by passing custom xlim, ylim or mar values to image_draw.

gray <- image_convert(x, colorspace = "Gray")

go <- x %>% image_data(channels = "gray") %>% as.integer
go <- go[, , 1]
go <- t(go)
points <- image.CornerDetectionHarris:::detect_corners(go, 800, 600)
xyz <- image_draw(x)
points(points$x, points$y, col = "red", pch = 20, lwd = 0.5)


plot(image)

points(points$x, 300-points$y, col = "red", pch = 20, lwd = 0.5)
