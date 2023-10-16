#include "utils/include/array/karray.h"

#include <stdlib.h>

#define KEV_ARRAY_DEFAULT_SIZE      (16)

bool karray_init(KArray* array) {
  if (!array) return false;
  if (!(array->begin = (void**)malloc(sizeof (void*) * KEV_ARRAY_DEFAULT_SIZE))) {
    array->end = NULL;
    array->current = NULL;
    return false;
  }

  array->end = array->begin + KEV_ARRAY_DEFAULT_SIZE;
  array->current = array->begin;
  return true;
}

void karray_destroy(KArray* array) {
  if (array) {
    free(array->begin);
    array->begin = NULL;
    array->end = NULL;
    array->current = NULL;
  }
}

KArray* karray_create(void) {
  KArray* array = (KArray*)malloc(sizeof (KArray));
  if (!array || !karray_init(array)) {
    karray_delete(array);
    return NULL;
  }
  return array;
}

void karray_delete(KArray* array) {
  karray_destroy(array);
  free(array);
}

bool karray_expand(KArray* array) {
  size_t new_size = karray_size(array) * 2;
  void** new_array = (void**)realloc(array->begin, sizeof (void*) * new_size);
  if (!new_array) return false;
  array->current = new_array + (array->current - array->begin);
  array->begin = new_array;
  array->end = new_array + new_size;
  return true;
}


