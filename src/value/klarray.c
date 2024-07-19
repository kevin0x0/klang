#include "include/value/klarray.h"
#include "include/misc/klutils.h"

#include <stdlib.h>

#define KLARRAY_DEFAULT_SIZE      (8)

static KlGCObject* klarray_propagate(KlArray* array, KlMM* klmm, KlGCObject* gclist);
static void klarray_delete(KlArray* array, KlMM* klmm);

static const KlGCVirtualFunc klarray_gcvfunc = { .destructor = (KlGCDestructor)klarray_delete, .propagate = (KlGCProp)klarray_propagate, .after = NULL };


KlArray* klarray_create(KlMM* klmm, size_t capacity) {
  KlArray* array = (KlArray*)klmm_alloc(klmm, sizeof (KlArray));
  if (kl_unlikely(!array)) return NULL;
  if (kl_unlikely(!(array->begin = (KlValue*)klmm_alloc(klmm, sizeof (KlValue) * capacity)))) {
    klmm_free(klmm, array, sizeof (KlArray));
    return NULL;
  }
  array->capacity = capacity;
  array->size = 0;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(array), &klarray_gcvfunc);
  return array;
}

static void klarray_delete(KlArray* array, KlMM* klmm) {
  klmm_free(klmm, array->begin, klarray_capacity(array) * sizeof (KlValue));
  klmm_free(klmm, array, sizeof (KlArray));
}

bool klarray_grow(KlArray* array, KlMM* klmm, size_t new_capacity) {
  kl_assert((klarray_capacity(array) < new_capacity), "");

  new_capacity = new_capacity == 0 ? 4 : new_capacity;
  new_capacity = klarray_capacity(array) * 2 > new_capacity ? klarray_capacity(array) * 2 : new_capacity;
  KlValue* new_array = (KlValue*)klmm_realloc(klmm, array->begin, sizeof (KlValue) * new_capacity, sizeof (KlValue) * klarray_capacity(array));
  if (!new_array) return false;
  array->begin = new_array;
  array->capacity = new_capacity;
  return true;
}


static KlGCObject* klarray_propagate(KlArray* array, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  KlArrayIter end = klarray_iter_end(array);
  KlArrayIter begin = klarray_iter_begin(array);
  for (KlArrayIter itr = begin; itr != end; itr = klarray_iter_next(itr)) {
    if (klvalue_collectable(itr))
      klmm_gcobj_mark(klvalue_getgcobj(itr), gclist);
  }
  return gclist;
}
