#include "tokenizer_generator/include/finite_automaton/array/set_array.h"
#include "tokenizer_generator/include/finite_automaton/bitset/bitset.h"

#include <stdlib.h>

#define KEV_SETARRAY_DEFAULT_SIZE     (16)

bool kev_setarray_init(KevSetArray* array) {
  if (!array) return false;
  if (!(array->begin = (KevBitSet**)malloc(sizeof (KevBitSet*) * KEV_SETARRAY_DEFAULT_SIZE))) {
    array->end = NULL;
    array->current = NULL;
    return false;
  }

  array->end = array->begin + KEV_SETARRAY_DEFAULT_SIZE;
  array->current = array->begin;
  return true;
}

void kev_setarray_destroy(KevSetArray* array) {
  if (array) {
    free(array->begin);
    array->begin = NULL;
    array->end = NULL;
    array->current = NULL;
  }
}

bool kev_setarray_push_back(KevSetArray* array, KevBitSet* set) {
  if (array->current == array->end &&
      !kev_setarray_expand(array)) {
    return false;
  }
  *array->current++ = set;
  return true;
}

inline bool kev_setarray_expand(KevSetArray* array) {
  uint64_t new_size = kev_setarray_size(array) * 2;
  KevBitSet** new_array = (KevBitSet**)realloc(array->begin, sizeof (KevBitSet*) * new_size);
  if (!new_array) return false;
  array->current = new_array + (array->current - array->begin);
  array->begin = new_array;
  array->end = new_array + new_size;
  return true;
}


