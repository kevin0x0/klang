#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCLOSURE_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCLOSURE_H

#include "klang/include/value/klcfunc.h"
#include "klang/include/value/klkfunc.h"
#include "klang/include/value/klref.h"


#define klclosure_checkrange(clo, refidx)   ((refidx) < (clo)->nref)


typedef struct tagKlClosure {
  KlGCObject gcbase;
  void* func;
  size_t nref;
  KlRef* refs[];
} KlClosure;


typedef struct tagKlKClosure {
  KlGCObject gcbase;
  KlKFunction* kfunc;
  size_t nref;
  KlRef* refs[];
} KlKClosure;

typedef struct tagKlCClosure {
  KlGCObject gcbase;
  KlCFunction* cfunc;
  size_t nref;
  KlRef* refs[];
} KlCClosure;

KlKClosure* klkclosure_create(KlMM* klmm, KlKFunction* kfunc, KlValue* stkbase, KlRef** openreflist, KlRef** refs);
KlCClosure* klcclosure_create(KlMM* klmm, KlCFunction* cfunc, KlValue* stkbase, KlRef** openreflist, size_t ref_no);

static inline KlRef* klclosure_getref(KlClosure* clo, size_t refidx);

static inline KlRef* klclosure_getref(KlClosure* clo, size_t ref_no) {
  return clo->refs[ref_no];
}


#endif
