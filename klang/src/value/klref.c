#include "klang/include/value/klref.h"


static KlGCObject* klref_propagate(KlRef* ref, KlGCObject* gclist);
static void klref_delete(KlRef* ref);

static KlGCVirtualFunc klref_gcvfunc = { .destructor = (KlGCDestructor)klref_delete, .propagate = (KlGCProp)klref_propagate };

KlRef* klref_new(KlMM* klmm, KlRef** reflist, KlValue* stkval) {
  KlRef* ref = *reflist;
  KlRef* newref = (KlRef*)klmm_alloc(klmm, sizeof (KlRef));
  if (kl_unlikely(!newref)) return NULL;
  newref->pval = stkval;
  newref->open.next = ref;
  *reflist = newref;
  return newref;
}

static void klref_delete(KlRef* ref) {
  klmm_free(klmm_gcobj_getmm(klmm_to_gcobj(ref)), ref, sizeof (KlRef));
}

static KlGCObject* klref_propagate(KlRef* ref, KlGCObject* gclist) {
  if (klvalue_collectable(&ref->closed.val))
    klmm_gcobj_mark_accessible(klvalue_getgcobj(&ref->closed.val), gclist);
  return gclist;
}

void klref_close(KlRef** reflist, KlValue* bound, KlMM* klmm) {
  KlRef* ref = *reflist;
  while (ref->pval >= bound) {
    klvalue_setvalue(&ref->closed.val, ref->pval);
    ref->pval = &ref->closed.val;
    klmm_gcobj_enable(klmm, klmm_to_gcobj(ref), &klref_gcvfunc);
    ref = ref->open.next;
  }
  *reflist = ref;
}

