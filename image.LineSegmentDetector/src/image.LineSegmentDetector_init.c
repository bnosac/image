#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

static R_CallMethodDef callMethods[] = {
	{NULL, NULL, 0}
};

void R_init_image_LineSegmentDetector(DllInfo* info) {
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);
	R_useDynamicSymbols(info, TRUE);
}
