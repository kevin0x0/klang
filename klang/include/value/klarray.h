#ifndef KEVCC_KLANG_INCLUDE_VALUE_KKLARRAY_H
#define KEVCC_KLANG_INCLUDE_VALUE_KKLARRAY_H
#include "klang/include/value/typedecl.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct tagKlArray {
  KlValue** begin;
  KlValue** end;
  KlValue** current;
} KlArray;

bool klarray_init(KlArray* array);
void klarray_destroy(KlArray* array);
KlArray* klarray_create(void);
void klarray_delete(KlArray* array);

static inline KlValue* klarray_swap(KlArray* array, size_t index, KlValue* value);
static inline bool klarray_push_back(KlArray* array, KlValue* value);
static inline void klarray_pop_back(KlArray* array);
static inline void klarray_make_empty(KlArray* array);
bool klarray_expand(KlArray* array);
static inline KlValue* klarray_access(KlArray* array, size_t index);
static inline KlValue* klarray_top(KlArray* array);
static inline size_t klarray_size(KlArray* array);
static inline KlValue** klarray_steal(KlArray* array);
static inline KlValue** klarray_raw(KlArray* array);

static inline size_t klarray_size(KlArray* array) {
  return array->current - array->begin;
}

static inline bool klarray_push_back(KlArray* array, KlValue* value) {
  if (array->current == array->end &&
      !klarray_expand(array)) {
    return false;
  }
  *array->current++ = value;
  return true;
}

static inline void klarray_pop_back(KlArray* array) {
  array->current--;
}

static inline KlValue* klarray_access(KlArray* array, size_t index) {
  return array->begin[index];
}

static inline KlValue* klarray_top(KlArray* array) {
  return *(array->current - 1);
}

static inline void klarray_make_empty(KlArray* array) {
  array->current = array->begin;
}

static inline KlValue** klarray_steal(KlArray* array) {
  KlValue** ret = array->begin;
  array->begin = NULL;
  array->current = NULL;
  array->end = NULL;
  return ret;
}

static inline KlValue** klarray_raw(KlArray* array) {
  return array->begin;
}

static inline KlValue* klarray_swap(KlArray* array, size_t index, KlValue* value) {
  void* ret = array->begin[index];
  array->begin[index] = value;
  return ret;
}

#endif
