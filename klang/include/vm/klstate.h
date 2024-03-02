#ifndef KEVCC_KLANG_INCLUDE_VM_KLSTATE_H
#define KEVCC_KLANG_INCLUDE_VM_KLSTATE_H

#include "klang/include/mm/klmm.h"
#include "klang/include/vm/klstack.h"
#include "klang/include/value/klmap.h"
#include "klang/include/value/klclosure.h"
#include "klang/include/vm/klexception.h"
#include "klang/include/vm/klcommon.h"
#include "klang/include/vm/klthrow.h"



#define KLSTATE_CI_STATUS_NORM  (0)
/* stop execution when RETURN */
#define KLSTATE_CI_STATUS_STOP  (klbit(0))
/* callable.cfunc valid */
#define KLSTATE_CI_STATUS_CFUN  (klbit(1))
/* callable.cclo valid */
#define KLSTATE_CI_STATUS_CCLO  (klbit(2))
/* callable.kclo valid */
#define KLSTATE_CI_STATUS_KCLO  (klbit(3))


typedef struct tagKlCallInfo KlCallInfo;
struct tagKlCallInfo {
  KlCallInfo* prev;
  KlCallInfo* next;
  KlValue env_this;             /* 'this' */
  union {
    KlGCObject* clo;            /* klang closure or C closure(determined by status) */
    KlCFunction* cfunc;         /* C function in execution */
  } callable;
  KlValue* top;                 /* stack frame top for this call */
  KlInstruction* savedpc;       /* pointed to current instruction */
  uint8_t nret;                 /* expected number of returned value */
  uint8_t narg;                 /* actual number of received arguments */
  int16_t retoff;               /* the offset of position relative to stkbase where returned values place. */
  uint32_t status;
};

typedef struct tagKlState {
  KlGCObject base;
  KlStack stack;                /* stack */
  KlThrowInfo throwinfo;        /* store the throwed but not handled exception */
  KlCommon* common;             /* store some gcobjects that are often used */
  KlRef* reflist;               /* all open references */
  KlStrPool* strpool;           /* maintain all strings */
  KlMapNodePool* mapnodepool;   /* used by KlMap when inserting a new k-v pair */
  KlCallInfo* callinfo;         /* call information of the closure in execution */
  KlMap* global;                /* store global variables */
  KlCallInfo baseci;            /* the bottom of callinfo stack, never actually used. */
} KlState;



KlState* klstate_create(KlMM* klmm, KlMap* global, KlCommon* common, KlStrPool* strpool, KlMapNodePool* mapnodepool);
void klstate_delete(KlState* state);

static inline KlMM* klstate_getmm(KlState* state);
static inline KlStack* klstate_getstk(KlState* state);

static inline size_t klstate_getnarg(KlState* state);

static inline KlValue* klstate_stktop(KlState* state);

KlException klstate_throw(KlState* state, KlException type, const char* format, ...);

/* offset must be negative */
static inline KlValue* klstate_getval(KlState* state, int offset);


KlValue* klstate_getmethod(KlState* state, KlValue* val, KlString* methodname);
static inline void klstate_set_this(KlCallInfo* callinfo, KlValue* val);


KlException klstate_alter_stack(KlState* state, size_t size);

static inline KlException klstate_checkframe(KlState* state, size_t framesize);
KlException klstate_growstack(KlState* state, size_t capacity);

static inline KlMM* klstate_getmm(KlState* state) {
  return klmm_gcobj_getmm(klmm_to_gcobj(state));
}

static inline KlStack* klstate_getstk(KlState* state) {
  return &state->stack;
}

static inline size_t klstate_getnarg(KlState* state) {
  return state->callinfo->narg;
}

static inline KlValue* klstate_stktop(KlState* state) {
  return klstack_top(klstate_getstk(state));
}

static inline KlValue* klstate_getval(KlState* state, int offset) {
  return klstate_stktop(state) + offset;
}

static inline void klstate_set_this(KlCallInfo* callinfo, KlValue* val) {
  klvalue_setvalue(&callinfo->env_this, val);
}

static inline KlException klstate_checkframe(KlState* state, size_t framesize) {
  if (kl_unlikely(klstack_residual(klstate_getstk(state)) < framesize))
    return klstate_growstack(state, framesize);
  return KL_E_NONE;
}

#endif
