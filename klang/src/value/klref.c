#include "klang/include/value/klref.h"


KlRef* klref_new(KlRef** reflist, KlMM* klmm, KlValue* stkval) {
  KlRef* ref = *reflist;
  KlRef* newref = (KlRef*)klmm_alloc(klmm, sizeof (KlRef));
  if (kl_unlikely(!newref)) return NULL;
  newref->pval = stkval;
  newref->open.next = ref;
  newref->pincount = 0;
  *reflist = newref;
  klref_pin(newref);
  return newref;
}

void klreflist_close(KlRef** reflist, KlValue* bound, KlMM* klmm) {
  KlRef* ref = *reflist;
  while (ref->pval >= bound) {
    KlRef* next = ref->open.next;
    klvalue_setvalue(&ref->closed.val, ref->pval);
    ref->pval = &ref->closed.val;
    klref_unpin(ref, klmm);
    ref = next;
  }
  *reflist = ref;
}

