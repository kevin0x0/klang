#ifndef _KLANG_INCLUDE_VALUE_KLKFUNC_H_
#define _KLANG_INCLUDE_VALUE_KLKFUNC_H_

#include "include/lang/kltypes.h"
#include "include/mm/klmm.h"
#include "include/value/klvalue.h"
#include "include/value/klref.h"
#include "include/lang/klinst.h"
#include <stdbool.h>

typedef struct tagKlKFunction KlKFunction;
struct tagKlKFunction {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlInstruction* code;      /* code executed by klang virtual machine */
  KlValue* constants;       /* constants table */
  KlRefInfo* refinfo;       /* reference information */
  KlKFunction** subfunc;    /* sub-functions, functions defined in this function */
  unsigned codelen;         /* code length */
  unsigned short nconst;    /* number of constants */
  unsigned short nref;      /* number of references */
  unsigned short nsubfunc;  /* number of sub-functions */
  KlUByte framesize;        /* stack size needed by this klang function. (including parameters) */
  KlUByte nparam;           /* number of parameters (including 'this' if it is method) */
};

/* allocate memory and initialize some fields, but do not initialize the constants and refinfo.
 * the constants and refinfo should be initialized by caller.
 */
KlKFunction* klkfunc_alloc(KlMM* klmm, KlInstruction* code, unsigned codelen, unsigned short nconst,
                           unsigned short nref, unsigned short nsubfunc, KlUByte framesize, KlUByte nparam);
/* assume the initialization is done, enable gc for 'kfunc'. */
void klkfunc_initdone(KlMM* klmm, KlKFunction* kfunc);

static inline KlValue* klkfunc_constants(KlKFunction* kfunc);
static inline KlRefInfo* klkfunc_refinfo(KlKFunction* kfunc);
static inline KlKFunction** klkfunc_subfunc(KlKFunction* kfunc);
static inline KlInstruction* klkfunc_entrypoint(KlKFunction* kfunc);
static inline size_t klkfunc_framesize(KlKFunction* kfunc);
static inline size_t klkfunc_nparam(KlKFunction* kfunc);

static inline KlValue* klkfunc_constants(KlKFunction* kfunc) {
  return kfunc->constants;
}

static inline KlRefInfo* klkfunc_refinfo(KlKFunction* kfunc) {
  return kfunc->refinfo;
}

static inline KlKFunction** klkfunc_subfunc(KlKFunction* kfunc) {
  return kfunc->subfunc;
}

static inline KlInstruction* klkfunc_entrypoint(KlKFunction* kfunc) {
  return kfunc->code;
}

static inline size_t klkfunc_framesize(KlKFunction* kfunc) {
  return kfunc->framesize;
}

static inline size_t klkfunc_nparam(KlKFunction* kfunc) {
  return kfunc->nparam;
}

#endif
