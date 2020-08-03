# image -  Computer Vision and Image Recognition algorithms for R users 

This repository contains a suite of R packages which perform image algorithms currently not available in other R packages like [magick](https://CRAN.R-project.org/package=magick), [imager](https://CRAN.R-project.org/package=imager) or [EBImage](https://bioconductor.org/packages/release/bioc/html/EBImage.html). 

These algorithms are put into different packages because of license differences. Currently the following R packages are available:

| Package           | Functionality                          | License     | Details|
|-------------------|----------------------------------------|-------------|--------|
| **image.CornerDetectionF9**    | FAST-9 corner detection for images     | BSD-2   | [Details](image.CornerDetectionF9)       |
| **image.CornerDetectionHarris**| Harris corner detection for images     | BSD-2   | [Details](image.CornerDetectionHarris)   |
| **image.LineSegmentDetector**  | Line Segment Detector (LSD) for images | AGPL-3  | [Details](image.LineSegmentDetector)     |
| **image.ContourDetector**      | Unsupervised Smooth Contour Line Detection for images | AGPL-3  | [Details](image.ContourDetector)     |
| **image.CannyEdges**           | Canny Edge Detector for Images         | GPL-3   | [Details](image.CannyEdges)              |
| **image.Otsu**                 | Otsu's Image Segmentation Method       | MIT     | [Details](image.Otsu)                    |
| **image.dlib**                 | Speeded up robust features (SURF) and histogram of oriented gradients (HOG) features | AGPL-3 | [Details](image.dlib)     |
| **image.libfacedetection**     | CNN for Face Detection                 | BSD-3   | [Details](image.libfacedetection)        |
| **image.darknet**              | Image classification using darknet with deep learning models AlexNet, Darknet, VGG-16, Extraction (GoogleNet) and Darknet19. As well object detection using the state-of-the art YOLO detection system | MIT   | [Details](image.darknet)        |
| **image.OpenPano**             | Image Stitching                        | see file LICENSE | [Details](image.OpenPano)       |
| **image.DenoiseNLMeans**       | Non-local means denoising              | see file LICENSE | [Details](image.DenoiseNLMeans) |

More packages and extensions are under development.

A presentation given at the useR-2017 conference is available in file [presentation-user2017.pdf](presentation-user2017.pdf) 

![](logo-image.png)

## Installation

- Some packages are on CRAN

```
install.packages("image.CannyEdges")
install.packages("image.ContourDetector")
install.packages("image.CornerDetectionF9")
install.packages("image.CornerDetectionHarris")
install.packages("image.dlib")
install.packages("image.libfacedetection")
install.packages("image.LineSegmentDetector")
install.packages("image.Otsu")
install.packages("image.binarization")
```

- You can see if the binary packages for your operating system are on the BNOSAC drat repo at https://github.com/bnosac/drat
- If they are, you can just install them as follows, where you replace `thepackagename` with one of the packages you are interested in 

```
install.packages("thepackagename", repos = "https://bnosac.github.io/drat")
```

### Development packages

Install the development version of packages as follows:

```
install.packages("remotes")
remotes::install_github("bnosac/image", subdir = "image.CornerDetectionF9")
remotes::install_github("bnosac/image", subdir = "image.CornerDetectionHarris")
remotes::install_github("bnosac/image", subdir = "image.LineSegmentDetector")
remotes::install_github("bnosac/image", subdir = "image.ContourDetector")
remotes::install_github("bnosac/image", subdir = "image.CannyEdges")
remotes::install_github("bnosac/image", subdir = "image.Otsu")
remotes::install_github("bnosac/image", subdir = "image.dlib")
remotes::install_github("bnosac/image", subdir = "image.darknet")
remotes::install_github("bnosac/image", subdir = "image.DenoiseNLMeans")
remotes::install_github("bnosac/image", subdir = "image.libfacedetection")
remotes::install_github("bnosac/image", subdir = "image.OpenPano")
```

### CI builds

- CI builds are here 
    - [Travis](https://travis-ci.org/bnosac/image)
    - [AppVeyor](https://ci.appveyor.com/project/jwijffels/image) 


## Support in image recognition

Need support in image recognition?
Contact BNOSAC: http://www.bnosac.be

