#include "utils/include/array/addr_array.h"

#include <stdlib.h>

#define KEV_ARRAY_DEFAULT_SIZE      (16)

bool kev_addrarray_init(KevAddrArray* array) {
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

void kev_addrarray_destroy(KevAddrArray* array) {
  if (array) {
    free(array->begin);
    array->begin = NULL;
    array->end = NULL;
    array->current = NULL;
  }
}

KevAddrArray* kev_addrarray_create(void) {
  KevAddrArray* array = (KevAddrArray*)malloc(sizeof (KevAddrArray));
  if (!array || !kev_addrarray_init(array)) {
    kev_addrarray_delete(array);
    return NULL;
  }
  return array;
}

void kev_addrarray_delete(KevAddrArray* array) {
  kev_addrarray_destroy(array);
  free(array);
}

bool kev_addrarray_expand(KevAddrArray* array) {
  size_t new_size = kev_addrarray_size(array) * 2;
  void** new_array = (void**)realloc(array->begin, sizeof (void*) * new_size);
  if (!new_array) return false;
  array->current = new_array + (array->current - array->begin);
  array->begin = new_array;
  array->end = new_array + new_size;
  return true;
}


