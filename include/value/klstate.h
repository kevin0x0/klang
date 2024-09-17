#ifndef _KLANG_INCLUDE_VM_KLSTATE_H_
#define _KLANG_INCLUDE_VM_KLSTATE_H_

#include "include/lang/kltypes.h"
#include "include/mm/klmm.h"
#include "include/value/klcoroutine.h"
#include "include/vm/klstack.h"
#include "include/value/klmap.h"
#include "include/value/klclosure.h"
#include "include/vm/klexception.h"
#include "include/vm/klcommon.h"
#include "include/vm/klthrow.h"



#define KLSTATE_CI_STATUS_NORM  (0)
/* stop execution when RETURN */
#define KLSTATE_CI_STATUS_STOP  (klbit(0))
/* callable.cfunc valid */
#define KLSTATE_CI_STATUS_CFUN  (klbit(1))
/* callable.cclo valid */
#define KLSTATE_CI_STATUS_CCLO  (klbit(2))
/* callable.kclo valid */
#define KLSTATE_CI_STATUS_KCLO  (klbit(3))

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
  union {
    KlGCObject* clo;                /* klang closure or C closure(determined by status) */
    KlCFunction* cfunc;             /* C function in execution */
  } callable;
  union {
    struct {
      const KlInstruction* savedpc; /* pointed to current klang instruction */
      KlValue* top;                 /* stack frame top for this klang call */
    };
    struct {
      KlCIUD ud;                    /* userdata for C call when coroutine resuming */
      KlCFunction* afteryield;      /* function to be executed after yielding */
    } resume;
  };
  KlValue* base;                    /* stack base */
  int retoff;                       /* the offset of position relative to stkbase where returned values place. */
  KlUnsigned narg;                  /* actual number of received arguments */
  KlUByte nret;                     /* expected number of returned value */
  KlUByte status;
};

typedef struct tagKlState {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlThrowInfo throwinfo;            /* store the thrown but not handled exception */
  KlStack stack;                    /* stack */
  KlMM* klmm;                       /* memory manager */
  KlMap* global;                    /* global variables */
  KlCallInfo* callinfo;             /* call information of the closure in execution */
  KlCommon* common;                 /* store some gcobjects that are often used */
  KlRef* reflist;                   /* all open references */
  KlStrPool* strpool;               /* maintain all strings */
  KlCallInfo baseci;                /* the bottom of callinfo stack, never actually used. */
  KlCoroutine coinfo;               /* coroutine information */
} KlState;



KlState* klstate_create(KlMM* klmm, KlMap* global, KlCommon* common, KlStrPool* strpool, KlKClosure* kclo);

static inline KlMM* klstate_getmm(const KlState* state);
static inline const KlCallInfo* klstate_baseci(const KlState* state);
static inline KlCallInfo* klstate_currci(const KlState* state);
static inline void klstate_setcurrci(KlState* state, KlCallInfo* callinfo);
static inline KlStack* klstate_stack(KlState* state);
static inline KlCommon* klstate_common(const KlState* state);
static inline KlMap* klstate_global(const KlState* state);
static inline KlStrPool* klstate_strpool(const KlState* state);
static inline KlRef** klstate_reflist(KlState* state);

static inline size_t klstate_getnarg(const KlState* state);

static inline bool klstate_isrunning(const KlState* state);

static inline KlValue* klstate_stktop(KlState* state);

static inline KlException klstate_currexception(const KlState* state);
static inline KlState* klstate_exception_source(const KlState* state);
static inline const char* klstate_exception_message(const KlState* state);
static inline KlValue* klstate_exception_object(KlState* state);

KlException klstate_throw_link(KlState* state, KlState* src);
KlException klstate_throw(KlState* state, KlException type, const char* format, ...);
KlException klstate_throw_oom(KlState* state, const char* when);

/* offset must be negative */
static inline KlValue* klstate_getval(KlState* state, int offset);


static inline KlException klstate_checkframe(KlState* state, size_t framesize);
KlException klstate_growstack(KlState* state, size_t capacity);

static inline KlMM* klstate_getmm(const KlState* state) {
  return state->klmm;
}

static inline KlCallInfo* klstate_currci(const KlState* state) {
  return state->callinfo;
}

static inline void klstate_setcurrci(KlState* state, KlCallInfo* callinfo) {
  state->callinfo = callinfo;
}

static inline const KlCallInfo* klstate_baseci(const KlState* state) {
  return &state->baseci;
}

static inline KlStack* klstate_stack(KlState* state) {
  return &state->stack;
}

static inline KlCommon* klstate_common(const KlState* state) {
  return state->common;
}

static inline KlMap* klstate_global(const KlState* state) {
  return state->global;
}

static inline KlStrPool* klstate_strpool(const KlState* state) {
  return state->strpool;
}

static inline KlRef** klstate_reflist(KlState* state) {
  return &state->reflist;
}

static inline size_t klstate_getnarg(const KlState* state) {
  return state->callinfo->narg;
}

static inline bool klstate_isrunning(const KlState* state) {
  return state->callinfo != &state->baseci;
}

static inline KlValue* klstate_stktop(KlState* state) {
  return klstack_top(klstate_stack(state));
}

static inline KlException klstate_currexception(const KlState* state) {
  return klthrow_getetype(&state->throwinfo);
}

static inline KlState* klstate_exception_source(const KlState* state) {
  return klthrow_getesrc(&state->throwinfo);
}

static inline const char* klstate_exception_message(const KlState* state) {
  return klthrow_getemsg(&state->throwinfo);
}

static inline KlValue* klstate_exception_object(KlState* state) {
  return klthrow_geteobj(&state->throwinfo);
}

static inline KlValue* klstate_getval(KlState* state, int offset) {
  return klstate_stktop(state) + offset;
}

static inline KlException klstate_checkframe(KlState* state, size_t framesize) {
  if (kl_unlikely(klstack_residual(klstate_stack(state)) < framesize))
    return klstate_growstack(state, framesize);
  return KL_E_NONE;
}

#endif
