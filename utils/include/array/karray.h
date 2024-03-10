#ifndef KEVCC_UTILS_INCLUDE_ARRAY_KARRAY_H
#define KEVCC_UTILS_INCLUDE_ARRAY_KARRAY_H

#include "utils/include/utils/utils.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct tagKArray {
  void** begin;
  void** end;
  void** current;
} KArray;

bool karray_init(KArray* array);
void karray_destroy(KArray* array);
KArray* karray_create(void);
void karray_delete(KArray* array);

static inline void* karray_swap(KArray* array, size_t index, void* value);
static inline bool karray_push_back(KArray* array, void* addr);
static inline void karray_pop_back(KArray* array);
static inline void karray_make_empty(KArray* array);
bool karray_expand(KArray* array);
static inline void* karray_access(KArray* array, size_t index);
static inline void* karray_top(KArray* array);
static inline void* karray_front(KArray* array);
static inline size_t karray_size(KArray* array);
static inline size_t karray_capacity(KArray* array);
/* shrink the array to exactly fit its size */
static inline void karray_shrink(KArray* array);
static inline void** karray_steal(KArray* array);
static inline void** karray_raw(KArray* array);

static inline size_t karray_size(KArray* array) {
  return array->current - array->begin;
}

static inline size_t karray_capacity(KArray* array) {
  return array->end - array->begin;
}

static inline void karray_shrink(KArray* array) {
  size_t size = karray_size(array);
  size_t shrinksize = size == 0 ? 1 : size;
  void** newarr = (void**)realloc(array->begin, shrinksize * sizeof (void*));
  if (k_likely(newarr)) {
    array->begin = newarr;
    array->current = array->begin + size;
    array->end = array->begin + shrinksize;
  }
}

static inline bool karray_push_back(KArray* array, void* addr) {
  if (k_unlikely(array->current == array->end &&
      !karray_expand(array))) {
    return false;
  }
  *array->current++ = addr;
  return true;
}

static inline void karray_pop_back(KArray* array) {
  array->current--;
}

static inline void* karray_access(KArray* array, size_t index) {
  return array->begin[index];
}

static inline void* karray_top(KArray* array) {
  return *(array->current - 1);
}

static inline void* karray_front(KArray* array) {
  return array->begin[0];
}

static inline void karray_make_empty(KArray* array) {
  array->current = array->begin;
}

static inline void** karray_steal(KArray* array) {
  void** ret = array->begin;
  array->begin = NULL;
  array->current = NULL;
  array->end = NULL;
  return ret;
}

static inline void** karray_raw(KArray* array) {
  return array->begin;
}

static inline void* karray_swap(KArray* array, size_t index, void* value) {
  void* ret = array->begin[index];
  array->begin[index] = value;
  return ret;
}

#endif
