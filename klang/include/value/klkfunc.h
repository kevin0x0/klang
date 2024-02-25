#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLKFUNC_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLKFUNC_H

#include "klang/include/mm/klmm.h"
#include "klang/include/vm/klinst.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/value/klref.h"
#include <stdbool.h>

typedef struct tagKlKFunction {
  KlGCObject gcbase;
  KlInstruction* code;    /* code executed by klang virtual machine */
  KlValue* constants;     /* constants table */
  KlRefInfo* refinfo;     /* reference information */
  size_t codelen;         /* code length */
  size_t nconst;          /* number of constants */
  size_t nref;            /* number of references */
  size_t framesize;       /* stack size needed by this klang function. (including parameters) */
  uint8_t nparam;         /* number of parameters */
} KlKFunction;

/* allocate memory and initialize some fields, but do not initialize the constants and refinfo.
 * the constants and refinfo should be initialized by caller.
 */
KlKFunction* klkfunc_alloc(KlMM* klmm, KlInstruction* code, size_t codelen, size_t nconst, size_t nref, size_t framesize, size_t nparam);
/* assume the initialization is done, enable gc for 'kfunc'. */
void klkfunc_initdone(KlMM* klmm, KlKFunction* kfunc);

static inline KlValue* klkfunc_constants(KlKFunction* kfunc);
static inline KlRefInfo* klkfunc_refinfo(KlKFunction* kfunc);
static inline KlInstruction* klkfunc_entrypoint(KlKFunction* kfunc);
static inline size_t klkfunc_framesize(KlKFunction* kfunc);
static inline size_t klkfunc_nparam(KlKFunction* kfunc);

static inline KlValue* klkfunc_constants(KlKFunction* kfunc) {
  return kfunc->constants;
}

static inline KlRefInfo* klkfunc_refinfo(KlKFunction* kfunc) {
  return kfunc->refinfo;
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
