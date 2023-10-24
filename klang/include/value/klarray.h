#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLARRAY_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLARRAY_H
#include "klang/include/value/typedecl.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct tagKlArray {
  KlValue** begin;
  KlValue** end;
  KlValue** current;
} KlArray;

typedef KlValue** KlArrayIter;

bool klarray_init(KlArray* array);
bool klarray_init_copy(KlArray* array, KlArray* src);
void klarray_destroy(KlArray* array);
KlArray* klarray_create(void);
KlArray* klarray_create_copy(KlArray* src);
void klarray_delete(KlArray* array);

static inline KlValue* klarray_swap(KlArray* array, size_t index, KlValue* value);
static inline bool klarray_push_back(KlArray* array, KlValue* value);
static inline void klarray_pop_back(KlArray* array);
static inline void klarray_make_empty(KlArray* array);
bool klarray_expand(KlArray* array);
static inline KlValue* klarray_access(KlArray* array, size_t index);
static inline KlValue* klarray_top(KlArray* array);
static inline size_t klarray_size(KlArray* array);
static inline size_t klarray_capacity(KlArray* array);
static inline KlValue** klarray_steal(KlArray* array);
static inline KlValue** klarray_raw(KlArray* array);

static inline KlArrayIter klarray_iter_begin(KlArray* array);
static inline KlArrayIter klarray_iter_end(KlArray* array);
static inline KlArrayIter klarray_iter_next(KlArrayIter itr);

static inline size_t klarray_size(KlArray* array) {
  return array->current - array->begin;
}

static inline size_t klarray_capacity(KlArray* array) {
  return array->end - array->begin;
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

static inline KlArrayIter klarray_iter_begin(KlArray* array) {
  return array->begin;
}

static inline KlArrayIter klarray_iter_end(KlArray* array) {
  return array->current;
}

static inline KlArrayIter klarray_iter_next(KlArrayIter itr) {
  return itr + 1;
}


#endif
