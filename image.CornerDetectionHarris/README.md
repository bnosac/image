# image.CornerDetectionHarris - Detect Harris Points in images 

The  **image.CornerDetectionHarris** R package detects relevant points in images, namely:

- An implementation of the Harris Corner Detection as described in the paper "An Analysis and Implementation of the Harris Corner Detector" by Sánchez J. et al. The package allows to detect characteristic points in digital images. 
- More details in the paper at <https://www.ipol.im/pub/art/2018/229/>

## Examples

```r
library(magick)
library(image.CornerDetectionHarris)
path <- system.file(package = "image.CornerDetectionHarris", "extdata", "building.png")
img  <- image_read(path)
pts  <- image_harris(img)
pts

plt <- image_draw(img)
points(pts$x, pts$y, col = "red", pch = 20)
dev.off()
plt
```

![](https://raw.githubusercontent.com/bnosac/image/master/image.CornerDetectionHarris/inst/extdata/building-result.png?raw=true)


## Support in image recognition

Need support in image recognition?
Contact BNOSAC: http://www.bnosac.be