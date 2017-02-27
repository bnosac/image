#ifndef _ABSTRACT_DSF_H
#define _ABSTRACT_DSF_H


// implementation of a DSF with path compression but without union by rank

void adsf_assert_consistency(int *t, int n);
void adsf_begin(int *t, int n);
int adsf_union(int *t, int n, int a, int b);
int adsf_find(int *t, int n, int a);

#endif /* _ABSTRACT_DSF_H */
