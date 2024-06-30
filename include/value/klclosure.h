#ifndef _KLANG_INCLUDE_VALUE_KLCLOSURE_H_
#define _KLANG_INCLUDE_VALUE_KLCLOSURE_H_

#include "include/value/klcfunc.h"
#include "include/value/klkfunc.h"
#include "include/value/klref.h"

#define klclosure_checkrange(clo, refidx)   ((refidx) < (clo)->nref)


typedef struct tagKlClosure {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  void* func;
  unsigned short nref;
  KlRef* refs[];
} KlClosure;


typedef struct tagKlKClosure {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlKFunction* kfunc;
  unsigned short nref;
  KlRef* refs[];
} KlKClosure;

typedef struct tagKlCClosure {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlCFunction* cfunc;
  unsigned short nref;
  KlRef* refs[];
} KlCClosure;

KlKClosure* klkclosure_create(KlMM* klmm, KlKFunction* kfunc, KlValue* stkbase, KlRef** openreflist, KlRef** refs);
KlCClosure* klcclosure_create(KlMM* klmm, KlCFunction* cfunc, KlValue* stkbase, KlRef** openreflist, unsigned short ref_no);

static inline KlRef* klkclosure_getref(KlKClosure* kclo, unsigned short ref_no) {
  return kclo->refs[ref_no];
}

static inline KlRef* klcclosure_getref(KlCClosure* cclo, unsigned short ref_no) {
  return cclo->refs[ref_no];
}
#endif
