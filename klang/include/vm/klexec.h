#ifndef KEVCC_KLANG_INCLUDE_VM_KLEXEC_H
#define KEVCC_KLANG_INCLUDE_VM_KLEXEC_H

#include "klang/include/value/klcfunc.h"
#include "klang/include/vm/klstate.h"


#define klexec_savestack(state, stkptr)     ((stkptr) - klstack_raw(klstate_getstk((state))))
#define klexec_restorestack(state, diff)    (klstack_raw(klstate_getstk((state))) + (diff))


KlValue* klexec_getfield(KlState* state, KlValue* callable, KlString* op);
KlException klexec_concat(KlState* state, size_t nparam);
KlException klexec_callc(KlState* state, KlCFunction* cfunc, size_t narg, size_t nret);
KlException klexec_callprepare(KlState* state, KlCallInfo* callinfo, KlValue* callable, size_t narg);
KlException klexec_methodprepare(KlState* state, KlCallInfo* callinfo, KlValue* method, size_t narg);
static inline KlException klexec_call(KlState* state, KlValue* callable, size_t narg, size_t nret);
static inline KlException klexec_method(KlState* state, KlValue* thisobj, KlValue* callable, size_t narg, size_t nret);
KlException klexec_execute(KlState* state);
static inline void klexec_pop_callinfo(KlState* state);
static inline void klexec_push_callinfo(KlState* state);
static inline KlCallInfo* klexec_new_callinfo(KlState* state, size_t nret, int retoff);
KlCallInfo* klexec_alloc_callinfo(KlState* state);

/* 'a' should be stack value */
KlException klexec_dobinopmethod(KlState* state, KlValue* a, KlValue* b, KlValue* c, KlString* op);
/* 'a' should be stack value */
KlException klexec_dobinopmethodi(KlState* state, KlValue* a, KlValue* b, KlInt c, KlString* op);
static inline KlException klexec_callbinopmethod(KlState* state, KlString* op);

KlException klexec_equal(KlState* state, KlValue* a, KlValue* b);
KlException klexec_notequal(KlState* state, KlValue* a, KlValue* b);
KlException klexec_lt(KlState* state, KlValue* a, KlValue* b);
KlException klexec_le(KlState* state, KlValue* a, KlValue* b);
KlException klexec_gt(KlState* state, KlValue* a, KlValue* b);
KlException klexec_ge(KlState* state, KlValue* a, KlValue* b);

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
  callinfo->status = KLSTATE_CI_STATUS_NORM;
  return callinfo;
}

static inline KlException klexec_call(KlState* state, KlValue* callable, size_t narg, size_t nret) {
  KlCallInfo* prevci = state->callinfo;
  KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
  klvalue_setnil(&newci->env_this);
  KlException exception = klexec_callprepare(state, newci, callable, narg);
  if (exception) return exception;
  if (kl_likely(prevci != state->callinfo)) { /* is a klang call ? */
    state->callinfo->status |= KLSTATE_CI_STATUS_STOP;
    return klexec_execute(state);
  }
  return KL_E_NONE;
}

static inline KlException klexec_method(KlState* state, KlValue* thisobj, KlValue* callable, size_t narg, size_t nret) {
  KlCallInfo* prevci = state->callinfo;
  KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
  klvalue_setvalue(&newci->env_this, thisobj);
  KlException exception = klexec_methodprepare(state, newci, callable, narg);
  if (exception) return exception;
  if (kl_likely(prevci != state->callinfo)) { /* is a klang call ? */
    state->callinfo->status |= KLSTATE_CI_STATUS_STOP;
    return klexec_execute(state);
  }
  return KL_E_NONE;
}

static inline KlException klexec_callopindex(KlState* state, KlValue* res, KlValue* indexable, KlValue* key) {
  ptrdiff_t resdiff = klexec_savestack(state, res);
  klstack_pushvalue(klstate_getstk(state), key);
  KlValue* method = klexec_getfield(state, indexable, state->common->string.index);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(state->common->string.index));
  }
  KlException exception = klexec_method(state, indexable, method, 1, 1);
  if (kl_unlikely(exception)) {
    klstack_move_top(klstate_getstk(state), -1);
    return exception;
  }
  klvalue_setvalue(klexec_restorestack(state, resdiff), klstate_stktop(state) - 1);
  klstack_move_top(klstate_getstk(state), -1);
  return KL_E_NONE;
}

static inline KlException klexec_callopindexas(KlState* state, KlValue* indexable, KlValue* key, KlValue* val) {
  klstack_pushvalue(klstate_getstk(state), key);
  klstack_pushvalue(klstate_getstk(state), val);
  KlValue* method = klexec_getfield(state, indexable, state->common->string.indexas);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(state->common->string.indexas));
  }
  KlException exception = klexec_method(state, indexable, method, 2, 0);
  klstack_move_top(klstate_getstk(state), -2);
  return exception;
}

static inline KlException klexec_callbinopmethod(KlState* state, KlString* op) {
  KlValue* l = klstate_getval(state, -2);
  KlValue* r = klstate_getval(state, -1);
  KlValue* method = klexec_getfield(state, l, op);
  if (!method) method = klexec_getfield(state, r, op);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(op));
  }
  return klexec_call(state, method, 2, 1);
}


#endif
