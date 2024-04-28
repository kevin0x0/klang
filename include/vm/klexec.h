#ifndef KEVCC_KLANG_INCLUDE_VM_KLEXEC_H
#define KEVCC_KLANG_INCLUDE_VM_KLEXEC_H

#include "include/value/klcfunc.h"
#include "include/value/klcoroutine.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include "include/value/klstate.h"


#define klexec_savestack(state, stkptr)     ((stkptr) - klstack_raw(klstate_stack((state))))
#define klexec_restorestack(state, diff)    (klstack_raw(klstate_stack((state))) + (diff))
#define klexec_satisfy(val, cond)           ((klvalue_checktype((val), KL_BOOL) && klvalue_getbool((val)) == (cond)) || (!klvalue_checktype((val), KL_NIL)) == (cond))


typedef KlException (*KlCallPrepCallBack)(KlState* state, KlValue* callable, size_t narg);

KlValue* klexec_getfield(KlState* state, KlValue* object, KlString* field);
KlException klexec_callc(KlState* state, KlCFunction* cfunc, size_t narg, size_t nret);
KlException klexec_callprepare(KlState* state, KlValue* callable, size_t narg, KlCallPrepCallBack callback);
KlException klexec_call(KlState* state, KlValue* callable, size_t narg, size_t nret);
KlException klexec_execute(KlState* state);
static inline void klexec_pop_callinfo(KlState* state);
static inline void klexec_push_callinfo(KlState* state);
static inline KlCallInfo* klexec_new_callinfo(KlState* state, size_t nret, int retoff);
static inline KlCallInfo* klexec_newed_callinfo(KlState* state);
KlCallInfo* klexec_alloc_callinfo(KlState* state);

static inline void klexec_setnils(KlValue* vals, size_t nnil);

/* 'a' should be stack value */
KlException klexec_dobinopmethod(KlState* state, KlValue* a, KlValue* b, KlValue* c, KlString* op);

KlException klexec_mapsearch(KlState* state, KlMap* map, KlValue* key, KlValue* res);
KlException klexec_mapinsert(KlState* state, KlMap* map, KlValue* key, KlValue* val);


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

static inline void klexec_setnils(KlValue* vals, size_t nnil) {
  while (nnil--) klvalue_setnil(vals);
}

#endif
