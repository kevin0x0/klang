#ifndef KEVCC_KLANG_INCLUDE_VM_KLSTATE_H
#define KEVCC_KLANG_INCLUDE_VM_KLSTATE_H

#include "klang/include/mm/klmm.h"
#include "klang/include/value/klcoroutine.h"
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
/* conditional jump requires a boolean value at callinfo->top */
#define KLSTATE_CI_STATUS_CJMP  (klbit(4))

typedef union tagKlCIUD {
  size_t ui;
  long long i;
  float f32;
  double f64;
  void* p;
} KlCIUD;

typedef struct tagKlCallInfo KlCallInfo;
struct tagKlCallInfo {
  KlCallInfo* prev;
  KlCallInfo* next;
  KlValue env_this;             /* 'this' */
  union {
    KlGCObject* clo;            /* klang closure or C closure(determined by status) */
    KlCFunction* cfunc;         /* C function in execution */
  } callable;
  union {
    KlValue* top;               /* stack frame top for this klang call */
    KlValue* base;              /* stack base for C call */
  };
  union {
    KlInstruction* savedpc;     /* pointed to current klang instruction */
    KlCIUD resume_ud;           /* userdata for C call when coroutine resuming */
  };
  uint8_t nret;                 /* expected number of returned value */
  uint8_t narg;                 /* actual number of received arguments */
  int16_t retoff;               /* the offset of position relative to stkbase where returned values place. */
  uint32_t status;
};

typedef struct tagKlState {
  KlGCObject gcbase;
  KlStack stack;                /* stack */
  KlThrowInfo throwinfo;        /* store the throwed but not handled exception */
  KlCommon* common;             /* store some gcobjects that are often used */
  KlRef* reflist;               /* all open references */
  KlStrPool* strpool;           /* maintain all strings */
  KlMapNodePool* mapnodepool;   /* used by KlMap when inserting a new k-v pair */
  KlCallInfo* callinfo;         /* call information of the closure in execution */
  KlMap* global;                /* store global variables */
  KlCallInfo baseci;            /* the bottom of callinfo stack, never actually used. */
  KlCoroutine coinfo;           /* coroutine information */
} KlState;



KlState* klstate_create(KlMM* klmm, KlMap* global, KlCommon* common, KlStrPool* strpool, KlMapNodePool* mapnodepool, KlKClosure* kclo);
void klstate_delete(KlState* state);

static inline KlMM* klstate_getmm(KlState* state);
static inline KlCallInfo* klstate_currci(KlState* state);
static inline KlStack* klstate_stack(KlState* state);
static inline KlCommon* klstate_common(KlState* state);
static inline KlMap* klstate_global(KlState* state);
static inline KlStrPool* klstate_strpool(KlState* state);
static inline KlMapNodePool* klstate_mapnodepool(KlState* state);

static inline size_t klstate_getnarg(KlState* state);

static inline bool klstate_isrunning(KlState* state);

static inline KlValue* klstate_stktop(KlState* state);

KlException klstate_throw_link(KlState* state, KlState* src);
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

static inline KlCallInfo* klstate_currci(KlState* state) {
  return state->callinfo;
}

static inline KlStack* klstate_stack(KlState* state) {
  return &state->stack;
}

static inline KlCommon* klstate_common(KlState* state) {
  return state->common;
}

static inline KlMap* klstate_global(KlState* state) {
  return state->global;
}

static inline KlStrPool* klstate_strpool(KlState* state) {
  return state->strpool;
}

static inline KlMapNodePool* klstate_mapnodepool(KlState* state) {
  return state->mapnodepool;
}

static inline size_t klstate_getnarg(KlState* state) {
  return state->callinfo->narg;
}

static inline bool klstate_isrunning(KlState* state) {
  return klstate_currci(state) != &state->baseci;
}

static inline KlValue* klstate_stktop(KlState* state) {
  return klstack_top(klstate_stack(state));
}

static inline KlValue* klstate_getval(KlState* state, int offset) {
  return klstate_stktop(state) + offset;
}

static inline void klstate_set_this(KlCallInfo* callinfo, KlValue* val) {
  klvalue_setvalue(&callinfo->env_this, val);
}

static inline KlException klstate_checkframe(KlState* state, size_t framesize) {
  if (kl_unlikely(klstack_residual(klstate_stack(state)) < framesize))
    return klstate_growstack(state, framesize);
  return KL_E_NONE;
}

#endif
