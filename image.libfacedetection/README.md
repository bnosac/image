# image.libfacedetection - Detect Faces in Images

The **image.libfacedetection** provides a pretrained convolutional neural network to detect faces in images.

## Examples

```r
library(magick)
library(image.libfacedetection)
path  <- system.file(package="image.libfacedetection", "images", "handshake.jpg")
x     <- image_read(path)
faces <- image_detect_faces(x)
faces
plot(faces, x, border = "red", lwd = 7, col = "white")
```

![](https://raw.githubusercontent.com/bnosac/image/master/image.libfacedetection/inst/images/libfacedetection-example.gif?raw=true)

## Support in image recognition

Need support in image recognition?
Contact BNOSAC: http://www.bnosac.be

