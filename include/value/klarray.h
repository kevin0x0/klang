#ifndef _KLANG_INCLUDE_VALUE_KLARRAY_H_
#define _KLANG_INCLUDE_VALUE_KLARRAY_H_

#include "include/misc/klutils.h"
#include "include/mm/klmm.h"
#include "include/value/klclass.h"
#include "include/value/klvalue.h"

#include <stddef.h>

typedef struct tagKlArray {
  KL_DERIVE_FROM(KlObject, _objectbase_);
  KlValue* begin;
  size_t size;
  size_t capacity;
  KLOBJECT_TAIL;
} KlArray;

typedef KlValue* KlArrayIter;

KlArray* klarray_create(KlClass* arrayclass, KlMM* klmm, size_t capacity);

bool klarray_grow(KlArray* array, KlMM* klmm, size_t capacity);

static inline size_t klarray_size(const KlArray* array);
static inline size_t klarray_capacity(const KlArray* array);

static inline bool klarray_expand(KlArray* array, KlMM* klmm);

static inline void klarray_fill(KlArray* array, const KlValue* elems, size_t nelem);
static inline bool klarray_push_back(KlArray* array, KlMM* klmm, const KlValue* vals, size_t nval);
static inline void klarray_pop_back(KlArray* array);
static inline void klarray_multipop(KlArray* array, size_t count);

static inline KlValue* klarray_access(KlArray* array, size_t index);
static inline void klarray_index(KlArray* array, KlInt index, KlValue* val);
static inline KlException klarray_indexas(KlArray* array, size_t index, const KlValue* val);
//static inline KlValue* klarray_set_from_top(KlArray* array, size_t index);
static inline KlValue* klarray_raw(KlArray* array);

static inline KlArrayIter klarray_iter_begin(const KlArray* array);
static inline KlArrayIter klarray_iter_end(const KlArray* array);
static inline KlArrayIter klarray_iter_next(KlArrayIter itr);
static inline KlValue* klarray_iter_get(KlArray* array, KlArrayIter itr);


static inline size_t klarray_size(const KlArray* array) {
  return array->size;
}

static inline size_t klarray_capacity(const KlArray* array) {
  return array->capacity;
}


static inline bool klarray_expand(KlArray* array, KlMM* klmm) {
  return klarray_grow(array, klmm, klarray_size(array) * 2);
}

static inline void klarray_fill(KlArray* array, const KlValue* elems, size_t nelem) {
  kl_assert(klarray_capacity(array) >= nelem, "");
  array->size = nelem;
  for (size_t i = 0; i < nelem; ++i)
    klvalue_setvalue(&array->begin[i], &elems[i]);
}

static inline bool klarray_push_back(KlArray* array, KlMM* klmm, const KlValue* vals, size_t nval) {
  if (kl_unlikely(klarray_capacity(array) < klarray_size(array) + nval &&
                  !klarray_grow(array, klmm, klarray_size(array) + nval))) {
    return false;
  }
  KlValue* pval = array->begin + klarray_size(array);
  array->size += nval;
  while (nval--)
    klvalue_setvalue(pval++, vals++);
  return true;
}

static inline void klarray_multipop(KlArray* array, size_t count) {
  array->size -= count;
}

static inline void klarray_pop_back(KlArray* array) {
  klarray_multipop(array, 1);
}

static inline KlValue* klarray_access(KlArray* array, size_t index) {
  return &array->begin[index];
}

static inline void klarray_index(KlArray* array, KlInt index, KlValue* val) {
  if (kl_likely(0 <= index && index < (KlInt)klarray_size(array))) {
    klvalue_setvalue(val, klarray_access(array, index));
  } else {
    klvalue_setnil(val);
  }
}

static inline KlException klarray_indexas(KlArray* array, size_t index, const KlValue* val) {
  if (kl_likely(index < klarray_size(array))) {
    klvalue_setvalue(klarray_access(array, index), val);
    return KL_E_NONE;
  } else {
    return KL_E_RANGE;
  }
}

static inline KlValue* klarray_raw(KlArray* array) {
  return array->begin;
}

static inline KlArrayIter klarray_iter_begin(const KlArray* array) {
  return array->begin;
}

static inline KlArrayIter klarray_iter_end(const KlArray* array) {
  return array->begin + array->size;
}

static inline KlArrayIter klarray_iter_next(const KlArrayIter itr) {
  return itr + 1;
}

static inline KlValue* klarray_iter_get(KlArray* array, KlArrayIter itr) {
  kl_unused(array);
  return itr;
}


#endif
