#ifndef KEVCC_UTILS_INCLUDE_ARRAY_KGARRAY_H
#define KEVCC_UTILS_INCLUDE_ARRAY_KGARRAY_H

#include "utils/include/utils/utils.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#define kgarray_pass_ref(KType)     KType*
#define kgarray_pass_val(KType)     KType
#define kgarray_pass_ref_get(val)   *val
#define kgarray_pass_val_get(val)   val

#define kgarray_decl(T, arrname, prefix, pass, kstatic)                         \
typedef struct tag##arrname {                                                   \
  T* begin;                                                                     \
  T* end;                                                                       \
  T* current;                                                                   \
} arrname;                                                                      \
                                                                                \
kstatic bool prefix##_init(arrname* array, size_t size);                        \
kstatic void prefix##_destroy(arrname* array);                                  \
kstatic arrname* prefix##_create(size_t size);                                  \
kstatic void prefix##_delete(arrname* array);                                   \
                                                                                \
static inline bool prefix##_push_back(arrname* array, kgarray_##pass(T) val);   \
static inline void prefix##_pop_back(arrname* array, size_t npop);              \
static inline T* prefix##_back(arrname* array);                                 \
static inline T* prefix##_front(arrname* array);                                \
kstatic bool prefix##_expand(arrname* array);                                   \
static inline T* prefix##_access(arrname* array, size_t index);                 \
static inline size_t prefix##_size(arrname* array);                             \
static inline size_t prefix##_capacity(arrname* array);                         \
/* shrink the array to exactly fit its size */                                  \
static inline void prefix##_shrink(arrname* array);                             \
static inline T* prefix##_steal(arrname* array);                                \
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
  size_t shrinksize = size == 0 ? 1 : size;                                     \
  T* newarr = (T*)realloc(array->begin, shrinksize * sizeof (T));               \
  if (k_likely(newarr)) {                                                       \
    array->begin = newarr;                                                      \
    array->current = array->begin + size;                                       \
    array->end = array->begin + shrinksize;                                     \
  }                                                                             \
}                                                                               \
                                                                                \
static inline bool prefix##_push_back(arrname* array, kgarray_##pass(T) val) {  \
  if (k_unlikely(array->current == array->end &&                                \
      !prefix##_expand(array))) {                                               \
    return false;                                                               \
  }                                                                             \
  *array->current++ = kgarray_##pass##_get(val);                                \
  return true;                                                                  \
}                                                                               \
                                                                                \
static inline void prefix##_pop_back(arrname* array, size_t npop) {             \
  array->current -= npop;                                                       \
}                                                                               \
                                                                                \
static inline T* prefix##_back(arrname* array) {                                \
  return array->current - 1;                                                    \
}                                                                               \
                                                                                \
static inline T* prefix##_front(arrname* array) {                               \
    return array->begin;                                                        \
}                                                                               \
                                                                                \
static inline T* prefix##_access(arrname* array, size_t index) {                \
  return &array->begin[index];                                                  \
}                                                                               \
                                                                                \
static inline T* prefix##_steal(arrname* array) {                               \
  T* ret = array->begin;                                                        \
  array->begin = NULL;                                                          \
  array->current = NULL;                                                        \
  array->end = NULL;                                                            \
  return ret;                                                                   \
}



#define kgarray_impl(T, arrname, prefix, pass, kstatic)                         \
kstatic bool prefix##_init(arrname* array, size_t size) {                       \
  if (k_unlikely(!array)) return false;                                         \
  if (k_unlikely(!(array->begin = (T*)malloc(sizeof (T) * size)))) {            \
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
  T* new_array = (T*)realloc(array->begin, sizeof (T) * new_size);              \
  if (k_unlikely(!new_array)) return false;                                     \
  array->current = new_array + (array->current - array->begin);                 \
  array->begin = new_array;                                                     \
  array->end = new_array + new_size;                                            \
  return true;                                                                  \
}                                                                               

#endif
