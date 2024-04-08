#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCLOSURE_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCLOSURE_H

#include "klang/include/value/klcfunc.h"
#include "klang/include/value/klkfunc.h"
#include "klang/include/value/klref.h"

#define KLCLO_STATUS_NORM   (0)
/* is a method */
#define KLCLO_STATUS_METH   (klbit(0))

#define klclosure_checkrange(clo, refidx)   ((refidx) < (clo)->nref)

#define klclosure_set(clo, val)             ((clo)->status |= (val))
#define klclosure_clr(clo, val)             ((clo)->status &= ~(val))
#define klclosure_test(clo, val)            ((clo)->status & (val))

#define klclosure_ismethod(clo)             klclosure_test((clo), KLCLO_STATUS_METH)


typedef struct tagKlClosure {
  KlGCObject gcbase;
  void* func;
  size_t nref;
  int status;
  KlRef* refs[];
} KlClosure;


typedef struct tagKlKClosure {
  KlGCObject gcbase;
  KlKFunction* kfunc;
  size_t nref;
  int status;
  KlRef* refs[];
} KlKClosure;

typedef struct tagKlCClosure {
  KlGCObject gcbase;
  KlCFunction* cfunc;
  size_t nref;
  int status;
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
