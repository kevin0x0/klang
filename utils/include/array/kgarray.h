#ifndef KEVCC_UTILS_INCLUDE_ARRAY_KGARRAY_H
#define KEVCC_UTILS_INCLUDE_ARRAY_KGARRAY_H

#include "utils/include/utils/utils.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#define kgarray_decl(KType, arrname, prefix, kstatic)                           \
typedef struct tag##arrname {                                                   \
  KType* begin;                                                                 \
  KType* end;                                                                   \
  KType* current;                                                               \
} arrname;                                                                      \
                                                                                \
kstatic bool prefix##_init(arrname* array, size_t size);                        \
kstatic void prefix##_destroy(arrname* array);                                  \
kstatic arrname* prefix##_create(size_t size);                                  \
kstatic void prefix##_delete(arrname* array);                                   \
                                                                                \
static inline bool prefix##_push_back(arrname* array, KType* addr);             \
kstatic bool prefix##_expand(arrname* array);                                   \
static inline KType* prefix##_access(arrname* array, size_t index);             \
static inline size_t prefix##_size(arrname* array);                             \
static inline size_t prefix##_capacity(arrname* array);                         \
/* shrink the array to exactly fit its size */                                  \
static inline void prefix##_shrink(arrname* array);                             \
static inline KType* prefix##_steal(arrname* array);                            \
                                                                                \
static inline size_t prefix##_size(arrname* array) {                            \
  return array->current - array->begin;                                         \
}                                                                               \
                                                                                \
static inline size_t prefix##_capacity(arrname* array) {                        \
  return array->end - array->begin;                                             \
}                                                                               \
                                                                                \
static inline void prefix##_shrink(arrname* array) {                            \
  size_t size = prefix##_size(array);                                           \
  KType* newarr = (KType*)realloc(array->begin, size * sizeof (KType));         \
  if (k_likely(newarr)) {                                                       \
    array->begin = newarr;                                                      \
    array->current = array->begin + size;                                       \
    array->end = array->current;                                                \
  }                                                                             \
}                                                                               \
                                                                                \
static inline bool prefix##_push_back(arrname* array, KType* addr) {            \
  if (k_unlikely(array->current == array->end &&                                \
      !prefix##_expand(array))) {                                               \
    return false;                                                               \
  }                                                                             \
  *array->current++ = *addr;                                                    \
  return true;                                                                  \
}                                                                               \
                                                                                \
static inline KType* prefix##_access(arrname* array, size_t index) {            \
  return &array->begin[index];                                                  \
}                                                                               \
                                                                                \
static inline KType* prefix##_steal(arrname* array) {                           \
  KType* ret = array->begin;                                                    \
  array->begin = NULL;                                                          \
  array->current = NULL;                                                        \
  array->end = NULL;                                                            \
  return ret;                                                                   \
}



#define kgarray_impl(KType, arrname, prefix, kstatic)                           \
kstatic bool prefix##_init(arrname* array, size_t size) {                       \
  if (k_unlikely(!array)) return false;                                         \
  if (k_unlikely(!(array->begin = (KType*)malloc(sizeof (KType) * size)))) {    \
    array->end = NULL;                                                          \
    array->current = NULL;                                                      \
    return false;                                                               \
  }                                                                             \
                                                                                \
  array->end = array->begin + size;                                             \
  array->current = array->begin;                                                \
  return true;                                                                  \
}                                                                               \
                                                                                \
kstatic void prefix##_destroy(arrname* array) {                                 \
  if (k_unlikely(!array)) return;                                               \
  free(array->begin);                                                           \
  array->begin = NULL;                                                          \
  array->end = NULL;                                                            \
  array->current = NULL;                                                        \
}                                                                               \
                                                                                \
kstatic arrname* prefix##_create(size_t size) {                                 \
  arrname* array = (arrname*)malloc(sizeof (arrname));                          \
  if (k_unlikely(!array || !prefix##_init(array, size))) {                      \
    prefix##_delete(array);                                                     \
    return NULL;                                                                \
  }                                                                             \
  return array;                                                                 \
}                                                                               \
                                                                                \
kstatic void prefix##_delete(arrname* array) {                                  \
  prefix##_destroy(array);                                                      \
  free(array);                                                                  \
}                                                                               \
                                                                                \
kstatic bool prefix##_expand(arrname* array) {                                  \
  size_t new_size = prefix##_size(array) * 2;                                   \
  KType* new_array = (KType*)realloc(array->begin, sizeof (KType) * new_size);  \
  if (k_unlikely(!new_array)) return false;                                     \
  array->current = new_array + (array->current - array->begin);                 \
  array->begin = new_array;                                                     \
  array->end = new_array + new_size;                                            \
  return true;                                                                  \
}                                                                               

#endif
