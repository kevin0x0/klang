#ifndef _KLANG_INCLUDE_VALUE_KLTUPLE_H_
#define _KLANG_INCLUDE_VALUE_KLTUPLE_H_

#include "include/mm/klmm.h"
#include "include/value/klvalue.h"

#include <stddef.h>

typedef struct tagKlTuple {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  size_t nval;
  KlValue vals[];
} KlTuple;

KlTuple* kltuple_create(KlMM* klmm, const KlValue* vals, size_t nval);

static inline size_t kltuple_nval(const KlTuple* tuple);
static inline size_t kltuple_size(const KlTuple* tuple);

static inline KlValue* kltuple_access(KlTuple* tuple, size_t index);
static inline void kltuple_index(KlTuple* tuple, KlInt index, KlValue* val);
static inline KlException kltuple_indexas(KlTuple* tuple, size_t index, const KlValue* val);
static inline const KlValue* kltuple_iter_begin(const KlTuple* tuple);
static inline const KlValue* kltuple_iter_end(const KlTuple* tuple);

static inline size_t kltuple_nval(const KlTuple* tuple) {
  return tuple->nval;
}

static inline size_t kltuple_size(const KlTuple* tuple) {
  return sizeof (KlTuple) + tuple->nval * sizeof (KlValue);
}

static inline KlValue* kltuple_access(KlTuple* tuple, size_t index) {
  return &tuple->vals[index];
}

static inline void kltuple_index(KlTuple* tuple, KlInt index, KlValue* val) {
  if (kl_likely(0 <= index && index < (KlInt)kltuple_nval(tuple))) {
    klvalue_setvalue(val, kltuple_access(tuple, index));
  } else {
    klvalue_setnil(val);
  }
}

static inline KlException kltuple_indexas(KlTuple* tuple, size_t index, const KlValue* val) {
  if (kl_likely(index < kltuple_nval(tuple))) {
    klvalue_setvalue(kltuple_access(tuple, index), val);
    return KL_E_NONE;
  } else {
    return KL_E_RANGE;
  }
}

static inline const KlValue* kltuple_iter_begin(const KlTuple* tuple) {
  return tuple->vals;
}

static inline const KlValue* kltuple_iter_end(const KlTuple* tuple) {
  return tuple->vals + tuple->nval;
}

#endif
