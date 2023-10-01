#ifndef KEVCC_UTILS_INCLUDE_ARRAY_ADDR_ARRAY_H
#define KEVCC_UTILS_INCLUDE_ARRAY_ADDR_ARRAY_H

#include "utils/include/general/global_def.h"

typedef struct tagKevAddrArray {
  void** begin;
  void** end;
  void** current;
} KevAddrArray;

bool kev_addrarray_init(KevAddrArray* array);
void kev_addrarray_destroy(KevAddrArray* array);
KevAddrArray* kev_addrarray_create(void);
void kev_addrarray_delete(KevAddrArray* array);

static inline bool kev_addrarray_push_back(KevAddrArray* array, void* addr);
static inline void kev_addrarray_pop_back(KevAddrArray* array);
static inline void kev_addrarray_make_empty(KevAddrArray* array);
bool kev_addrarray_expand(KevAddrArray* array);
static inline void* kev_addrarray_visit(KevAddrArray* array, size_t index);
static inline size_t kev_addrarray_size(KevAddrArray* array);
static inline void** kev_addrarray_steal(KevAddrArray* array);
static inline void** kev_addrarray_raw_array(KevAddrArray* array);

static inline size_t kev_addrarray_size(KevAddrArray* array) {
  return array->current - array->begin;
}

static inline bool kev_addrarray_push_back(KevAddrArray* array, void* addr) {
  if (array->current == array->end &&
      !kev_addrarray_expand(array)) {
    return false;
  }
  *array->current++ = addr;
  return true;
}

static inline void kev_addrarray_pop_back(KevAddrArray* array) {
  array->current--;
}

static inline void* kev_addrarray_visit(KevAddrArray* array, size_t index) {
  return array->begin[index];
}

static inline void kev_addrarray_make_empty(KevAddrArray* array) {
  array->current = array->begin;
}

static inline void** kev_addrarray_steal(KevAddrArray* array) {
  void** ret = array->begin;
  array->begin = NULL;
  array->current = NULL;
  array->end = NULL;
  return ret;
}

static inline void** kev_addrarray_raw_array(KevAddrArray* array) {
  return array->begin;
}

#endif
