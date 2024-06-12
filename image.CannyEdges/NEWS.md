### CHANGES IN image.CannyEdges version 0.1.1

- Use Rf_error("%s", fmt) instead of Rf_error(fmt) in tools.c:41:12 to avoid warning: format string is not a string literal
- Wrap part of the examples of image_canny_edge_detector in donttest to avoid timing issues with magick on cran debian

### CHANGES IN image.CannyEdges version 0.1

- Initial package
