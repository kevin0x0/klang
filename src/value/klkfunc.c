#include "include/value/klkfunc.h"
#include "include/lang/klinst.h"
#include "include/lang/kltypes.h"
#include "include/misc/klutils.h"
#include "include/value/klvalue.h"

static KlGCObject* klkfunc_propagate(KlKFunction* kfunc, KlMM* klmm, KlGCObject* gclist);
static void klkfunc_delete(KlKFunction* kfunc, KlMM* klmm);

static KlGCVirtualFunc klkfunc_gcvfunc = { .destructor = (KlGCDestructor)klkfunc_delete, .propagate = (KlGCProp)klkfunc_propagate, .after = NULL };


KlKFunction* klkfunc_alloc(KlMM* klmm, KlInstruction* code, unsigned codelen, unsigned short nconst, unsigned short nref, unsigned short nsubfunc, KlUByte framesize, KlUByte nparam) {
  KlKFunction* kfunc = (KlKFunction*)klmm_alloc(klmm, sizeof (KlKFunction));
  KlValue* constants = (KlValue*)klmm_alloc(klmm, sizeof (KlValue) * nconst);
  KlRefInfo* refinfo = (KlRefInfo*)klmm_alloc(klmm, sizeof (KlRefInfo) * nref);
  KlKFunction** subfunc = (KlKFunction**)klmm_alloc(klmm, sizeof (KlKFunction*) * nsubfunc);
  if (kl_unlikely(!kfunc || !constants || !refinfo || !subfunc)) {
    klmm_free(klmm, kfunc, sizeof (KlKFunction));
    klmm_free(klmm, constants, sizeof (KlValue) * nconst);
    klmm_free(klmm, refinfo, sizeof (KlRefInfo) * nref);
    return NULL;
  }
  kfunc->code = code;
  kfunc->codelen = codelen;
  kfunc->constants = constants;
  kfunc->subfunc = subfunc;
  kfunc->nconst = nconst;
  kfunc->refinfo = refinfo;
  kfunc->nref = nref;
  kfunc->nsubfunc = nsubfunc;
  kfunc->framesize = framesize;
  kfunc->nparam = nparam;
  kfunc->debuginfo.posinfo = NULL;
  kfunc->debuginfo.srcfile = NULL;
  return kfunc;
}

void klkfunc_initdone(KlMM* klmm, KlKFunction* kfunc) {
  klmm_gcobj_enable(klmm, klmm_to_gcobj(kfunc), &klkfunc_gcvfunc);
}

static KlGCObject* klkfunc_propagate(KlKFunction* kfunc, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  for (KlValue* constant = kfunc->constants;
       constant != kfunc->constants + kfunc->nconst;
       ++constant) {
    if (klvalue_collectable(constant))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(constant), gclist);
  }
  for (KlKFunction** itr = kfunc->subfunc; itr != kfunc->subfunc + kfunc->nsubfunc; ++itr)
    klmm_gcobj_mark_accessible(klmm_to_gcobj(*itr), gclist);
  return gclist;
}

static void klkfunc_delete(KlKFunction* kfunc, KlMM* klmm) {
  klmm_free(klmm, kfunc->constants, kfunc->nconst * sizeof (KlValue));
  klmm_free(klmm, kfunc->refinfo, kfunc->nref * sizeof (KlRefInfo));
  klmm_free(klmm, kfunc->subfunc, kfunc->nsubfunc * sizeof (KlKFunction*));
  klmm_free(klmm, kfunc->code, kfunc->codelen * sizeof (KlInstruction));
  if (kfunc->debuginfo.srcfile)
    klmm_free(klmm, kfunc->debuginfo.srcfile, kfunc->debuginfo.srcfilelen);
  if (kfunc->debuginfo.posinfo)
    klmm_free(klmm, kfunc->debuginfo.posinfo, kfunc->codelen * sizeof (KlKFuncFilePosition));
  klmm_free(klmm, kfunc, sizeof (KlKFunction));
}
