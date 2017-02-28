#include "network.h"
#include "utils.h"
#include "parser.h"
#include "option_list.h"
#include "blas.h"
#include "assert.h"
#include "classifier.h"
#include "cuda.h"
#include <sys/time.h>
#include <string.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#ifdef OPENCV
#include "opencv2/highgui/highgui_c.h"
image get_image_from_stream(CvCapture *cap);
#endif


void darknet_predict_classifier(char *datacfg, char *cfgfile, char *weightfile, char *filename, int top,
                                char **pred_lab, double *pred_score, char **names, int resize){
  
  network net = parse_network_cfg(cfgfile);
  if(weightfile){
    load_weights(&net, weightfile);
  }
  set_batch_network(&net, 1);
  srand(2222222);
  /*
  list *options = read_data_cfg(datacfg);
  
  char *name_list = option_find_str(options, "names", 0);
  if(!name_list) name_list = option_find_str(options, "labels", "data/labels.list");
  if(top == 0) top = option_find_int(options, "top", 1);
  */
  int i = 0;
  //char **names = get_labels(name_list);
  clock_t time;
  int *indexes = calloc(top, sizeof(int));
  char buff[256];
  char *input = buff;
  int size = net.w;
  while(1){
    strncpy(input, filename, 256);
    image im = load_image_color(input, 0, 0);
    image r = resize_min(im, size);
    if(resize > 0) {
      resize_network(&net, r.w, r.h);
    }
    //printf("%d %d\n", r.w, r.h);
    
    float *X = r.data;
    time=clock();
    float *predictions = network_predict(net, X);
    if(net.hierarchy) hierarchy_predictions(predictions, net.outputs, net.hierarchy, 0);
    top_k(predictions, net.outputs, top, indexes);
    //printf("%s: Predicted in %f seconds.\n", input, sec(clock()-time));
    for(i = 0; i < top; ++i){
      int index = indexes[i];
      //if(net.hierarchy) printf("%d, %s: %f, parent: %s \n",index, names[index], predictions[index], (net.hierarchy->parent[index] >= 0) ? names[net.hierarchy->parent[index]] : "Root");
      //else printf("%s: %f\n",names[index], predictions[index]);
      pred_lab[i] = names[index];
      pred_score[i] = predictions[index];
    }
    if(r.data != im.data) free_image(r);
    free_image(im);
    if (filename) break;
  }
}


SEXP darknet_predict(SEXP datasetup, SEXP modelsetup, SEXP modelweights, SEXP image, SEXP first, SEXP labels, SEXP resize){
  const char *datacfg = CHAR(STRING_ELT(datasetup, 0));
  const char *cfgfile = CHAR(STRING_ELT(modelsetup, 0));
  const char *weightfile = CHAR(STRING_ELT(modelweights, 0));
  const char *filename = CHAR(STRING_ELT(image, 0));
  int top = INTEGER(first)[0];
  int resizing = INTEGER(resize)[0];
  char* pred_lab[top];
  double pred_score[top];
  
  PROTECT(labels = AS_CHARACTER(labels));
  int labels_size = LENGTH(labels);
  char* output_labels[labels_size];
  for(int i=0; i<labels_size; i++) {
    output_labels[i] = (char *)CHAR(STRING_ELT(labels, i));
  }

  darknet_predict_classifier((char *)datacfg, (char *)cfgfile, (char *)weightfile, (char *)filename, 
                             top, pred_lab, pred_score, output_labels, resizing);
  
  SEXP pred_labels = PROTECT(allocVector(STRSXP, top));
  SEXP pred_scores = PROTECT(allocVector(REALSXP, top));
  SEXP result = PROTECT(allocVector(VECSXP, 2));
  SET_VECTOR_ELT(result, 0, pred_scores);
  SET_VECTOR_ELT(result, 1, pred_labels);
  for(int i = 0; i < top; ++i){
    SET_STRING_ELT(pred_labels, i, mkChar(pred_lab[i]));
    REAL(pred_scores)[i] = pred_score[i];
  }
  UNPROTECT(4);
  return(result);
}