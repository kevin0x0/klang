#ifndef _KLANG_INCLUDE_VM_KLEXEC_H_
#define _KLANG_INCLUDE_VM_KLEXEC_H_

#include "include/vm/klexception.h"
#include "include/value/klcfunc.h"
#include "include/value/klcoroutine.h"
#include "include/value/klvalue.h"
#include "include/value/klstate.h"

#define KLEXEC_VARIABLE_RESULTS     (KLINST_VARRES)


#define klexec_savestack(state, stkptr)     ((stkptr) - klstack_raw(klstate_stack((state))))
#define klexec_restorestack(state, diff)    (klstack_raw(klstate_stack((state))) + (diff))
#define klexec_satisfy(val, cond)           ((klvalue_checktype((val), KL_BOOL) && klvalue_getbool((val)) == (cond)) || (!klvalue_checktype((val), KL_BOOL) && klvalue_checktype((val), KL_NIL) != (cond)))


typedef KlException (*KlCallPrepCallBack)(KlState* state, KlValue* callable, size_t narg);

KlException klexec_execute(KlState* state);
KlException klexec_call(KlState* state, KlValue* callable, size_t narg, size_t nret, KlValue* respos);
KlValue* klexec_getfield(KlState* state, KlValue* object, KlString* field);
const char* klexec_typename(KlState* state, KlValue* val);
KlException klexec_tailcall(KlState* state, KlValue* callable, size_t narg);

static inline void klexec_pop_callinfo(KlState* state);
static inline KlCallInfo* klexec_newed_callinfo(KlState* state);
static inline void klexec_setnils(KlValue* vals, size_t nnil);


static inline void klexec_pop_callinfo(KlState* state) {
  state->callinfo = state->callinfo->prev;
}

static inline void klexec_push_callinfo(KlState* state) {
  state->callinfo = state->callinfo->next;
}

static inline KlCallInfo* klexec_newed_callinfo(KlState* state) {
  return state->callinfo->next;
}

static inline void klexec_setnils(KlValue* vals, size_t nnil) {
  while (nnil--) klvalue_setnil(vals++);
}

#endif
