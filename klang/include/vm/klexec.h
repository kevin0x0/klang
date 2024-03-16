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


KlValue* klexec_getfield(KlState* state, KlValue* callable, KlString* op);
KlException klexec_callc(KlState* state, KlCFunction* cfunc, size_t narg, size_t nret);
KlException klexec_callprepare(KlState* state, KlCallInfo* callinfo, KlValue* callable, size_t narg);
KlException klexec_call(KlState* state, KlValue* callable, size_t narg, size_t nret);
KlException klexec_call_noyield(KlState* state, KlValue* callable, size_t narg, size_t nret);
static inline KlException klexec_method(KlState* state, KlValue* thisobj, KlValue* callable, size_t narg, size_t nret);
KlException klexec_execute(KlState* state);
static inline void klexec_pop_callinfo(KlState* state);
static inline void klexec_push_callinfo(KlState* state);
static inline KlCallInfo* klexec_new_callinfo(KlState* state, size_t nret, int retoff);
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

static inline void klexec_setnils(KlValue* vals, size_t ncopy) {
  while (ncopy--) klvalue_setnil(vals);
}

static inline KlException klexec_method(KlState* state, KlValue* thisobj, KlValue* callable, size_t narg, size_t nret) {
  KlCallInfo* prevci = state->callinfo;
  KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
  klvalue_setvalue(&newci->env_this, thisobj);
  KlException exception = klexec_callprepare(state, newci, callable, narg);
  if (exception) return exception;
  if (kl_likely(prevci != state->callinfo)) { /* is a klang call ? */
    state->callinfo->status |= KLSTATE_CI_STATUS_STOP;
    return klexec_execute(state);
  }
  return KL_E_NONE;
}

static inline KlException klexec_callophash(KlState* state, size_t* hash, KlValue* key, KlValue* hashmethod) {
  if (kl_unlikely(!klvalue_callable(hashmethod))) {
    return klstate_throw(state, KL_E_TYPE,
                         "hash method is expected to be closure or C function , got %s",
                         klvalue_typename(klvalue_gettype(hashmethod)));
  }
  KlException exception = klexec_method(state, key, hashmethod, 0, 1);
  if (kl_unlikely(exception)) return exception;
  KlValue* res = klstate_getval(state, -1);
  if (kl_unlikely(!klvalue_checktype(res, KL_INT))) {
    return klstate_throw(state, KL_E_TYPE,
                         "expect integer returned by hash method, got %s",
                         klvalue_typename(klvalue_gettype(res)));
  }
  *hash = klvalue_getint(res);
  return KL_E_NONE;
}

static inline KlException klexec_callopindexas(KlState* state, KlValue* indexable, KlValue* key, KlValue* val) {
  klstack_pushvalue(klstate_stack(state), key);
  klstack_pushvalue(klstate_stack(state), val);
  KlValue* method = klexec_getfield(state, indexable, state->common->string.indexas);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(state->common->string.indexas));
  }
  KlException exception = klexec_method(state, indexable, method, 2, 0);
  return exception;
}

#endif
