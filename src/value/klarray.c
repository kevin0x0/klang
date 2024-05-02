#include "include/value/klarray.h"
#include "include/misc/klutils.h"
#include "include/vm/klexception.h"

#include <stdlib.h>

#define KLARRAY_DEFAULT_SIZE      (8)

static KlGCObject* klarray_propagate(KlArray* array, KlMM* klmm, KlGCObject* gclist);
static void klarray_delete(KlArray* array, KlMM* klmm);

static KlGCVirtualFunc klarray_gcvfunc = { .destructor = (KlGCDestructor)klarray_delete, .propagate = (KlGCProp)klarray_propagate };


KlArray* klarray_create(KlClass* arrayclass, KlMM* klmm, size_t capacity) {
  KlArray* array = (KlArray*)klclass_objalloc(arrayclass, klmm);
  if (kl_unlikely(!array)) return NULL;
  if (kl_unlikely(!(array->begin = (KlValue*)klmm_alloc(klmm, sizeof (KlValue) * capacity)))) {
    klobject_free(klcast(KlObject*, array), klmm);
    return NULL;
  }
  array->end = array->begin + capacity;
  array->current = array->begin;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(array), &klarray_gcvfunc);
  return array;
}

static void klarray_delete(KlArray* array, KlMM* klmm) {
  klmm_free(klmm, array->begin, klarray_capacity(array) * sizeof (KlValue));
  klobject_free(klcast(KlObject*, array), klmm);
}

static KlException klarray_constructor(KlClass* arrayclass, KlMM* klmm, KlValue* value) {
  KlArray* arr = klarray_create(arrayclass, klmm, 1);
  if (kl_unlikely(!arr)) return KL_E_OOM;
  klvalue_setobj(value, arr, KL_ARRAY);
  return KL_E_NONE;
}

KlClass* klarray_class(KlMM* klmm) {
  return klclass_create(klmm, 5, klobject_attrarrayoffset(KlArray), NULL, (KlObjectConstructor)klarray_constructor);
}

bool klarray_check_capacity(KlArray* array, KlMM* klmm, size_t new_capacity) {
  new_capacity = new_capacity == 0 ? 4 : new_capacity;
  if (klarray_capacity(array) >= new_capacity)
    return true;
  new_capacity = klarray_capacity(array) * 2 > new_capacity ? klarray_capacity(array) * 2 : new_capacity;
  KlValue* new_array = (KlValue*)klmm_realloc(klmm, array->begin, sizeof (KlValue) * new_capacity, sizeof (KlValue) * klarray_capacity(array));
  if (!new_array) return false;
  array->current = new_array + (array->current - array->begin);
  array->begin = new_array;
  array->end = new_array + new_capacity;
  return true;
}


static KlGCObject* klarray_propagate(KlArray* array, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  KlArrayIter end = klarray_iter_end(array);
  KlArrayIter begin = klarray_iter_begin(array);
  for (KlArrayIter itr = begin; itr != end; itr = klarray_iter_next(itr)) {
    if (klvalue_collectable(itr))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(itr), gclist);
  }
  return klobject_propagate_nomm(klcast(KlObject*, array), gclist);
}
