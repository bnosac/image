# image.ContourDetector - Unsupervised Smooth Contour Line Detection for images

The  **image.ContourDetector** R package implements Unsupervised Smooth Contour Line Detection for images. 

- It contains 1 main function **image_contour_detector**. If you give it an image matrix with grey scale values in the 0-255 range, it will find contour lines in the image.
- The algorithm is defined in Rafael Grompone von Gioi, and Gregory Randall, Unsupervised Smooth Contour Detection, Image Processing On Line, 6 (2016), pp. 233-267. https://doi.org/10.5201/ipol.2016.175
- Mark that if you are looking in detecting lines, you might also be interested in https://github.com/bnosac/image/image.LineSegmentDetector


## Examples

Read in an image with values in the 0-255 range (pgm image: http://netpbm.sourceforge.net/doc/pgm.html)

```r
library(image.ContourDetector)
library(pixmap)
imagelocation <- system.file("extdata", "image.pgm", package="image.ContourDetector")
image <- read.pnm(file = imagelocation, cellres = 1)
x     <- image@grey * 255
contourlines <- image_contour_detector(x)
contourlines
plot(image)
plot(contourlines)
```
![](https://raw.githubusercontent.com/bnosac/image/master/image.ContourDetector/inst/extdata/cld-result.png?raw=true)

If you have another type of image (jpg, png, ...). Convert the image to portable grey format before applying the function

```r
library(magick)
x   <- image_read(system.file("extdata", "atomium.jpg", package="image.ContourDetector"))
mat <- image_data(x, channels = "gray")
mat <- as.integer(mat, transpose = TRUE)
mat <- drop(mat)
contourlines <- image_contour_detector(mat)
plot(contourlines)

plt <- image_draw(x)
contourlines$data$y <- image_info(x)$height - contourlines$data$y
plot(contourlines, add = TRUE, col = "red", lwd = 3)
dev.off()
```

![](https://raw.githubusercontent.com/bnosac/image/master/image.ContourDetector/inst/extdata/cld-result2.png?raw=true)


## Support in image recognition

Need support in image recognition?
Contact BNOSAC: http://www.bnosac.be

