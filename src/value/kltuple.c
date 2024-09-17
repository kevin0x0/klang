#include "include/value/kltuple.h"

static KlGCObject* kltuple_propagate(KlTuple* tuple, KlMM* klmm, KlGCObject* gclist);
static void kltuple_delete(KlTuple* tuple, KlMM* klmm);

static const KlGCVirtualFunc kltuple_gcvfunc = { .destructor = (KlGCDestructor)kltuple_delete, .propagate = (KlGCProp)kltuple_propagate, .after = NULL };


KlTuple* kltuple_create(KlMM* klmm, const KlValue* vals, size_t nval) {
  KlTuple* tuple = (KlTuple*)klmm_alloc(klmm, sizeof (KlTuple) + nval * sizeof (KlValue));
  if (kl_unlikely(!tuple)) return NULL;
  for (KlValue* p = tuple->vals; p != tuple->vals + nval; ++p)
    klvalue_setvalue(p, vals++);
  tuple->nval = nval;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(tuple), &kltuple_gcvfunc);
  return tuple;
}

static void kltuple_delete(KlTuple* tuple, KlMM* klmm) {
  klmm_free(klmm, tuple, kltuple_size(tuple));
}

static KlGCObject* kltuple_propagate(KlTuple* tuple, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  for (KlValue* val = tuple->vals; val != tuple->vals + tuple->nval; ++val) {
    if (klvalue_collectable(val))
      klmm_gcobj_mark(klvalue_getgcobj(val), gclist);
  }
  return gclist;
}
