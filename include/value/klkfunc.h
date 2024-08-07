#ifndef _KLANG_INCLUDE_VALUE_KLKFUNC_H_
#define _KLANG_INCLUDE_VALUE_KLKFUNC_H_

#include "include/lang/kltypes.h"
#include "include/mm/klmm.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/value/klref.h"
#include "include/lang/klinst.h"
#include <stdbool.h>

typedef struct tagKlKFuncFilePosition {
  unsigned begin;
  unsigned end;
} KlKFuncFilePosition;

typedef struct tagKlKFunction KlKFunction;
struct tagKlKFunction {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlInstruction* code;        /* code executed by klang virtual machine */
  KlValue* constants;         /* constants table */
  KlRefInfo* refinfo;         /* reference information */
  KlKFunction** subfunc;      /* sub-functions, functions defined in this function */
  struct {
    KlKFuncFilePosition* posinfo;
    KlString* srcfile;
  } debuginfo;
  unsigned codelen;           /* code length */
  unsigned short nconst;      /* number of constants */
  unsigned short nref;        /* number of references */
  unsigned short nsubfunc;    /* number of sub-functions */
  KlUByte framesize;          /* stack size needed by this klang function. (including parameters) */
  KlUByte nparam;             /* number of parameters (including 'this' if it is method) */
};

/* allocate memory and initialize some fields, but do not initialize the constants and refinfo.
 * the constants and refinfo should be initialized by caller.
 */
KlKFunction* klkfunc_alloc(KlMM* klmm, unsigned codelen, unsigned short nconst,
                           unsigned short nref, unsigned short nsubfunc, KlUByte framesize, KlUByte nparam);
/* assume the initialization is done, enable gc for 'kfunc'. */
void klkfunc_initdone(KlMM* klmm, KlKFunction* kfunc);
/* init failed, clean resources */
void klkfunc_initabort(KlMM* klmm, KlKFunction* kfunc);

static inline KlInstruction* klkfunc_setinsts(KlKFunction* kfunc);
static inline KlValue* klkfunc_setconstants(KlKFunction* kfunc);
static inline KlRefInfo* klkfunc_setrefinfo(KlKFunction* kfunc);
static inline const KlValue* klkfunc_constants(const KlKFunction* kfunc);
static inline const KlRefInfo* klkfunc_refinfo(const KlKFunction* kfunc);
static inline KlKFunction** klkfunc_subfunc(KlKFunction* kfunc);
static inline KlKFuncFilePosition* klkfunc_posinfo(KlKFunction* kfunc);
static inline const KlString* klkfunc_srcfile(const KlKFunction* kfunc);
static inline const KlInstruction* klkfunc_entrypoint(const KlKFunction* kfunc);
static inline size_t klkfunc_codelen(const KlKFunction* kfunc);
static inline size_t klkfunc_framesize(const KlKFunction* kfunc);
static inline size_t klkfunc_nparam(const KlKFunction* kfunc);

static inline void klkfunc_setsrcfile(KlKFunction* kfunc, KlString* srcfile);
/* the 'posinfo' must be allocated by klmm */
static inline void klkfunc_setposinfo(KlKFunction* kfunc, KlKFuncFilePosition* posinfo);

static inline KlInstruction* klkfunc_setinsts(KlKFunction* kfunc) {
  return kfunc->code;
}

static inline KlValue* klkfunc_setconstants(KlKFunction* kfunc) {
  return kfunc->constants;
}

static inline const KlValue* klkfunc_constants(const KlKFunction* kfunc) {
  return kfunc->constants;
}

static inline KlRefInfo* klkfunc_setrefinfo(KlKFunction* kfunc) {
  return kfunc->refinfo;
}

static inline const KlRefInfo* klkfunc_refinfo(const KlKFunction* kfunc) {
  return kfunc->refinfo;
}

static inline KlKFunction** klkfunc_subfunc(KlKFunction* kfunc) {
  return kfunc->subfunc;
}

static inline KlKFuncFilePosition* klkfunc_posinfo(KlKFunction* kfunc) {
  return kfunc->debuginfo.posinfo;
}

static inline const KlString* klkfunc_srcfile(const KlKFunction* kfunc) {
  return kfunc->debuginfo.srcfile;
}

static inline const KlInstruction* klkfunc_entrypoint(const KlKFunction* kfunc) {
  return kfunc->code;
}

static inline size_t klkfunc_codelen(const KlKFunction* kfunc) {
  return kfunc->codelen;
}

static inline size_t klkfunc_framesize(const KlKFunction* kfunc) {
  return kfunc->framesize;
}

static inline size_t klkfunc_nparam(const KlKFunction* kfunc) {
  return kfunc->nparam;
}


static inline void klkfunc_setsrcfile(KlKFunction* kfunc, KlString* srcfile) {
  kfunc->debuginfo.srcfile = srcfile;
}

static inline void klkfunc_setposinfo(KlKFunction* kfunc, KlKFuncFilePosition* posinfo) {
  kl_assert(kfunc->debuginfo.posinfo == NULL, "can not reset position info");
  kfunc->debuginfo.posinfo = posinfo;
}

#endif
