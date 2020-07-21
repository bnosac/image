# image.CannyEdges - Canny Edge Detector for Images

The  **image.CannyEdges** R package detects edges in images. 

- It contains 1 main function **image_canny_edge_detector**. If you give it an image matrix with grey scale values in the 0-255 range, it will find edges in the image.

## Examples

Read in an image with values in the 0-255 range (pgm image: http://netpbm.sourceforge.net/doc/pgm.html)

```r
library(image.CannyEdges)
library(pixmap)
imagelocation <- system.file("extdata", "chairs.pgm", package="image.CannyEdges")
image <- read.pnm(file = imagelocation, cellres = 1)
x <- image@grey * 255

edges <- image_canny_edge_detector(x)
edges
plot(edges)
```

![](https://raw.githubusercontent.com/bnosac/image/master/image.CannyEdges/inst/extdata/chairs-result.png)


## Installation

The package requires libpng and fftw3 to be installed
In Ubuntu this is done as follows

```
sudo apt-get install libpng-dev fftw3 fftw3-dev
```

## Support in image recognition

Need support in image recognition?
Contact BNOSAC: http://www.bnosac.be

