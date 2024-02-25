#ifndef KEVCC_UTILS_INCLUDE_ARRAY_SET_ARRAY_H
#define KEVCC_UTILS_INCLUDE_ARRAY_SET_ARRAY_H

#include "utils/include/set/bitset.h"
#include "utils/include/general/global_def.h"

typedef struct tagKevSetArray {
  KBitSet** begin;
  KBitSet** end;
  KBitSet** current;
} KevSetArray;

bool kev_setarray_init(KevSetArray* array);
void kev_setarray_destroy(KevSetArray* array);

static inline bool kev_setarray_push_back(KevSetArray* array, KBitSet* set);
static inline void kev_setarray_pop_back(KevSetArray* array);
bool kev_setarray_expand(KevSetArray* array);
static inline KBitSet* kev_setarray_visit(KevSetArray* array, size_t index);
static inline size_t kev_setarray_size(KevSetArray* array);

static inline size_t kev_setarray_size(KevSetArray* array) {
  return array->current - array->begin;
}

static inline bool kev_setarray_push_back(KevSetArray* array, KBitSet* set) {
  if (k_unlikely(array->current == array->end &&
      !kev_setarray_expand(array))) {
    return false;
  }
  *array->current++ = set;
  return true;
}

static inline void kev_setarray_pop_back(KevSetArray* array) {
  array->current--;
}

static inline KBitSet* kev_setarray_visit(KevSetArray* array, size_t index) {
  return array->begin[index];
}

#endif
