#ifndef KEVCC_UTILS_INCLUDE_ARRAY_KIARRAY_H
#define KEVCC_UTILS_INCLUDE_ARRAY_KIARRAY_H

#include <stddef.h>
#include <stdbool.h>

typedef struct tagKIArray {
  void** begin;
  void** end;
  void** current;
} KIArray;

bool kiarray_init(KIArray* array);
void kiarray_destroy(KIArray* array);
KIArray* kiarray_create(void);
void kiarray_delete(KIArray* array);

static inline void* kiarray_swap(KIArray* array, size_t index, void* value);
static inline bool kiarray_push_back(KIArray* array, void* addr);
static inline void kiarray_pop_back(KIArray* array);
static inline void kiarray_make_empty(KIArray* array);
bool kiarray_expand(KIArray* array);
static inline void* kiarray_access(KIArray* array, size_t index);
static inline void* kiarray_top(KIArray* array);
static inline size_t kiarray_size(KIArray* array);
static inline size_t kiarray_capacity(KIArray* array);
static inline void** kiarray_steal(KIArray* array);
static inline void** kiarray_raw(KIArray* array);

static inline size_t kiarray_size(KIArray* array) {
  return array->current - array->begin;
}

static inline size_t kiarray_capacity(KIArray* array) {
  return array->end - array->begin;
}

static inline bool kiarray_push_back(KIArray* array, void* addr) {
  if (array->current == array->end &&
      !kiarray_expand(array)) {
    return false;
  }
  *array->current++ = addr;
  return true;
}

static inline void kiarray_pop_back(KIArray* array) {
  array->current--;
}

static inline void* kiarray_access(KIArray* array, size_t index) {
  return array->begin[index];
}

static inline void* kiarray_top(KIArray* array) {
  return *(array->current - 1);
}

static inline void kiarray_make_empty(KIArray* array) {
  array->current = array->begin;
}

static inline void** kiarray_steal(KIArray* array) {
  void** ret = array->begin;
  array->begin = NULL;
  array->current = NULL;
  array->end = NULL;
  return ret;
}

static inline void** kiarray_raw(KIArray* array) {
  return array->begin;
}

static inline void* kiarray_swap(KIArray* array, size_t index, void* value) {
  void* ret = array->begin[index];
  array->begin[index] = value;
  return ret;
}

#endif
