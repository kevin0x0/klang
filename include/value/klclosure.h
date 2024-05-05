#ifndef _KLANG_INCLUDE_VALUE_KLCLOSURE_H_
#define _KLANG_INCLUDE_VALUE_KLCLOSURE_H_

#include "include/value/klcfunc.h"
#include "include/value/klkfunc.h"
#include "include/value/klref.h"

#define KLCLO_STATUS_NORM   (0)
/* is a method */
#define KLCLO_STATUS_METH   (klbit(0))

#define klclosure_checkrange(clo, refidx)   ((refidx) < (clo)->nref)

#define klclosure_set(clo, val)             ((clo)->status |= (val))
#define klclosure_clr(clo, val)             ((clo)->status &= ~(val))
#define klclosure_test(clo, val)            ((clo)->status & (val))

#define klclosure_ismethod(clo)             klclosure_test((clo), KLCLO_STATUS_METH)


typedef struct tagKlClosure {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  void* func;
  int status;
  unsigned short nref;
  KlRef* refs[];
} KlClosure;


typedef struct tagKlKClosure {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlKFunction* kfunc;
  int status;
  unsigned short nref;
  KlRef* refs[];
} KlKClosure;

typedef struct tagKlCClosure {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlCFunction* cfunc;
  int status;
  unsigned short nref;
  KlRef* refs[];
} KlCClosure;

KlKClosure* klkclosure_create(KlMM* klmm, KlKFunction* kfunc, KlValue* stkbase, KlRef** openreflist, KlRef** refs);
KlCClosure* klcclosure_create(KlMM* klmm, KlCFunction* cfunc, KlValue* stkbase, KlRef** openreflist, size_t ref_no);

static inline KlRef* klkclosure_getref(KlKClosure* kclo, size_t ref_no) {
  return kclo->refs[ref_no];
}

static inline KlRef* klcclosure_getref(KlCClosure* cclo, size_t ref_no) {
  return cclo->refs[ref_no];
}
#endif
