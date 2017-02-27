# image -  Computer Vision and Image Recognition algorithms for R users 

This repository contains a suite of R packages which perform image algorithms currently not available in other R packages like ![magick](https://CRAN.R-project.org/package=magick), ![imageR](https://CRAN.R-project.org/package=imageR) or ![EBImage](https://bioconductor.org/packages/release/bioc/html/EBImage.html). 

There are put into different packages because of license differences. Currently the following R packages are available:

- image.CornerDetectionF9:  FAST-9 corner detection for images  (license: BSD-2)
- image.LineSegmentDetector: Line Segment Detector (LSD) for images (license: AGPL-3)
- image.ContourDetector:  Unsupervised Smooth Contour Line Detection for images (license: AGPL-3)
- image.CannyEdges: Canny Edge Detector for Images (license: GPL-3)
- image.dlib: Speeded up robust features (SURF) and histogram of oriented gradients (HOG) features (license: AGPL-3)

More packages and extensions are under development.

## Installation

Install all packages

```
install.packages("devtools")
install.packages("dlib")
devtools::install_github("bnosac/image", subdir = "image.CornerDetectionF9", build_vignettes = TRUE)
devtools::install_github("bnosac/image", subdir = "image.LineSegmentDetector", build_vignettes = TRUE)
devtools::install_github("bnosac/image", subdir = "image.ContourDetector", build_vignettes = TRUE)
devtools::install_github("bnosac/image", subdir = "image.CannyEdges", build_vignettes = TRUE)
devtools::install_github("bnosac/image", subdir = "image.dlib", build_vignettes = TRUE)
```

## Support in image recognition

Need support in image recognition?
Contact BNOSAC: http://www.bnosac.be

