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
  KlValue* end;
  KlValue* current;
  KLOBJECT_TAIL;
} KlArray;

typedef KlValue* KlArrayIter;

KlArray* klarray_create(KlClass* arrayclass, KlMM* klmm, size_t capacity);

bool klarray_check_capacity(KlArray* array, KlMM* klmm, size_t capacity);

static inline size_t klarray_size(const KlArray* array);
static inline size_t klarray_capacity(const KlArray* array);

static inline bool klarray_expand(KlArray* array, KlMM* klmm);

static inline void klarray_fill(KlArray* array, const KlValue* elems, size_t nelem);
static inline bool klarray_push_back(KlArray* array, KlMM* klmm, const KlValue* vals, size_t nval);
static inline void klarray_pop_back(KlArray* array);
static inline void klarray_multipop(KlArray* array, size_t count);
static inline KlValue* klarray_top(KlArray* array);

static inline void klarray_make_empty(KlArray* array);
static inline KlValue* klarray_access(KlArray* array, size_t index);
static inline void klarray_index(KlArray* array, KlInt index, KlValue* val);
static inline KlException klarray_indexas(KlArray* array, size_t index, const KlValue* val);
static inline KlValue* klarray_access_from_top(KlArray* array, size_t index);
//static inline KlValue* klarray_set_from_top(KlArray* array, size_t index);
static inline KlValue* klarray_raw(KlArray* array);

static inline KlArrayIter klarray_iter_begin(KlArray* array);
static inline KlArrayIter klarray_iter_end(KlArray* array);
static inline KlArrayIter klarray_iter_next(KlArrayIter itr);
static inline KlValue* klarray_iter_get(KlArray* array, KlArrayIter itr);


static inline size_t klarray_size(const KlArray* array) {
  return array->current - array->begin;
}

static inline size_t klarray_capacity(const KlArray* array) {
  return array->end - array->begin;
}


static inline bool klarray_expand(KlArray* array, KlMM* klmm) {
  return klarray_check_capacity(array, klmm, klarray_size(array) * 2);
}

static inline void klarray_fill(KlArray* array, const KlValue* elems, size_t nelem) {
  kl_assert(klarray_capacity(array) >= nelem, "");
  array->current = array->begin + nelem;
  for (size_t i = 0; i < nelem; ++i)
    klvalue_setvalue(&array->begin[i], &elems[i]);
}

static inline bool klarray_push_back(KlArray* array, KlMM* klmm, const KlValue* vals, size_t nval) {
  if (kl_unlikely(!klarray_check_capacity(array, klmm, klarray_size(array) + nval))) {
    return false;
  }
  while (nval--)
    klvalue_setvalue(array->current++, vals++);
  return true;
}

static inline void klarray_multipop(KlArray* array, size_t count) {
  array->current -= count;
}

static inline void klarray_pop_back(KlArray* array) {
  klarray_multipop(array, 1);
}

static inline KlValue* klarray_top(KlArray* array) {
  return array->current - 1;
}


static inline KlValue* klarray_access(KlArray* array, size_t index) {
  return &array->begin[index];
}

static inline void klarray_index(KlArray* array, KlInt index, KlValue* val) {
  if (0 <= index && index < (KlInt)klarray_size(array)) {
    klvalue_setvalue(val, klarray_access(array, index));
    return;
  }
  klvalue_setnil(val);
}

static inline KlException klarray_indexas(KlArray* array, size_t index, const KlValue* val) {
  if (index < klarray_size(array)) {
    klvalue_setvalue(klarray_access(array, index), val);
    return KL_E_NONE;
  }
  return KL_E_RANGE;
}

static inline KlValue* klarray_access_from_top(KlArray* array, size_t index) {
  return &array->current[-index];
}

static inline void klarray_make_empty(KlArray* array) {
  array->current = array->begin;
}

static inline KlValue* klarray_raw(KlArray* array) {
  return array->begin;
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

static inline KlValue* klarray_iter_get(KlArray* array, KlArrayIter itr) {
  kl_unused(array);
  return itr;
}


#endif
