#ifndef KEVCC_KLANG_INCLUDE_VM_KLEXEC_H
#define KEVCC_KLANG_INCLUDE_VM_KLEXEC_H

#include "klang/include/value/klcfunc.h"
#include "klang/include/value/klcoroutine.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klexception.h"
#include "klang/include/value/klstate.h"
#include <stddef.h>


#define klexec_savestack(state, stkptr)     ((stkptr) - klstack_raw(klstate_stack((state))))
#define klexec_restorestack(state, diff)    (klstack_raw(klstate_stack((state))) + (diff))

typedef KlException (*KlCallPrepCallBack)(KlState* state, KlValue* callable, size_t narg);

KlValue* klexec_getfield(KlState* state, KlValue* callable, KlString* op);
KlException klexec_callc(KlState* state, KlCFunction* cfunc, size_t narg, size_t nret);
KlException klexec_callprepare(KlState* state, KlValue* callable, size_t narg, KlCallPrepCallBack callback);
KlException klexec_call(KlState* state, KlValue* callable, size_t narg, size_t nret);
KlException klexec_execute(KlState* state);
static inline void klexec_pop_callinfo(KlState* state);
static inline void klexec_push_callinfo(KlState* state);
static inline KlCallInfo* klexec_new_callinfo(KlState* state, size_t nret, int retoff);
static inline KlCallInfo* klexec_newed_callinfo(KlState* state);
KlCallInfo* klexec_alloc_callinfo(KlState* state);

static inline void klexec_setnils(KlValue* vals, size_t ncopy);

/* 'a' should be stack value */
KlException klexec_dobinopmethod(KlState* state, KlValue* a, KlValue* b, KlValue* c, KlString* op);

KlException klexec_mapsearch(KlState* state, KlMap* map, KlValue* key, KlValue* res);
KlException klexec_mapinsert(KlState* state, KlMap* map, KlValue* key, KlValue* val);

static inline bool klexec_if(KlValue* val) {
  return !(klvalue_checktype(val, KL_NIL) || (klvalue_checktype(val, KL_BOOL) && klvalue_getbool(val) == KL_FALSE));
}

static inline void klexec_pop_callinfo(KlState* state) {
  state->callinfo = state->callinfo->prev;
}

static inline void klexec_push_callinfo(KlState* state) {
  state->callinfo = state->callinfo->next;
}

static inline KlCallInfo* klexec_new_callinfo(KlState* state, size_t nret, int retoff) {
  KlCallInfo* callinfo = state->callinfo->next ? state->callinfo->next : klexec_alloc_callinfo(state);
  if (kl_unlikely(!callinfo)) return NULL;
  callinfo->nret = nret;
  callinfo->retoff = retoff;
  return callinfo;
}

static inline KlCallInfo* klexec_newed_callinfo(KlState* state) {
  return state->callinfo->next;
}

static inline void klexec_setnils(KlValue* vals, size_t ncopy) {
  while (ncopy--) klvalue_setnil(vals);
}

#endif
