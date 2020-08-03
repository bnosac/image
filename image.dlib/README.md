# image.dlib

The **image.dlib** package allows to quickly obtain image features (SURF and HOG features) and to do image matching. 

## Examples

```r
library(magick)
library(image.dlib)

## Read 2 images
img1   <- image_read("input_0.png")
img2   <- image_read("input_1.png")
height <- image_info(img1)$height
```

```r
## Get the surf descriptors
sp1 <- image_surf(image_data(img1, channels = "rgb"), max_points = 50)
sp2 <- image_surf(image_data(img2, channels = "rgb"), max_points = 50)

## Match surf points 
library(FNN)
knn <- get.knnx(sp1$surf, sp2$surf, k = 1)

## Plot the 2 images and the top 15 matches of the SURF points
combined <- image_append(c(img1, img2), stack = TRUE)
plt <- image_draw(combined)
points(sp1$x, sp1$y, col = "red", pch = 20)
points(sp2$x, sp2$y + height, col = "blue", pch = 20)
for(i_from in head(order(knn$nn.dist), 15)){
  i_to   <- knn$nn.index[i_from]
  lines(x = c(sp1$x[i_to], sp2$x[i_from]), y = c(sp1$y[i_to], sp2$y[i_from] + height), col = "red")
}
dev.off()
plt
```

![](https://raw.githubusercontent.com/bnosac/image/master/image.dlib/inst/images/imagematching-example.png?raw=true)

- Look to the documentation of the R package for examples: `help(package = image.dlib)`


## Support in image recognition

Need support in image recognition?
Contact BNOSAC: http://www.bnosac.be

