#include "include/value/klclosure.h"
#include "include/misc/klutils.h"
#include "include/mm/klmm.h"
#include <string.h>


static KlGCObject* klkclosure_propagate(const KlKClosure* kclo, KlMM* klmm, KlGCObject* gclist);
static KlGCObject* klcclosure_propagate(const KlCClosure* cclo, KlMM* klmm, KlGCObject* gclist);
static void klkclosure_delete(KlKClosure* kclo, KlMM* klmm);
static void klcclosure_delete(KlCClosure* cclo, KlMM* klmm);

static const KlGCVirtualFunc klkclo_gcvfunc = { .destructor = (KlGCDestructor)klkclosure_delete, .propagate = (KlGCProp)klkclosure_propagate, .after = NULL };
static const KlGCVirtualFunc klcclo_gcvfunc = { .destructor = (KlGCDestructor)klcclosure_delete, .propagate = (KlGCProp)klcclosure_propagate, .after = NULL };


KlKClosure* klkclosure_create(KlMM* klmm, KlKFunction* kfunc, KlValue* stkbase, KlRef** openreflist, KlRef* const* refs) {
  KlKClosure* kclo = (KlKClosure*)klmm_alloc(klmm, sizeof (KlKClosure) + sizeof (KlRef*) * kfunc->nref);
  if (kl_unlikely(!kclo)) return NULL;
  kclo->kfunc = kfunc;
  unsigned short nref = kfunc->nref;
  kclo->nref = nref;
  const KlRefInfo* refinfo = kfunc->refinfo;
  KlRef** ref = kclo->refs;
  const KlRefInfo* end = refinfo + nref;
  for (; refinfo != end; ++refinfo, ++ref) {
    if (!refinfo->on_stack) {       /* not on the stack */
      *ref = refs[refinfo->index];  /* in parent closure */
      klref_pin(*ref);
      continue;
    }
    KlRef* newref = klref_get(openreflist, klmm, stkbase + refinfo->index);
    if (kl_unlikely(!newref)) {
      for (KlRef** unpinref = kclo->refs; unpinref != ref; ++unpinref)
        klref_unpin(*unpinref, klmm);
      klmm_free(klmm, kclo, sizeof (KlKClosure) + sizeof (KlRef*) * kfunc->nref);
      return NULL;
    }
    klref_pin(newref);
    *ref = newref;
  }

  klmm_gcobj_enable(klmm, klmm_to_gcobj(kclo), &klkclo_gcvfunc);
  return kclo;
}

static void klkclosure_delete(KlKClosure* kclo, KlMM* klmm) {
  KlRef** refs = kclo->refs;
  unsigned short nref = kclo->nref;
  for (size_t i = 0; i < nref; ++i)
    klref_unpin(refs[i], klmm);
  klmm_free(klmm, kclo, sizeof (KlKClosure) + sizeof (KlRef*) * kclo->nref);
}

static KlGCObject* klkclosure_propagate(const KlKClosure* kclo, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  klmm_gcobj_mark(klmm_to_gcobj(kclo->kfunc), gclist);
  for (size_t i = 0; i < kclo->nref; ++i) {
    if (klref_closed(kclo->refs[i]))
      gclist = klref_propagate(kclo->refs[i], gclist);
  }
  return gclist;
}

KlCClosure* klcclosure_create(KlMM* klmm, KlCFunction* cfunc, KlValue* stkbase, KlRef** openreflist, unsigned short nref) {
  KlCClosure* cclo = (KlCClosure*)klmm_alloc(klmm, sizeof (KlKClosure) + sizeof (KlRef*) * nref);
  if (kl_unlikely(!cclo)) return NULL;
  cclo->cfunc = cfunc;
  cclo->nref = nref;
  KlRef** ref = cclo->refs;
  KlRef** end = ref + nref;
  for (; ref != end; ++ref) {
    KlRef* newref = klref_get(openreflist, klmm, stkbase++);
    if (kl_unlikely(!newref)) {
      for (KlRef** unpinref = cclo->refs; unpinref != ref; ++unpinref)
        klref_unpin(*unpinref, klmm);
      klmm_free(klmm, cclo, sizeof (KlCClosure) + sizeof (KlRef*) * cclo->nref);
      return NULL;
    }
    klref_pin(newref);
    *ref = newref;
  }

  klmm_gcobj_enable(klmm, klmm_to_gcobj(cclo), &klcclo_gcvfunc);
  return cclo;
}

static void klcclosure_delete(KlCClosure* cclo, KlMM* klmm) {
  KlRef** refs = cclo->refs;
  size_t nref = cclo->nref;
  for (size_t i = 0; i < nref; ++i)
    klref_unpin(refs[i], klmm);
  klmm_free(klmm, cclo, sizeof (KlKClosure) + sizeof (KlRef*) * cclo->nref);
}

static KlGCObject* klcclosure_propagate(const KlCClosure* cclo, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  for (size_t i = 0; i < cclo->nref; ++i) {
    if (klref_closed(cclo->refs[i]))
      gclist = klref_propagate(cclo->refs[i], gclist);
  }
  return gclist;
}
