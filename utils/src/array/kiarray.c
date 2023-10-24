#include "utils/include/array/kiarray.h"

#include <stdlib.h>

#define KARRAY_SIZE       (4)

bool kiarray_init(KIArray* array) {
  if (!array) return false;
  if (!(array->begin = (void**)malloc(sizeof (void*) * KARRAY_SIZE))) {
    array->end = NULL;
    array->current = NULL;
    return false;
  }

  array->end = array->begin + KARRAY_SIZE;
  array->current = array->begin;
  return true;
}

void kiarray_destroy(KIArray* array) {
  if (array) {
    free(array->begin);
    array->begin = NULL;
    array->end = NULL;
    array->current = NULL;
  }
}

KIArray* kiarray_create(void) {
  KIArray* array = (KIArray*)malloc(sizeof (KIArray));
  if (!array || !kiarray_init(array)) {
    kiarray_delete(array);
    return NULL;
  }
  return array;
}

void kiarray_delete(KIArray* array) {
  kiarray_destroy(array);
  free(array);
}

bool kiarray_expand(KIArray* array) {
  size_t new_size = kiarray_size(array) * 2;
  void** new_array = (void**)realloc(array->begin, sizeof (void*) * new_size);
  if (!new_array) return false;
  array->current = new_array + (array->current - array->begin);
  array->begin = new_array;
  array->end = new_array + new_size;
  return true;
}


