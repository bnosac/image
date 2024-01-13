#include "network.h"
#include "region_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "demo.h"
#include "option_list.h"

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>


image **load_alphabet_pkg(char *path)
{
  int i, j;
  const int nsize = 8;
  image **alphabets = calloc(nsize, sizeof(image));
  for(j = 0; j < nsize; ++j){
    alphabets[j] = calloc(128, sizeof(image));
    for(i = 32; i < 127; ++i){
      char buff[256];
      sprintf(buff, "%s/data/labels/%d_%d.png", path, i, j);
      alphabets[j][i] = load_image_color(buff, 0, 0);
    }
  }
  return alphabets;
}


int darknet_test_detector(char *cfgfile, char *weightfile, char *filename, float thresh, float hier_thresh, char **names, char *path)
{
  image **alphabet = load_alphabet_pkg(path);
  network *net = load_network(cfgfile, weightfile, 0);
  
  set_batch_network(net, 1);
  srand(2222222);
  clock_t time;
  char buff[256];
  char *input = buff;
  int j;
  float nms=.4;
  int boxes_abovethreshold = 0;
  while(1){
    strncpy(input, filename, 256);
    image im = load_image_color(input,0,0);
    
    image sized = letterbox_image(im, net->w, net->h);
    layer l = net->layers[net->n-1];
    
    box *boxes = calloc(l.w*l.h*l.n, sizeof(box));
    float **probs = calloc(l.w*l.h*l.n, sizeof(float *));
    for(j = 0; j < l.w*l.h*l.n; ++j) probs[j] = calloc(l.classes + 1, sizeof(float *));
    
    float *X = sized.data;
    time=clock();
    network_predict(net, X);
    printf("%s: Predicted in %f seconds.\n", input, sec(clock()-time));
    
    int nboxes = 0;
    detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
    //printf("%d\n", nboxes);
    //if (nms) do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
    if (nms) do_nms_sort(dets, nboxes, l.classes, nms);
    draw_detections(im, dets, nboxes, thresh, names, alphabet, l.classes);
    free_detections(dets, nboxes);
    
    save_image(im, "predictions");
    
    free_image(im);
    free_image(sized);
    free(boxes);
    free_ptrs((void **)probs, l.w*l.h*l.n);
    if (filename) break;
  }
  return(boxes_abovethreshold);
}

SEXP darknet_detect(SEXP modelsetup, SEXP modelweights, SEXP image, SEXP th, SEXP hier_th, SEXP labels, SEXP darknet_root){
  const char *cfgfile = CHAR(STRING_ELT(modelsetup, 0));
  const char *weightfile = CHAR(STRING_ELT(modelweights, 0));
  const char *filename = CHAR(STRING_ELT(image, 0));
  float thresh = REAL(th)[0];
  float hier_thresh = REAL(hier_th)[0];
  const char *path = CHAR(STRING_ELT(darknet_root, 0));

  PROTECT(labels = AS_CHARACTER(labels));
  int labels_size = LENGTH(labels);
  char* output_labels[labels_size];
  for(int i=0; i<labels_size; i++) {
    output_labels[i] = (char *)CHAR(STRING_ELT(labels, i));
  }
  
  int objects_found = darknet_test_detector( 
                        (char *)cfgfile, 
                        (char *)weightfile, 
                        (char *)filename, 
                        thresh, hier_thresh,
                        output_labels,
                        (char *)path);
  UNPROTECT(1);
  return(modelsetup);
}