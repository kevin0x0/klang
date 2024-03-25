#include "klang/include/value/klkfunc.h"
#include "klang/include/value/klvalue.h"

static KlGCObject* klkfunc_propagate(KlKFunction* kfunc, KlGCObject* gclist);
static void klkfunc_delete(KlKFunction* kfunc);

static KlGCVirtualFunc klkfunc_gcvfunc = { .destructor = (KlGCDestructor)klkfunc_delete, .propagate = (KlGCProp)klkfunc_propagate };


KlKFunction* klkfunc_alloc(KlMM* klmm, KlInstruction* code, size_t codelen, size_t nconst, size_t nref, size_t framesize, size_t nparam, bool is_method) {
  KlKFunction* kfunc = (KlKFunction*)klmm_alloc(klmm, sizeof (KlKFunction));
  KlValue* constants = (KlValue*)klmm_alloc(klmm, sizeof (KlValue) * nconst);
  KlRefInfo* refinfo = (KlRefInfo*)klmm_alloc(klmm, sizeof (KlRefInfo) * nref);
  if (kl_unlikely(!kfunc || !constants || !refinfo)) {
    klmm_free(klmm, kfunc, sizeof (KlKFunction));
    klmm_free(klmm, constants, sizeof (KlValue) * nconst);
    klmm_free(klmm, refinfo, sizeof (KlRefInfo) * nref);
    return NULL;
  }
  kfunc->code = code;
  kfunc->codelen = codelen;
  kfunc->constants = constants;
  kfunc->nconst = nconst;
  kfunc->refinfo = refinfo;
  kfunc->nref = nref;
  kfunc->framesize = framesize;
  kfunc->nparam = nparam;
  kfunc->is_method = is_method;
  return kfunc;
}

void klkfunc_initdone(KlMM* klmm, KlKFunction* kfunc) {
  klmm_gcobj_enable(klmm, klmm_to_gcobj(kfunc), &klkfunc_gcvfunc);
}

static KlGCObject* klkfunc_propagate(KlKFunction* kfunc, KlGCObject* gclist) {
  for (KlValue* constant = kfunc->constants;
       constant != kfunc->constants + kfunc->nconst;
       ++constant) {
    if (klvalue_collectable(constant))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(constant), gclist);
  }
  return gclist;
}

static void klkfunc_delete(KlKFunction* kfunc) {
  KlMM* klmm = klmm_gcobj_getmm(klmm_to_gcobj(kfunc));
  klmm_free(klmm, kfunc->constants, kfunc->nconst * sizeof (KlValue));
  klmm_free(klmm, kfunc->refinfo, kfunc->nref * sizeof (KlRefInfo));
  klmm_free(klmm, kfunc->code, kfunc->codelen * sizeof (KlInstruction));
  klmm_free(klmm, kfunc, sizeof (KlKFunction));
}
