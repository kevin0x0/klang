#include "utils/include/array/set_array.h"

#include <stdlib.h>

#define KEV_SETARRAY_DEFAULT_SIZE     (16)

bool kev_setarray_init(KevSetArray* array) {
  if (k_unlikely(!array)) return false;
  if (k_unlikely(!(array->begin = (KBitSet**)malloc(sizeof (KBitSet*) * KEV_SETARRAY_DEFAULT_SIZE)))) {
    array->end = NULL;
    array->current = NULL;
    return false;
  }

  array->end = array->begin + KEV_SETARRAY_DEFAULT_SIZE;
  array->current = array->begin;
  return true;
}

void kev_setarray_destroy(KevSetArray* array) {
  if (k_unlikely(!array)) return;
  free(array->begin);
  array->begin = NULL;
  array->end = NULL;
  array->current = NULL;
}

bool kev_setarray_expand(KevSetArray* array) {
  size_t new_size = kev_setarray_size(array) * 2;
  KBitSet** new_array = (KBitSet**)realloc(array->begin, sizeof (KBitSet*) * new_size);
  if (k_unlikely(!new_array)) return false;
  array->current = new_array + (array->current - array->begin);
  array->begin = new_array;
  array->end = new_array + new_size;
  return true;
}


