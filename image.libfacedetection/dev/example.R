library(image.libfacedetection)
library(magick)
x <- system.file(package = "image.libfacedetection", "images", "keliamoniz1.jpg")
x <- system.file(package = "image.libfacedetection", "images", "keliamoniz2.jpg")
x <- system.file(package = "image.libfacedetection", "images", "handshake.jpg")
x <- "C:/Users/Jan/Desktop/faces.jpg"
x <- "C:/Users/Jan/Desktop/Vanessa.png"
x <- "C:/Users/Jan/Desktop/dt.jpeg"
x <- "C:/Users/Jan/Desktop/bartholomeus_van_der_helst.jpg"
x <- "C:/Users/Jan/Desktop/130829-F-FR885-018.JPG"
x <- "C:/Users/Jan/Desktop/180719-F-JA727-0002.JPG"
x <- "C:/Users/Jan/Desktop/people.jpg"
x <- "C:/Users/Jan/Desktop/handjeschudden.jpg"
img <- image_read(x)
#img <- image_resize(img, "800x")
w <- image_info(img)$width
h <- image_info(img)$height
x <- image_data(img)
x <- as.integer(x)
#x <- aperm(x, c(2, 1, 3))
#x <- aperm(x, c(2, 3, 1))
x <- aperm(x, c(3, 2, 1))
x <- as.vector(x)
print(str(x))
#faces <- image.libfacedetection:::detect_faces(x, 640, 480, 1*640*3)
#faces <- image.libfacedetection:::detect_faces(x, w, h, 1*w*3)
faces <- image.libfacedetection:::detect_faces(x, w, h, 1*w*3)
idx <- 
print(str(faces))


image_draw(img)
face <- lapply(seq_along(faces$x), FUN = function(i){
  face <- lapply(faces, FUN=function(x) x[i])
  #rect(xleft = face$x, xright = face$x + face$width,
  #     ytop = h-face$y, ybottom = h-face$y-face$height, border = "red")
  rect(xleft = face$x, xright = face$x + face$width,
       ytop = face$y, ybottom = face$y+face$height, border = "red", lwd = 5)
  text(x = face$x, y = face$y, adj = 0.5, labels = face$neighbours, col = "white", cex = 2)
  
})
