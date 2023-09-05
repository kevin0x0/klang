#include "kevfa/include/array/node_array.h"

#include <stdlib.h>

#define KEV_SETARRAY_DEFAULT_SIZE     (16)

bool kev_nodearray_init(KevNodeArray* array) {
  if (!array) return false;
  if (!(array->begin = (KevGraphNode**)malloc(sizeof (KevGraphNode*) * KEV_SETARRAY_DEFAULT_SIZE))) {
    array->end = NULL;
    array->current = NULL;
    return false;
  }

  array->end = array->begin + KEV_SETARRAY_DEFAULT_SIZE;
  array->current = array->begin;
  return true;
}

void kev_nodearray_destroy(KevNodeArray* array) {
  if (array) {
    free(array->begin);
    array->begin = NULL;
    array->end = NULL;
    array->current = NULL;
  }
}

bool kev_nodearray_push_back(KevNodeArray* array,KevGraphNode* node) {
  if (array->current == array->end &&
      !kev_nodearray_expand(array)) {
    return false;
  }
  *array->current++ = node;
  return true;
}

inline bool kev_nodearray_expand(KevNodeArray* array) {
  size_t new_size = kev_nodearray_size(array) * 2;
  KevGraphNode** new_array = (KevGraphNode**)realloc(array->begin, sizeof (KevGraphNode*) * new_size);
  if (!new_array) return false;
  array->current = new_array + (array->current - array->begin);
  array->begin = new_array;
  array->end = new_array + new_size;
  return true;
}


