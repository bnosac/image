# ABOUT

* Author    : Antoni Buades <toni.buades@gmail.com>
* Copyright : (C) 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
* Licence

- nlmeans_ipol.cpp, libdenoising.cpp and libdenoising.h
may be linked to the EP patent 1,749,278 by A. Buades, T. Coll and J.M. Morel.
They are provided for scientific and education only.

- All the other files are distributed under the terms of the
  LGPLv3 license.


## Examples

```r
library(magick)
library(image.DenoiseNLMeans)
img <- system.file(package = "image.DenoiseNLMeans", "extdata", "img_garden.png")
image_read(img)
x <- image_denoise_nlmeans(img, sigma = 10)
image_read(x$file_denoised)
```