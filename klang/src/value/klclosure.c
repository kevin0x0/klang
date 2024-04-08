#include "klang/include/value/klclosure.h"
#include "utils/include/utils/utils.h"
#include <string.h>


static KlGCObject* klkclosure_propagate(KlKClosure* kclo, KlGCObject* gclist);
static KlGCObject* klcclosure_propagate(KlCClosure* cclo, KlGCObject* gclist);
static void klkclosure_delete(KlKClosure* kclo, KlMM* klmm);
static void klcclosure_delete(KlCClosure* cclo, KlMM* klmm);

static KlGCVirtualFunc klkclo_gcvfunc = { .destructor = (KlGCDestructor)klkclosure_delete, .propagate = (KlGCProp)klkclosure_propagate };
static KlGCVirtualFunc klcclo_gcvfunc = { .destructor = (KlGCDestructor)klcclosure_delete, .propagate = (KlGCProp)klcclosure_propagate };


KlKClosure* klkclosure_create(KlMM* klmm, KlKFunction* kfunc, KlValue* stkbase, KlRef** openreflist, KlRef** refs) {
  KlKClosure* kclo = (KlKClosure*)klmm_alloc(klmm, sizeof (KlKClosure) + sizeof (KlRef*) * kfunc->nref);
  if (k_unlikely(!kclo)) return NULL;
  klmm_newlevel(klmm, klmm_to_gcobj(kclo));
  kclo->kfunc = kfunc;
  size_t nref = kfunc->nref;
  kclo->nref = nref;
  KlRefInfo* refinfo = kfunc->refinfo;
  KlRef** ref = kclo->refs;
  KlRefInfo* end = refinfo + nref;
  for (; refinfo != end; ++refinfo, ++ref) {
    if (!refinfo->on_stack) {       /* not in the stack */
      *ref = refs[refinfo->index];  /* in parent closure */
      klref_pin(*ref);
      continue;
    }
    KlRef* newref = klref_get(openreflist, klmm, stkbase + refinfo->index);
    if (k_unlikely(!newref)) {
      klmm_newlevel_abort(klmm);
      klmm_free(klmm, kclo, sizeof (KlKClosure) + sizeof (KlRef*) * kfunc->nref);
      return NULL;
    }
    klref_pin(newref);
    *ref = newref;
  }

  kclo->status = KLCLO_STATUS_NORM;
  klmm_newlevel_done(klmm, &klkclo_gcvfunc);
  return kclo;
}

static void klkclosure_delete(KlKClosure* kclo, KlMM* klmm) {
  KlRef** refs = kclo->refs;
  size_t nref = kclo->nref;
  for (size_t i = 0; i < nref; ++i)
    klref_unpin(refs[i], klmm);
  klmm_free(klmm, kclo, sizeof (KlKClosure) + sizeof (KlRef*) * kclo->nref);
}

static KlGCObject* klkclosure_propagate(KlKClosure* kclo, KlGCObject* gclist) {
  klmm_gcobj_mark_accessible(klmm_to_gcobj(kclo->kfunc), gclist);
  for (size_t i = 0; i < kclo->nref; ++i) {
    if (klref_closed(kclo->refs[i]))
      gclist = klref_propagate(kclo->refs[i], gclist);
  }
  return gclist;
}

KlCClosure* klcclosure_create(KlMM* klmm, KlCFunction* cfunc, KlValue* stkbase, KlRef** openreflist, size_t nref) {
  KlCClosure* cclo = (KlCClosure*)klmm_alloc(klmm, sizeof (KlKClosure) + sizeof (KlRef*) * nref);
  if (k_unlikely(!cclo)) return NULL;
  klmm_newlevel(klmm, klmm_to_gcobj(cclo));
  cclo->cfunc = cfunc;
  cclo->nref = nref;
  KlRef** ref = cclo->refs;
  KlRef** end = ref + nref;
  for (; ref != end; ++ref) {
    KlRef* newref = klref_get(openreflist, klmm, stkbase++);
    if (k_unlikely(!newref)) {
      klmm_newlevel_abort(klmm);
      klmm_free(klmm, cclo, sizeof (KlCClosure) + sizeof (KlRef*) * cclo->nref);
      return NULL;
    }
    klref_pin(newref);
    *ref = newref;
  }

  cclo->status = KLCLO_STATUS_NORM;
  klmm_newlevel_done(klmm, &klcclo_gcvfunc);
  return cclo;
}

static void klcclosure_delete(KlCClosure* cclo, KlMM* klmm) {
  KlRef** refs = cclo->refs;
  size_t nref = cclo->nref;
  for (size_t i = 0; i < nref; ++i)
    klref_unpin(refs[i], klmm);
  klmm_free(klmm, cclo, sizeof (KlKClosure) + sizeof (KlRef*) * cclo->nref);
}

static KlGCObject* klcclosure_propagate(KlCClosure* cclo, KlGCObject* gclist) {
  for (size_t i = 0; i < cclo->nref; ++i) {
    if (klref_closed(cclo->refs[i]))
      gclist = klref_propagate(cclo->refs[i], gclist);
  }
  return gclist;
}
