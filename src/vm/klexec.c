#include "include/vm/klexec.h"
#include "include/value/klmap.h"
#include "include/vm/klexception.h"
#include "include/vm/klstack.h"
#include "include/value/klarray.h"
#include "include/value/klcfunc.h"
#include "include/value/klclass.h"
#include "include/value/klclosure.h"
#include "include/value/klcoroutine.h"
#include "include/value/klkfunc.h"
#include "include/value/klstate.h"
#include "include/value/klvalue.h"
#include "include/lang/klinst.h"
#include "include/lang/klconvert.h"
#include "include/misc/klutils.h"


/* extra stack frame size for calling operator method */
#define KLEXEC_STACKFRAME_EXTRA             (4)

#define klexec_savestktop(state, stktop)    klstack_set_top(klstate_stack((state)), (stktop))
#define klexec_savepc(callinfo, pc)         ((callinfo)->savedpc = (pc))




static KlException klexec_callprepare(KlState* state, const KlValue* callable, size_t narg, KlCallPrepCallBack callback);
static inline KlCallInfo* klexec_new_callinfo(KlState* state, size_t nret, int retoff);
static KlCallInfo* klexec_alloc_callinfo(KlState* state);
static bool klexec_getmethod(const KlState* state, const KlValue* object, const KlString* field, KlValue* result);

static inline KlCallInfo* klexec_new_callinfo(KlState* state, size_t nret, int retoff) {
  KlCallInfo* callinfo = state->callinfo->next ? state->callinfo->next : klexec_alloc_callinfo(state);
  if (kl_unlikely(!callinfo)) return NULL;
  callinfo->nret = nret;
  callinfo->retoff = retoff;
  return callinfo;
}




static KlException klexec_handle_newshared_exception(KlState* state, KlException exception, const KlString* key) {
  if (exception == KL_E_OOM) {
    return klstate_throw(state, exception, "out of memory when setting a new field: %s", klstring_content(key));
  } else {
    kl_assert(exception == KL_E_INVLD, "");
    return klstate_throw(state, exception, "can not overwrite local field: %s", klstring_content(key)); 
  }

  kl_assert(false, "control flow should not reach here");
  return KL_E_NONE;
}

static KlException klexec_handle_newlocal_exception(KlState* state, KlException exception, const KlString* key) {
  if (exception == KL_E_OOM) {
    return klstate_throw(state, exception, "out of memory when adding a new field: %s", klstring_content(key));
  } else {
    kl_assert(exception == KL_E_INVLD, "");
    return klstate_throw(state, exception, "can not overwrite local field: %s", klstring_content(key)); 
  }

  kl_assert(false, "control flow should not reach here");
  return KL_E_NONE;
}

static KlException klexec_handle_newobject_exception(KlState* state, KlException exception) {
  if (exception == KL_E_OOM) {
    return klstate_throw_oom(state, "creating new object");
  } else if (exception == KL_E_INVLD) {
    return klstate_throw(state, exception, "this class can not have any user-created instance");
  } else {
    return klstate_throw(state, exception, "unknown exception");
  }
}

static KlException klexec_handle_arrayindexas_exception(KlState* state, KlException exception, const KlArray* arr, KlInt key) {
  if (exception == KL_E_RANGE) {
    return klstate_throw(state, exception,
                         "index out of range: index = %zd, array size = %zu.",
                         key, klarray_size(arr));
  }
  kl_assert(false, "control flow should not reach here");
  return KL_E_NONE;
}

static KlClass* klexec_getclass(const KlState* state, const KlValue* obj) {
  return klvalue_checktype(obj, KL_CLASS) ? klvalue_getobj(obj, KlClass*)                  :
         klvalue_dotable(obj)             ? klobject_class(klvalue_getobj(obj, KlObject*)) :
                                            klstate_common(state)->klclass.phony[klvalue_gettype(obj)];
}

static KlCallInfo* klexec_alloc_callinfo(KlState* state) {
  KlCallInfo* callinfo = (KlCallInfo*)klmm_alloc(klstate_getmm(state), sizeof (KlCallInfo));
  if (kl_unlikely(!callinfo)) return NULL;
  state->callinfo->next = callinfo;
  callinfo->prev = state->callinfo;
  callinfo->next = NULL;
  return callinfo;
}

static KlException klexec_co_startcall(KlState* state, const KlValue* callable, size_t narg, size_t nret) {
  kl_assert(narg != KLEXEC_VARIABLE_RESULTS, "arguments can not have variable number of results");

  KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
  if (kl_unlikely(!newci))
    return klstate_throw_oom(state, "calling a callable object");
  KlException exception = klexec_callprepare(state, callable, narg, NULL);
  if (newci == state->callinfo) {  /* to be executed klang call */
    state->callinfo->status |= KLSTATE_CI_STATUS_STOP;
    exception = klexec_execute(state);
  }
  return exception;
}

static KlException klexec_co_start(KlState* costate, KlState* caller, size_t narg, size_t nret) {
  kl_assert(klco_status(&costate->coinfo) == KLCO_NORMAL, "should be normal coroutine");
  kl_assert(narg != KLINST_VARRES && klstack_size(klstate_stack(caller)) >= narg, "not enough number of argument");
  kl_assert(costate->coinfo.kclo != NULL, "no executable");

  if (kl_unlikely(klstate_checkframe(costate, narg)))
    return klstate_throw_link(caller, costate);

  /* move arguments to execution environment of coroutine */
  KlValue* argbase = klstate_getval(caller, -narg);
  KlValue* costktop = klstate_stktop(costate);
  costate->coinfo.respos_save = klexec_savestack(costate, costktop);
  size_t count = narg;
  KlValue* argpos = argbase;
  while (count--)
    klvalue_setvalue(costktop++, argpos++);
  klstack_set_top(klstate_stack(costate), costktop);
  klstack_set_top(klstate_stack(caller), argbase);

  KlValue kclo;
  klvalue_setobj(&kclo, costate->coinfo.kclo, KL_KCLOSURE);
  klco_setstatus(&costate->coinfo, KLCO_RUNNING); /* now we run this coroutine */
  if (setjmp(costate->coinfo.yieldpos) == KLCOJMP_NORMAL) {
    KlException exception = klexec_co_startcall(costate, &kclo, narg, KLINST_VARRES);
    if (kl_unlikely(exception)) {
      klco_setstatus(&costate->coinfo, KLCO_DEAD);
      return klstate_throw_link(caller, costate);
    }
    KlValue* firstres = klexec_restorestack(costate, costate->coinfo.respos_save);
    size_t nres = (size_t)(klstate_stktop(costate) - firstres);
    if (nret == KLINST_VARRES) {
      size_t ncopy = nres;
      if (kl_unlikely(klstate_checkframe(caller, ncopy))) {
        klco_setstatus(&costate->coinfo, KLCO_DEAD);
        return klstate_currexception(caller);
      }
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      klstack_set_top(klstate_stack(caller), retpos);
    } else {
      size_t ncopy = nret < nres ? nret : nres;
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      if (nres < nret)  /* completing missing returned value */
        klexec_setnils(retpos, nret - nres);
    }
    klstack_set_top(klstate_stack(costate), klexec_restorestack(costate, costate->coinfo.respos_save));
    klco_setstatus(&costate->coinfo, KLCO_DEAD);
    return KL_E_NONE;
  } else {  /* yielded */
    size_t nres = costate->coinfo.nyield;
    KlValue* firstres = costate->coinfo.yieldvals;
    if (nret == KLINST_VARRES) {
      size_t ncopy = nres;
      if (kl_unlikely(klstate_checkframe(caller, ncopy))) {
        klco_setstatus(&costate->coinfo, KLCO_DEAD);
        return klstate_currexception(caller);
      }
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      klstack_set_top(klstate_stack(caller), retpos);
    } else {
      size_t ncopy = nret < nres ? nret : nres;
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      if (nres < nret)  /* completing missing returned value */
        klexec_setnils(retpos, nret - nres);
    }
    klstack_set_top(klstate_stack(costate), costate->coinfo.yieldvals);
    return KL_E_NONE;
  }
}

static KlException klexec_co_resume_execute(KlState* costate) {
  KlException exception;
  while (klstate_isrunning(costate)) {
    KlCallInfo* ci = klstate_currci(costate);
    if (ci->status & KLSTATE_CI_STATUS_KCLO) {
      exception = klexec_execute(costate);
      if (kl_unlikely(exception)) return exception;
    } else if (ci->status & KLSTATE_CI_STATUS_CFUN) {
      if (kl_unlikely((exception = ci->callable.cfunc(costate))))
        return exception;
      klexec_pop_callinfo(costate);
    } else if (ci->status & KLSTATE_CI_STATUS_CCLO) {
      if (kl_unlikely((exception = klcast(KlCClosure*, ci->callable.clo)->cfunc(costate))))
        return exception;
      klexec_pop_callinfo(costate);
    }
  }
  return KL_E_NONE;
}

static KlException klexec_co_resume(KlState* costate, KlState* caller, size_t narg, size_t nret) {
  kl_assert(klco_status(&costate->coinfo) == KLCO_BLOCKED, "should be blocked coroutine");
  kl_assert(klstack_size(klstate_stack(caller)) >= narg, "not enough number of argument");
  kl_assert(costate->coinfo.kclo != NULL, "no executable");

  size_t nwanted = costate->coinfo.nwanted;
  if (nwanted == KLEXEC_VARIABLE_RESULTS)
    nwanted = narg;

  if (kl_unlikely(klstate_checkframe(costate, nwanted))) {
    klco_setstatus(&costate->coinfo, KLCO_DEAD);
    return klstate_throw_link(caller, costate);
  }

  /* move arguments to execution environment of coroutine */
  KlValue* argbase = klstate_getval(caller, -narg);
  KlValue* costktop = klstate_stktop(costate);
  size_t ncopy = narg < nwanted ? narg : nwanted;
  KlValue* argpos = argbase;
  while (ncopy--)
    klvalue_setvalue(costktop++, argpos++);
  if (narg < nwanted)
    klexec_setnils(costktop, nwanted - narg);
  klstack_set_top(klstate_stack(costate), klstate_stktop(costate) + nwanted);
  klstack_set_top(klstate_stack(caller), argbase);

  klco_setstatus(&costate->coinfo, KLCO_RUNNING);
  if (setjmp(costate->coinfo.yieldpos) == KLCOJMP_NORMAL) {
    KlException exception = klexec_co_resume_execute(costate);
    if (kl_unlikely(exception)) {
      klco_setstatus(&costate->coinfo, KLCO_DEAD);
      return klstate_throw_link(caller, costate);
    }
    KlValue* firstres = klexec_restorestack(costate, costate->coinfo.respos_save);
    size_t nres = (size_t)(klstate_stktop(costate) - firstres);
    if (nret == KLINST_VARRES) {
      size_t ncopy = nres;
      if (kl_unlikely(klstate_checkframe(caller, ncopy))) {
        klco_setstatus(&costate->coinfo, KLCO_DEAD);
        return klstate_currexception(caller);
      }
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      klstack_set_top(klstate_stack(caller), retpos);
    } else {
      size_t ncopy = nret < nres ? nret : nres;
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      if (nres < nret)  /* completing missing returned value */
        klexec_setnils(retpos, nret - nres);
    }
    klstack_set_top(klstate_stack(costate), klexec_restorestack(costate, costate->coinfo.respos_save));
    klco_setstatus(&costate->coinfo, KLCO_DEAD);
    return KL_E_NONE;
  } else {  /* yielded */
    size_t nres = costate->coinfo.nyield;
    KlValue* firstres = costate->coinfo.yieldvals;
    if (nret == KLINST_VARRES) {
      size_t ncopy = nres;
      if (kl_unlikely(klstate_checkframe(caller, ncopy))) {
        klco_setstatus(&costate->coinfo, KLCO_DEAD);
        return klstate_throw(costate, KL_E_OOM, "out of memory when grow stack");
      }
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      klstack_set_top(klstate_stack(caller), retpos);
    } else {
      size_t ncopy = nret < nres ? nret : nres;
      KlValue* retpos = klstate_getval(caller, klexec_newed_callinfo(caller)->retoff);
      while (ncopy--)   /* copy results */
        klvalue_setvalue(retpos++, firstres++);
      if (nres < nret)  /* completing missing returned value */
        klexec_setnils(retpos, nret - nres);
    }
    klstack_set_top(klstate_stack(costate), costate->coinfo.yieldvals);
    return KL_E_NONE;
  }
}

static KlException klexec_co_call(KlState* costate, KlState* caller, size_t narg, size_t nret) {
  if (kl_unlikely(costate->coinfo.kclo == NULL))
    return klstate_throw(caller, KL_E_INVLD, "invalid coroutine");
  if (kl_unlikely(klco_status(&costate->coinfo) == KLCO_DEAD || klco_status(&costate->coinfo) == KLCO_RUNNING))
    return klstate_throw(caller, KL_E_INVLD, "can not call a dead or running coroutine");
  KlException exception;
  if (kl_likely(klco_status(&costate->coinfo) == KLCO_BLOCKED)) {
    exception = klexec_co_resume(costate, caller, narg, nret);
  } else {  /* else is KLCO_NORMAL */
    kl_assert(klco_status(&costate->coinfo) == KLCO_NORMAL, "should be normal coroutine");
    exception = klexec_co_start(costate, caller, narg, nret);
  }
  if (kl_unlikely(exception))
    return klstate_throw_link(caller, costate);
  return KL_E_NONE;
}

KlException klexec_call(KlState* state, const KlValue* callable, size_t narg, size_t nret, KlValue* respos) {
  kl_assert(nret <= KLEXEC_VARIABLE_RESULTS, "number of returned values should be in range [0, 255) or KLAPI_VARIABLE_RESULTS");
  kl_assert(klstack_onstack(klstate_stack(state), respos), "'respos' should be a position on stack");

  KlCallInfo* newci = klexec_new_callinfo(state, nret, respos - (klstate_stktop(state) - narg));
  if (kl_unlikely(!newci))
    return klstate_throw_oom(state, "calling a callable object");
  bool yieldallowance_save = klco_yield_allowed(&state->coinfo);
  klco_allow_yield(&state->coinfo, false);
  KlException exception = klexec_callprepare(state, callable, narg, NULL);
  if (newci == state->callinfo) {  /* to be executed klang call */
    state->callinfo->status |= KLSTATE_CI_STATUS_STOP;
    exception = klexec_execute(state);
  }
  klco_allow_yield(&state->coinfo, yieldallowance_save);
  return exception;
}

KlException klexec_tailcall(KlState* state, const KlValue* callable, size_t narg) {
  KlCallInfo* newci = state->callinfo;
  newci->retoff += newci->base - (klstate_stktop(state) - narg);
  klexec_pop_callinfo(state);
  KlException exception = klexec_callprepare(state, callable, narg, NULL);
  if (newci == state->callinfo) {  /* to be executed klang call */
    state->callinfo->status |= KLSTATE_CI_STATUS_STOP;
    exception = klexec_execute(state);
  }
  if (kl_unlikely(exception)) return exception;
  klexec_push_callinfo(state);
  return KL_E_NONE;
}

/* Prepare for calling a callable object (C function, C closure, klang closure).
 * Also perform the actual call for C function and C closure.
 */
static KlException klexec_callprepare(KlState* state, const KlValue* callable, size_t narg, KlCallPrepCallBack callback) {
  if (kl_likely(klvalue_checktype(callable, KL_KCLOSURE))) {          /* is a klang closure ? */
    /* get closure in advance, in case of dangling pointer due to stack grow */
    KlKClosure* kclo = klvalue_getobj(callable, KlKClosure*);
    KlKFunction* kfunc = kclo->kfunc;
    size_t framesize = klkfunc_framesize(kfunc);
    /* ensure enough stack size for this call. */
    if (kl_unlikely(klstate_checkframe(state, framesize + KLEXEC_STACKFRAME_EXTRA)))
      return klstate_currexception(state);
    size_t nparam = klkfunc_nparam(kfunc);
    if (narg < nparam) {  /* complete missing arguments */
      klstack_pushnil(klstate_stack(state), nparam - narg);
      narg = nparam;
    }
    /* fill callinfo, callinfo->narg is not necessary for klang closure. */
    KlCallInfo* callinfo = klexec_newed_callinfo(state);
    callinfo->callable.clo = klmm_to_gcobj(kclo);
    callinfo->status = KLSTATE_CI_STATUS_KCLO;
    callinfo->savedpc = klkfunc_entrypoint(kfunc);
    KlValue* base = klstate_stktop(state) - narg;
    callinfo->top = base + framesize;
    callinfo->base = base;
    klexec_push_callinfo(state);
    return KL_E_NONE;
  } else if (kl_likely(klvalue_checktype(callable, KL_COROUTINE))) {  /* is a coroutine ? */
    KlCallInfo* callinfo = klexec_newed_callinfo(state);
    return klexec_co_call(klvalue_getobj(callable, KlState*), state, narg, callinfo->nret);
  } else if (kl_likely(klvalue_checktype(callable, KL_CFUNCTION))) {  /* is a C function ? */
    KlCFunction* cfunc = klvalue_getcfunc(callable);
    KlCallInfo* callinfo = klexec_newed_callinfo(state);
    callinfo->callable.cfunc = cfunc;
    callinfo->status = KLSTATE_CI_STATUS_CFUN;
    callinfo->narg = narg;
    callinfo->base = klstate_stktop(state) - narg;
    klexec_push_callinfo(state);
    /* do the call */
    KlException exception = cfunc(state);
    klexec_pop_callinfo(state);
    return exception;
  } else if (kl_likely(klvalue_checktype(callable, KL_CCLOSURE))) {   /* is a C closure ? */
    KlCClosure* cclo = klvalue_getobj(callable, KlCClosure*);
    KlCallInfo* callinfo = klexec_newed_callinfo(state);
    callinfo->callable.clo = klmm_to_gcobj(cclo);
    callinfo->status = KLSTATE_CI_STATUS_CCLO;
    callinfo->narg = narg;
    callinfo->base = klstate_stktop(state) - narg;
    klexec_push_callinfo(state);
    /* do the call */
    KlException exception = cclo->cfunc(state);
    klexec_pop_callinfo(state);
    return exception;
  } else {
    if (callback)
      return callback(state, callable, narg);
    return klstate_throw(state, KL_E_TYPE,
           "try to call a non-callable object with type '%s'(method '%s' can not be called from here)",
           klexec_typename(state, callable), klstring_content(state->common->string.call));
  }
}

static KlException klexec_callprep_callback_for_call(KlState* state, const KlValue* callable, size_t narg) {
  KlCallInfo* callinfo = klexec_newed_callinfo(state);
  KlValue method;
  bool ismethod = klexec_getmethod(state, callable, state->common->string.call, &method);
  if (ismethod) {
    ++narg;
    klvalue_setvalue(klstate_getval(state, -narg), callable);
    kl_assert(callinfo->retoff == -1, "");
    callinfo->retoff = 0;
  }
  return klexec_callprepare(state, &method, narg, NULL);
}

static KlException klexec_callprep_callback_for_method(KlState* state, const KlValue* callable, size_t narg) {
  KlCallInfo* callinfo = klexec_newed_callinfo(state);
  KlValue method;
  bool ismethod = klexec_getmethod(state, callable, state->common->string.call, &method);
  if (ismethod) {
    ++narg;
    klvalue_setvalue(klstate_getval(state, -narg), callable);
    callinfo->retoff = 0;
  }
  return klexec_callprepare(state, &method, narg, NULL);
}

static const KlValue* klexec_getfield(const KlState* state, const KlValue* object, const KlString* field) {
  static const KlValue nil = KLVALUE_NIL_INIT;
  if (klvalue_dotable(object)) {
    KlObject* obj = klvalue_getobj(object, KlObject*);
    KlValue* val = klobject_getfield(obj, field);
    return val ? val : &nil;
  } else {
    KlClass* phony = klvalue_checktype(object, KL_CLASS)
                   ? klvalue_getobj(object, KlClass*)
                   : state->common->klclass.phony[klvalue_gettype(object)];
    /* phony class should have only shared field */
    KlClassSlot* slot = klclass_find(phony, field);
    return slot && klclass_is_shared(slot) ? &slot->value : &nil;
  }
}

static bool klexec_getmethod(const KlState* state, const KlValue* object, const KlString* field, KlValue* result) {
  switch (klvalue_gettype(object)) {
    case KL_OBJECT:
    case KL_ARRAY: {
      return klobject_getmethod(klvalue_getobj(object, KlObject*), field, result);
    }
    case KL_CLASS: {
      KlClass* klclass = klvalue_getobj(object, KlClass*);
      KlClassSlot* slot = klclass_find(klclass, field);
      slot && klclass_is_shared(slot) ? klvalue_setvalue(result, &slot->value) : klvalue_setnil(result);
      return false;
    }
    default: {
      KlClass* klclass = state->common->klclass.phony[klvalue_gettype(object)];
      KlClassSlot* slot = klclass_find(klclass, field);
      if (slot && klclass_is_shared(slot)) {
        klvalue_setvalue(result, &slot->value);
        return klvalue_gettag(&slot->value) & KLCLASS_TAG_METHOD;
      } else {
        klvalue_setnil(result);
        return false;
      }
    }
  }
}

const char* klexec_typename(const KlState* state, const KlValue* val) {
  if (klvalue_checktype(val, KL_OBJECT) ||
      klvalue_checktype(val, KL_ARRAY)) {
    const KlValue* typename = klexec_getfield(state, val, state->common->string.typename);
    return klvalue_checktype(typename, KL_STRING)                ? 
           klstring_content(klvalue_getobj(typename, KlString*)) :
           klvalue_typename(klvalue_gettype(val));
  } else {
    return klvalue_typename(klvalue_gettype(val));
  }
}

static KlException klexec_dopreopmethod(KlState* state, const KlValue* a, const KlValue* b, const KlString* op) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_onstack(klstate_stack(state), a), "'a' must be on stack");
  kl_assert(klstack_residual(klstate_stack(state)) >= 1, "stack size not enough for calling prefix operator");

  const KlValue* method = klexec_getfield(state, b, state->common->string.call);
  if (kl_unlikely(klvalue_checktype(method, KL_NIL) || !klvalue_callable(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply prefix '%s' to value with type '%s'",
                         klstring_content(op), klexec_typename(state, b));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 1, a - klstate_stktop(state));
  if (kl_unlikely(!newci))
    return klstate_throw_oom(state, "calling a prefix operator method");
  klstack_pushvalue(klstate_stack(state), b);
  return klexec_callprepare(state, method, 1, NULL);
}

static KlException klexec_dobinopmethod(KlState* state, KlValue* a, const KlValue* b, const KlValue* c, const KlString* op) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_onstack(klstate_stack(state), a), "'a' must be on stack");
  kl_assert(klstack_residual(klstate_stack(state)) >= 2, "stack size not enough for calling binary operator");

  const KlValue* method = klexec_getfield(state, b, op);
  if (kl_unlikely(klvalue_checktype(method, KL_NIL) || !klvalue_callable(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply binary '%s' to values with type '%s' and '%s'",
                         klstring_content(op), klexec_typename(state, b), klexec_typename(state, c));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 1, a - klstate_stktop(state));
  if (kl_unlikely(!newci))
    return klstate_throw_oom(state, "calling a binary operator method");
  klstack_pushvalue(klstate_stack(state), b);
  klstack_pushvalue(klstate_stack(state), c);
  return klexec_callprepare(state, method, 2, NULL);
}

static KlException klexec_doindexmethod(KlState* state, KlValue* val, const KlValue* indexable, const KlValue* key) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_onstack(klstate_stack(state), val), "'a' must be on stack");
  kl_assert(klstack_residual(klstate_stack(state)) >= 1, "stack size not enough for calling binary operator");

  KlString* index = state->common->string.index;
  const KlValue* method = klexec_getfield(state, indexable, index);
  if (kl_unlikely(klvalue_checktype(method, KL_NIL) || !klvalue_callable(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply '%s' to values with type '%s' for key with type '%s'",
                         klstring_content(index), klexec_typename(state, indexable), klexec_typename(state, key));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 1, val - klstate_stktop(state));
  if (kl_unlikely(!newci))
    return klstate_throw_oom(state, "calling index operator method");
  klstack_pushvalue(klstate_stack(state), indexable);
  klstack_pushvalue(klstate_stack(state), key);
  return klexec_callprepare(state, method, 2, NULL);
}

static KlException klexec_doindexasmethod(KlState* state, const KlValue* indexable, const KlValue* key, const KlValue* val) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_residual(klstate_stack(state)) >= 2, "stack size not enough for calling indexas operator");

  KlString* indexas = state->common->string.indexas;
  const KlValue* method = klexec_getfield(state, indexable, indexas);
  if (kl_unlikely(klvalue_checktype(method, KL_NIL) || !klvalue_callable(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply '%s' to values with type '%s' for key with type '%s'",
                         klstring_content(indexas), klexec_typename(state, indexable), klexec_typename(state, key));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 0, 0);
  if (kl_unlikely(!newci))
    return klstate_throw_oom(state, "calling indexas operator method");
  klstack_pushvalue(klstate_stack(state), indexable);
  klstack_pushvalue(klstate_stack(state), key);
  klstack_pushvalue(klstate_stack(state), val);
  return klexec_callprepare(state, method, 3, NULL);
}

static KlException klexec_dolt(KlState* state, KlValue* a, const KlValue* b, const KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res < 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.lt);
}

static KlException klexec_dole(KlState* state, KlValue* a, const KlValue* b, const KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res <= 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.le);
}

static KlException klexec_dogt(KlState* state, KlValue* a, const KlValue* b, const KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res > 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.gt);
}

static KlException klexec_doge(KlState* state, KlValue* a, const KlValue* b, const KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res >= 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.lt);
}


static KlString* klexec_doconcat(KlState* state, const KlValue* b, const KlValue* c) {
#define KLEXEC_STRING_BUFSIZE (100)
  char buf1[KLEXEC_STRING_BUFSIZE];
  char buf2[KLEXEC_STRING_BUFSIZE];
  const char* p1;
  const char* p2;
  if (klvalue_checktype(b, KL_INT)) {
   kllang_int2str(buf1, KLEXEC_STRING_BUFSIZE, klvalue_getint(b));
   p1 = buf1;
  } else if (klvalue_checktype(b, KL_FLOAT)) {
   kllang_float2str(buf1, KLEXEC_STRING_BUFSIZE, klvalue_getfloat(b));
   p1 = buf1;
  } else {
    p1 = klstring_content(klvalue_getobj(b, KlString*));
  }
  if (klvalue_checktype(c, KL_INT)) {
   kllang_int2str(buf2, KLEXEC_STRING_BUFSIZE, klvalue_getint(c));
   p2 = buf2;
  } else if (klvalue_checktype(c, KL_FLOAT)) {
   kllang_float2str(buf2, KLEXEC_STRING_BUFSIZE, klvalue_getfloat(c));
   p2 = buf2;
  } else {
    p2 = klstring_content(klvalue_getobj(c, KlString*));
  }
  return klstrpool_string_concat_cstyle(state->strpool, p1, p2);
#undef KLEXEC_STRING_BUFSIZE
}

static KlException klexec_iforprep(KlState* state, KlValue* ctrlvars, int offset) {
  if (kl_unlikely(!klvalue_checktype(ctrlvars, KL_INT)      ||
                  !klvalue_checktype(ctrlvars + 1, KL_INT)  ||
                  !(klvalue_checktype(ctrlvars + 2, KL_INT) ||
                    klvalue_checktype(ctrlvars + 2, KL_NIL)))) {
    return klstate_throw(state, KL_E_TYPE, "expected integer for integer loop, got %s, %s, %s",
                         klexec_typename(state, ctrlvars),
                         klexec_typename(state, ctrlvars + 1),
                         klexec_typename(state, ctrlvars + 2));
  }
  KlInt begin = klvalue_getint(ctrlvars);
  KlInt end = klvalue_getint(ctrlvars + 1);
  if (klvalue_checktype(ctrlvars + 2, KL_NIL)) {
    klvalue_setint(ctrlvars + 2, begin > end ? -1 : 1);
  }
  KlInt step = klvalue_getint(ctrlvars + 2);
  if (begin < end) {
    if (step <= 0)
      return klstate_throw(state, KL_E_INVLD, "for loop: infinite loop: begin = %zd, end = %zd, step = %zd", begin, end, step);
    end = begin + ((end - begin + step - 1) / step) * step;
    klvalue_setint(ctrlvars + 1, end);
  } else if (begin > end) {
    if (step >= 0)
      return klstate_throw(state, KL_E_INVLD, "for loop: infinite loop: begin = %zd, end = %zd, step = %zd", begin, end, step);
    end = begin - ((begin - end - step - 1) / -step) * (-step);
    klvalue_setint(ctrlvars + 1, end);
  } else {
    state->callinfo->savedpc += offset;
  }
  return KL_E_NONE;
}

static void klexec_getfieldgeneric(KlState* state, const KlValue* dotable, const KlValue* key, KlValue* val) {
  /* this is ensured by klang compiler */
  kl_assert(klvalue_checktype(key, KL_STRING), "expected string to index field");

  KlString* keystr = klvalue_getobj(key, KlString*);
  if (klvalue_dotable(dotable)) {
    /* values with type KL_OBJECT(including map and array). */
    KlObject* object = klvalue_getobj(dotable, KlObject*);
    klobject_getfieldset(object, keystr, val);
  } else {  /* other types. search their phony class */
    KlClass* phony = klvalue_checktype(dotable, KL_CLASS)
                   ? klvalue_getobj(dotable, KlClass*)
                   : state->common->klclass.phony[klvalue_gettype(dotable)];
    /* phony class should have only shared field */
    KlClassSlot* slot = klclass_find(phony, keystr);
    slot && klclass_is_shared(slot) ? klvalue_setvalue(val, &slot->value) : klvalue_setnil(val);
  }
}

static KlException klexec_setfieldgeneric(KlState* state, const KlValue* dotable, const KlValue* key, KlValue* val) {
  /* this is ensured by klang compiler */
  kl_assert(klvalue_checktype(key, KL_STRING), "expected string to index field");

  KlString* keystr = klvalue_getobj(key, KlString*);
  switch (klvalue_gettype(dotable)) {
    case KL_NIL: {
      return klstate_throw(state, KL_E_INVLD, "can not set field of nil class");
    }
    case KL_OBJECT: {
      /* values with type KL_OBJECT(including map and array). */
      KlObject* object = klvalue_getobj(dotable, KlObject*);
      if (!klobject_setfield(object, keystr, val)) {
        klexec_savestktop(state, state->callinfo->top);
        KlClass* klclass = klobject_class(object);
        KlClassSlot* newslot = klclass_add(klclass, klstate_getmm(state), keystr);
        if (kl_unlikely(!newslot))
          return klstate_throw_oom(state, "adding new field");
        klvalue_setvalue(&newslot->value, val);
        klvalue_settag(&newslot->value, KLCLASS_TAG_NORMAL);
      }
      return KL_E_NONE;
    } 
    case KL_CLASS: {
      KlClass* klclass = klvalue_getobj(dotable, KlClass*);
      klexec_savestktop(state, state->callinfo->top);
      KlException exception = klclass_newshared_normal(klclass, klstate_getmm(state), keystr, val);
      if (kl_unlikely(exception))
        return klexec_handle_newshared_exception(state, exception, keystr);
      return KL_E_NONE;
    }
    default: {
      KlClass* klclass = state->common->klclass.phony[klvalue_gettype(dotable)];
      klexec_savestktop(state, state->callinfo->top);
      KlException exception = klclass_newshared_normal(klclass, klstate_getmm(state), keystr, val);
      if (kl_unlikely(exception))
        return klexec_handle_newshared_exception(state, exception, keystr);
      return KL_E_NONE;
    }
  }
  // KlString* keystr = klvalue_getobj(key, KlString*);
  // if (klvalue_dotable(dotable)) {
  //   /* values with type KL_OBJECT(including map and array). */
  //   KlObject* object = klvalue_getobj(dotable, KlObject*);
  //   if (!klobject_setfield(object, keystr, val)) {
  //     klexec_savestktop(state, state->callinfo->top);
  //     KlClass* klclass = klobject_class(object);
  //     KlClassSlot* newslot = klclass_add(klclass, klstate_getmm(state), keystr);
  //     if (kl_unlikely(!newslot))
  //       return klstate_throw_oom(state, "adding new field");
  //     klvalue_setvalue(&newslot->value, val);
  //     klvalue_settag(&newslot->value, KLCLASS_TAG_NORMAL);
  //   }
  // } else {  /* other types. search their phony class */
  //   KlClass* phony;
  //   if (klvalue_checktype(dotable, KL_CLASS)) {
  //     phony = klvalue_getobj(dotable, KlClass*);
  //   } else if (kl_likely(!klvalue_checktype(dotable, KL_NIL))) {
  //     phony = state->common->klclass.phony[klvalue_gettype(dotable)];
  //   } else {
  //     return klstate_throw(state, KL_E_INVLD, "can not set field of nil class");
  //   }
  //   klexec_savestktop(state, state->callinfo->top);
  //   KlException exception = klclass_newshared(phony, klstate_getmm(state), keystr, val);
  //   if (kl_unlikely(exception))
  //     return klexec_handle_newshared_exception(state, exception, keystr);
  // }
  // return KL_E_NONE;
}


/* macros for klstate_execute() */

#define klop_add(a, b)              ((a) + (b))
#define klop_sub(a, b)              ((a) - (b))
#define klop_mul(a, b)              ((a) * (b))
#define klop_div(a, b)              ((a) / (b))
#define klop_idiv(a, b)             ((a) / (b))
#define klop_mod(a, b)              ((a) % (b))

#define klorder_lt(a, b)            ((a) < (b))
#define klorder_gt(a, b)            ((a) > (b))
#define klorder_le(a, b)            ((a) <= (b))
#define klorder_ge(a, b)            ((a) >= (b))

#define klorder_strlt(a, b)         (klstring_compare((a), (b)) < 0)
#define klorder_strgt(a, b)         (klstring_compare((a), (b)) > 0)
#define klorder_strle(a, b)         (klstring_compare((a), (b)) <= 0)
#define klorder_strge(a, b)         (klstring_compare((a), (b)) >= 0)


/* update global status */
#define klexec_updateglobal(newbase) {                                            \
  callinfo = state->callinfo;                                                     \
  kl_assert(state->callinfo->status & KLSTATE_CI_STATUS_KCLO,                     \
            "expected klang closure in klexec_execute()");                        \
  closure = klcast(KlKClosure*, callinfo->callable.clo);                          \
  pc = callinfo->savedpc;                                                         \
  constants = klkfunc_constants(closure->kfunc);                                  \
  stkbase = (newbase);                                                            \
}

#define klexec_savetop(top)   klstack_set_top(klstate_stack(state), (top))

#define klexec_savestate(top, pc) {                                               \
  klexec_savetop((top));                                                          \
  klexec_savepc(callinfo, (pc));                                                  \
}


#define klexec_binop(op, a, b, c) {                                               \
  if (kl_likely(klvalue_bothinteger((b), (c)))) {                                 \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), klvalue_getint((c))));     \
  } else if (kl_likely(klvalue_bothnumber((b), (c)))) {                           \
    klvalue_setfloat((a), klop_##op(klvalue_getnumber((b)),                       \
                                    klvalue_getnumber((c))));                     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), (c), opname);   \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_bindiv(op, a, b, c) {                                              \
  if (kl_likely(klvalue_bothnumber((b), (c)))) {                                  \
    klvalue_setfloat((a), klop_##op(klvalue_getnumber((b)),                       \
                                    klvalue_getnumber((c))));                     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), (c), opname);   \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binidiv(op, a, b, c) {                                             \
  if (kl_likely(klvalue_bothinteger((b), (c)))) {                                 \
    KlInt cval = klvalue_getint((c));                                             \
    if (kl_unlikely(cval == 0)) {                                                 \
      klexec_savestate(callinfo->top, pc);                                        \
      return klstate_throw(state, KL_E_ZERO, "divided by zero");                  \
    }                                                                             \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), cval));                    \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), (c), opname);   \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binmod(op, a, b, c) {                                              \
  if (kl_likely(klvalue_bothinteger((b), (c)))) {                                 \
    KlInt cval = klvalue_getint((c));                                             \
    if (kl_unlikely(cval == 0)) {                                                 \
      klexec_savestate(callinfo->top, pc);                                        \
      return klstate_throw(state, KL_E_ZERO, "divided by zero");                  \
    }                                                                             \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), cval));                    \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), (c), opname);   \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binop_i(op, a, b, imm) {                                           \
  if (kl_likely(klvalue_checktype((b), KL_INT))) {                                \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), (imm)));                   \
  } else if (kl_likely(klvalue_checktype((b), KL_FLOAT))) {                       \
    klvalue_setfloat((a), klop_##op(klvalue_getfloat((b)), (KlFloat)(imm)));      \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlValue tmp;                                                                  \
    klvalue_setint(&tmp, imm);                                                    \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), &tmp, opname);  \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binidiv_i(op, a, b, imm) {                                         \
  if (kl_likely(klvalue_checktype((b), KL_INT))) {                                \
    if (kl_unlikely((imm) == 0)) {                                                \
      klexec_savestate(callinfo->top, pc);                                        \
      return klstate_throw(state, KL_E_ZERO, "divided by zero");                  \
    }                                                                             \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), imm));                     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlValue tmp;                                                                  \
    klvalue_setint(&tmp, (imm));                                                  \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), &tmp, opname);  \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_bindiv_i(op, a, b, imm) {                                          \
  if (kl_likely(klvalue_isnumber((b)))) {                                         \
    klvalue_setfloat((a), klop_##op(klvalue_getnumber((b)),                       \
                                    klcast(KlFloat, (imm))));                     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlValue tmp;                                                                  \
    klvalue_setint(&tmp, (imm));                                                  \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), &tmp, opname);  \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binmod_i(op, a, b, imm) {                                          \
  if (kl_likely(klvalue_checktype((b), KL_INT))) {                                \
    if (kl_unlikely((imm) == 0)) {                                                \
      klexec_savestate(callinfo->top, pc);                                        \
      return klstate_throw(state, KL_E_ZERO, "divided by zero");                  \
    }                                                                             \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), (imm)));                   \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc);  /* in case of error and gc */           \
    KlValue tmp;                                                                  \
    klvalue_setint(&tmp, (imm));                                                  \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), &tmp, opname);  \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_order(order, a, b, offset, cond) {                                 \
  if (kl_likely(klvalue_bothinteger((a), (b)))) {                                 \
    if (klorder_##order(klvalue_getint((a)), klvalue_getint((b))) == cond)        \
      pc += offset;                                                               \
  } else if (kl_likely(klvalue_bothnumber((a), (b)))) {                           \
    if (klorder_##order(klvalue_getnumber((a)), klvalue_getnumber(b)) == cond)    \
      pc += offset;                                                               \
  } else {                                                                        \
    klexec_savestate(callinfo->top, pc - 1);                                      \
    KlValue* respos = callinfo->top;                                              \
    KlException exception = klexec_do##order(state, respos, (a), (b));            \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
      KlValue* testval = callinfo->top;                                           \
      if (klexec_satisfy(testval, cond)) pc += offset;                            \
    }                                                                             \
  }                                                                               \
}

#define klexec_orderi(order, a, imm, offset, cond) {                              \
  if (kl_likely(klvalue_checktype((a), KL_INT))) {                                \
    if (klorder_##order(klvalue_getint((a)), (imm)) == cond)                      \
      pc += offset;                                                               \
  } else if (kl_likely(klvalue_checktype((a), KL_FLOAT))) {                       \
    if (klorder_##order(klvalue_getfloat((a)), (KlFloat)(imm)) == cond)           \
      pc += offset;                                                               \
  } else {                                                                        \
    KlValue tmpval;                                                               \
    klvalue_setint(&tmpval, imm);                                                 \
    klexec_savestate(callinfo->top, pc - 1);                                      \
    KlValue* respos = callinfo->top;                                              \
    KlException exception = klexec_do##order(state, respos, (a), &tmpval);        \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {                                                                      \
      if (kl_unlikely(exception)) return exception;                               \
      /* C function or C closure */                                               \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
      KlValue* testval = callinfo->top;                                           \
      if (klexec_satisfy(testval, cond)) pc += offset;                            \
    }                                                                             \
  }                                                                               \
}

#define klexec_bequal(a, b, offset, cond) {                                       \
  if (kl_likely(klvalue_sametype((a), (b)))) {                                    \
    if (klvalue_sameinstance((a), (b))) {                                         \
      if (cond) pc += offset;                                                     \
    } else if (!klvalue_canrawequal((a))) {                                       \
      klexec_savestate(callinfo->top, pc - 1);                                    \
      KlValue* respos = callinfo->top;                                            \
      KlString* op = state->common->string.eq;                                    \
      KlException exception = klexec_dobinopmethod(state, respos, (a), (b), op);  \
      if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */       \
        KlValue* newbase = state->callinfo->base;                                 \
        klexec_updateglobal(newbase);                                             \
      } else {                                                                    \
        if (kl_unlikely(exception)) return exception;                             \
        /* C function or C closure */                                             \
        /* stack may have grown. restore stkbase. */                              \
        stkbase = callinfo->base;                                                 \
        KlValue* testval = callinfo->top;                                         \
        if (klexec_satisfy(testval, cond)) pc += offset;                          \
      }                                                                           \
    } else {                                                                      \
      if (!cond) pc += offset;                                                    \
    }                                                                             \
  } else if (klvalue_bothnumber((a), (b))) {                                      \
    KlFloat af = klvalue_getnumber((a));                                          \
    KlFloat bf = klvalue_getnumber((b));                                          \
    if ((af == bf) == cond) pc += offset;                                         \
  } else {                                                                        \
    if (!cond) pc += offset;                                                      \
  }                                                                               \
}

#define klexec_bnequal(a, b, offset, cond) {                                      \
  if (kl_likely(klvalue_sametype((a), (b)))) {                                    \
    if (klvalue_sameinstance((a), (b))) {                                         \
      if (!cond) pc += offset;                                                    \
    } else if (klvalue_canrawequal((a))) {                                        \
      if (cond) pc += offset;                                                     \
    } else {                                                                      \
      klexec_savestate(callinfo->top, pc - 1);                                    \
      KlValue* respos = callinfo->top;                                            \
      KlString* op = state->common->string.neq;                                   \
      KlException exception = klexec_dobinopmethod(state, respos, (a), (b), op);  \
      if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */       \
        KlValue* newbase = state->callinfo->base;                                 \
        klexec_updateglobal(newbase);                                             \
      } else {                                                                    \
        if (kl_unlikely(exception)) return exception;                             \
        /* C function or C closure */                                             \
        /* stack may have grown. restore stkbase. */                              \
        stkbase = callinfo->base;                                                 \
        KlValue* testval = callinfo->top;                                         \
        if (klexec_satisfy(testval, cond)) pc += offset;                          \
      }                                                                           \
    }                                                                             \
  } else if (klvalue_bothnumber((a), (b))) {                                      \
    KlFloat af = klvalue_getnumber((a));                                          \
    KlFloat bf = klvalue_getnumber((b));                                          \
    if ((af != bf) == cond) pc += offset;                                         \
  } else {                                                                        \
    if (cond) pc += offset;                                                       \
  }                                                                               \
}

KlException klexec_execute(KlState* state) {
  kl_assert(state->callinfo->status & KLSTATE_CI_STATUS_KCLO, "expected klang closure in klexec_execute()");

  KlCallInfo* callinfo = state->callinfo;
  KlKClosure* closure = klcast(KlKClosure*, callinfo->callable.clo);
  const KlInstruction* pc = callinfo->savedpc;
  const KlValue* constants = klkfunc_constants(klcast(KlKClosure*, callinfo->callable.clo)->kfunc);
  KlValue* stkbase = callinfo->base;
  kl_assert(klcast(size_t, klexec_savestack(state, callinfo->top)) <= klstack_capacity(klstate_stack(state)), "stack size not enough");

  while (true) {
    KlInstruction inst = *pc++;
    KlOpcode opcode = KLINST_GET_OPCODE(inst);
    switch (opcode) {
      case KLOPCODE_MOVE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        klvalue_setvalue(a, b);
        break;
      }
      case KLOPCODE_MULTIMOVE: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        size_t nmove = KLINST_ABX_GETX(inst);
        if (nmove == KLEXEC_VARIABLE_RESULTS) {
          nmove = klstate_stktop(state) - b;
          klstack_set_top(klstate_stack(state), a + nmove);
        }
        if (a <= b) {
          while (nmove--)
            klvalue_setvalue(a++, b++);
        } else {
          a += nmove;
          b += nmove;
          while (nmove--)
            klvalue_setvalue(--a, --b);
        }
        break;
      }
      case KLOPCODE_ADD: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binop(add, a, b, c);
        break;
      }
      case KLOPCODE_SUB: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binop(sub, a, b, c);
        break;
      }
      case KLOPCODE_MUL: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binop(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIV: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_bindiv(div, a, b, c);
        break;
      }
      case KLOPCODE_MOD: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binmod(mod, a, b, c);
        break;
      }
      case KLOPCODE_IDIV: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binidiv(idiv, a, b, c);
        break;
      }
      case KLOPCODE_CONCAT: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_savestate(callinfo->top, pc);
        if (kl_likely(klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING))) {
          KlString* res = klstrpool_string_concat(state->strpool, klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
          if (kl_unlikely(!res))
            return klstate_throw_oom(state, "doing concat");
          klvalue_setobj(a, res, KL_STRING);
        } else if (kl_likely(klvalue_isstrornumber(b) && klvalue_isstrornumber(c))) {
          KlString* res = klexec_doconcat(state, b, c);
          if (kl_unlikely(!res))
            return klstate_throw_oom(state, "doing concat");
          klvalue_setobj(a, res, KL_STRING);
        } else {
          KlString* op = state->common->string.concat;
          KlException exception = klexec_dobinopmethod(state, a, b, c, op);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call */
            KlValue* newbase = callinfo->top;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_ADDI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binop_i(add, a, b, c);
        break;
      }
      case KLOPCODE_SUBI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binop_i(sub, a, b, c);
        break;
      }
      case KLOPCODE_MULI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binop_i(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIVI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt imm = KLINST_ABI_GETI(inst);
        klexec_bindiv_i(div, a, b, imm);
        break;
      }
      case KLOPCODE_MODI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binmod_i(mod, a, b, c);
        break;
      }
      case KLOPCODE_IDIVI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt imm = KLINST_ABI_GETI(inst);
        klexec_binidiv_i(idiv, a, b, imm);
        break;
      }
      case KLOPCODE_ADDC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        const KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(add, a, b, c);
        break;
      }
      case KLOPCODE_SUBC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        const KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(sub, a, b, c);
        break;
      }
      case KLOPCODE_MULC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        const KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIVC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        const KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_bindiv(div, a, b, c);
        break;
      }
      case KLOPCODE_MODC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        const KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binmod(mod, a, b, c);
        break;
      }
      case KLOPCODE_IDIVC: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        const KlValue* c = constants + KLINST_ABC_GETC(inst);
        klexec_binidiv(idiv, a, b, c);
        break;
      }
      case KLOPCODE_LEN: {
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        if (kl_likely(klvalue_checktype(b, KL_ARRAY))) {
          klvalue_setint(stkbase + KLINST_ABC_GETA(inst), klarray_size(klvalue_getobj(b, KlArray*)));
        } else if (kl_likely(klvalue_checktype(b, KL_STRING))) {
          klvalue_setint(stkbase + KLINST_ABC_GETA(inst), klstring_length(klvalue_getobj(b, KlString*)));
        } else {
          klexec_savestate(callinfo->top, pc);
          KlException exception = klexec_dopreopmethod(state, stkbase + KLINST_ABC_GETA(inst), b, state->common->string.len);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call */
            KlValue* newbase = callinfo->top;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_NEG: {
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        if (kl_likely(klvalue_checktype(b, KL_INT))) {
          klvalue_setint(stkbase + KLINST_ABC_GETA(inst), -klvalue_getint(b));
        } else if (kl_likely(klvalue_checktype(b, KL_FLOAT))) {
          klvalue_setfloat(stkbase + KLINST_ABC_GETA(inst), -klvalue_getfloat(b));
        } else {
          klexec_savestate(callinfo->top, pc);
          KlException exception = klexec_dopreopmethod(state, stkbase + KLINST_ABC_GETA(inst), b, state->common->string.neg);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call */
            KlValue* newbase = callinfo->top;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_SCALL: {
        KlValue* callable = stkbase + KLINST_AXY_GETA(inst);
        size_t narg = KLINST_AXY_GETX(inst);
        if (narg == KLEXEC_VARIABLE_RESULTS)
          narg = klstate_stktop(state) - callable - 1;
        size_t nret = KLINST_AXY_GETY(inst);
        klexec_savestate(callable + 1 + narg, pc);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, -1);
        if (kl_unlikely(!newci))
          return klstate_throw_oom(state, "calling a callable object");
        KlException exception = klexec_callprepare(state, callable, narg, klexec_callprep_callback_for_call);
        if (kl_likely(newci == state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = newci->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_CALL: {
        KlValue* callable = stkbase + KLINST_AXY_GETA(inst);
        KlInstruction extra = *pc++;
        kl_assert(KLINST_GET_OPCODE(extra) == KLOPCODE_EXTRA, "something wrong in code generation");
        size_t narg = KLINST_XYZ_GETX(extra);
        if (narg == KLEXEC_VARIABLE_RESULTS)
          narg = klstate_stktop(state) - callable - 1;
        size_t nret = KLINST_XYZ_GETY(extra);
        klexec_savestate(callable + 1 + narg, pc);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, (stkbase + KLINST_AXY_GETY(extra) - 1) - callable);
        if (kl_unlikely(!newci))
          return klstate_throw_oom(state, "calling a callable object");
        KlException exception = klexec_callprepare(state, callable, narg, klexec_callprep_callback_for_call);
        if (kl_likely(newci == state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = newci->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_METHOD: {
        kl_assert(klvalue_checktype(constants + KLINST_AX_GETX(inst), KL_STRING), "field name should be a string");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "something wrong in code generation");

        KlValue* thisobj = stkbase + KLINST_AX_GETA(inst);
        KlString* field = klvalue_getobj(constants + KLINST_AX_GETX(inst), KlString*);
        KlValue callable;
        bool ismethod = klexec_getmethod(state, thisobj, field, &callable);
        KlInstruction extra = *pc++;
        size_t nret = KLINST_XYZ_GETY(extra);
        size_t narg = KLINST_XYZ_GETX(extra);
        if (narg == KLEXEC_VARIABLE_RESULTS)
          narg = klstate_stktop(state) - thisobj - 1;
        klexec_savestate(thisobj + 1 + narg, pc);
        if (ismethod) ++narg;
        KlCallInfo* newci = klexec_new_callinfo(state, nret, (stkbase + KLINST_XYZ_GETZ(extra)) - (ismethod ? thisobj : thisobj + 1));
        if (kl_unlikely(!newci))
          return klstate_throw_oom(state, "calling a callable object");
        KlException exception = klexec_callprepare(state, &callable, narg, klexec_callprep_callback_for_method);
        if (kl_likely(newci == state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = newci->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_RETURN: {
        size_t nret = callinfo->nret;
        KlValue* res = stkbase + KLINST_AX_GETA(inst);
        size_t nres = KLINST_AX_GETX(inst);
        if (nres == KLEXEC_VARIABLE_RESULTS)
          nres = klstate_stktop(state) - res;
        size_t ncopy = nret == KLEXEC_VARIABLE_RESULTS ? nres : nres < nret ? nres : nret;
        KlValue* retpos = stkbase + callinfo->retoff;
        while (ncopy--) /* copy results to their position. */
          klvalue_setvalue(retpos++, res++);
        if (nret == KLEXEC_VARIABLE_RESULTS) {
          klstack_set_top(klstate_stack(state), retpos);
        } else if (nres < nret) { /* complete missing returned value */
          klexec_setnils(retpos, nret - nres);
        }
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_RETURN0: {
        size_t nret = callinfo->nret;
        if (nret == KLEXEC_VARIABLE_RESULTS) {
          klstack_set_top(klstate_stack(state), stkbase + callinfo->retoff);
        } else {
          klexec_setnils(stkbase + callinfo->retoff, nret);
        }
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_RETURN1: {
        kl_assert(KLEXEC_VARIABLE_RESULTS != 0, "");
        KlValue* retpos = stkbase + callinfo->retoff;
        KlValue* res = stkbase + KLINST_A_GETA(inst);
        size_t nret = callinfo->nret;
        if (kl_likely(nret == 1)) {
          klvalue_setvalue(retpos, res);
        } else if (nret == KLEXEC_VARIABLE_RESULTS) {
          klvalue_setvalue(retpos, res);
          klstack_set_top(klstate_stack(state), retpos + 1);
        } else if (nret != 0) {
          klvalue_setvalue(retpos, res);
          klexec_setnils(retpos + 1, nret - 1);   /* complete missing results */
        }
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_LOADBOOL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlBool boolval = KLINST_AX_GETX(inst);
        kl_assert(boolval == KL_TRUE || boolval == KL_FALSE, "instruction format error: LOADBOOL");
        klvalue_setbool(a, boolval);
        break;
      }
      case KLOPCODE_LOADI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt intval = KLINST_AI_GETI(inst);
        klvalue_setint(a, intval);
        break;
      }
      case KLOPCODE_LOADC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* c = constants + KLINST_AX_GETX(inst);
        klvalue_setvalue(a, c);
        break;
      }
      case KLOPCODE_LOADNIL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        size_t count = KLINST_AX_GETX(inst);
        klexec_setnils(a, count);
        break;
      }
      case KLOPCODE_LOADREF: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* ref = klref_getval(closure->refs[KLINST_AX_GETX(inst)]);
        klvalue_setvalue(a, ref);
        break;
      }
      case KLOPCODE_LOADGLOBAL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        kl_assert(klvalue_checktype(constants + KLINST_AX_GETX(inst), KL_STRING), "something wrong in code generation");
        KlString* varname = klvalue_getobj(constants + KLINST_AX_GETX(inst), KlString*);
        KlMapSlot* slot = klmap_searchstring(state->global, varname);
        slot ? klvalue_setvalue(a, &slot->value) : klvalue_setnil(a);
        break;
      }
      case KLOPCODE_STOREREF: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* ref = klref_getval(closure->refs[KLINST_AX_GETX(inst)]);
        klvalue_setvalue(ref, a);
        break;
      }
      case KLOPCODE_STOREGLOBAL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        kl_assert(klvalue_checktype(constants + KLINST_AX_GETX(inst), KL_STRING), "something wrong in code generation");
        KlString* varname = klvalue_getobj(constants + KLINST_AX_GETX(inst), KlString*);
        KlMapSlot* slot = klmap_searchstring(state->global, varname);
        if (kl_likely(slot)) {
          klmap_slot_setvalue(slot, a);
        } else {
          klexec_savestate(callinfo->top, pc);
          if (kl_unlikely(!klmap_insertstring(state->global, klstate_getmm(state), varname, a)))
            return klstate_throw_oom(state, "setting a global variable");
        }
        break;
      }
      case KLOPCODE_MKMAP: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* this instruction tells us current stack top */
        KlValue* stktop = stkbase + KLINST_ABX_GETB(inst);
        size_t capacity = KLINST_ABX_GETX(inst);
        klexec_savestate(stktop, pc); /* creating map may trigger gc */
        KlMap* map = klmap_create(klstate_getmm(state), capacity);
        if (kl_unlikely(!map))
          return klstate_throw_oom(state, "creating a map");
        klvalue_setobj(a, map, KL_MAP);
        break;
      }
      case KLOPCODE_MKARRAY: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* first value to be inserted to the array */
        KlValue* first = stkbase + KLINST_ABX_GETB(inst);
        size_t nelem = KLINST_ABX_GETX(inst);
        if (nelem == KLEXEC_VARIABLE_RESULTS)
          nelem = klstate_stktop(state) - first;
        /* now stack top is first + nelem */
        klexec_savestate(first + nelem, pc);  /* creating array may trigger gc */
        KlArray* arr = klarray_create(klstate_getmm(state), nelem);
        if (kl_unlikely(!arr))
          return klstate_throw_oom(state, "creating an array");
        /* fill this array with values on stack */
        klarray_fill(arr, first, nelem);
        klvalue_setobj(a, arr, KL_ARRAY);
        break;
      }
      case KLOPCODE_MKCLOSURE: {
        kl_assert(KLINST_AX_GETX(inst) < closure->kfunc->nsubfunc, "");
        KlKFunction* kfunc = klkfunc_subfunc(closure->kfunc)[KLINST_AX_GETX(inst)];
        klexec_savestate(callinfo->top, pc);
        KlKClosure* kclo = klkclosure_create(klstate_getmm(state), kfunc, stkbase, &state->reflist, closure->refs);
        if (kl_unlikely(!kclo))
          return klstate_throw_oom(state, "creating a closure");
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        klvalue_setobj(a, kclo, KL_KCLOSURE);
        break;
      }
      case KLOPCODE_APPEND: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* first value to be inserted to the array */
        KlValue* first = stkbase + KLINST_ABX_GETB(inst);
        size_t nelem = KLINST_ABX_GETX(inst);
        if (nelem == KLEXEC_VARIABLE_RESULTS) {
          nelem = klstate_stktop(state) - first;
          klexec_savepc(callinfo, pc);          /* creating array may trigger gc */
        } else {
          klexec_savestate(callinfo->top, pc);  /* creating array may trigger gc */
        }
        if (kl_unlikely(!klvalue_checktype(a, KL_ARRAY)))
          return klstate_throw(state, KL_E_TYPE, "can only append to an array");
        if (kl_unlikely(!klarray_push_back(klvalue_getobj(a, KlArray*), klstate_getmm(state), first, nelem)))
          return klstate_throw_oom(state, "appending");
        break;
      }
      case KLOPCODE_MKCLASS: {
        KlValue* a = stkbase + KLINST_ABTX_GETA(inst);
        /* this instruction tells us current stack top */
        KlValue* stktop = stkbase + KLINST_ABTX_GETB(inst);
        size_t capacity = KLINST_ABTX_GETX(inst);
        KlClass* klclass = NULL;
        if (KLINST_ABTX_GETT(inst)) { /* is stktop base class ? */
          klexec_savestate(stktop + 1, pc);   /* creating class may trigger gc */
          if (kl_unlikely(!klvalue_checktype(stktop, KL_CLASS)))
            return klstate_throw(state, KL_E_TYPE, "inherit a non-class value, type: %s", klexec_typename(state, stktop));
          KlClass* base = klvalue_getobj(stktop, KlClass*);
          if (kl_unlikely(klclass_isfinal(base)))
            return klstate_throw(state, KL_E_INVLD, "can not inherit this class");
          klclass = klclass_inherit(klstate_getmm(state), base);
        } else {  /* this class has no base */
          klexec_savestate(stktop, pc); /* creating class may trigger gc */
          klclass = klclass_create(klstate_getmm(state), capacity, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
        }
        if (kl_unlikely(!klclass))
          return klstate_throw_oom(state, "creating a class");
        klvalue_setobj(a, klclass, KL_CLASS);
        break;
      }
      case KLOPCODE_INDEXI: {
        KlValue* val = stkbase + KLINST_ABI_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABI_GETB(inst);
        KlInt index = KLINST_ABI_GETI(inst);
        if (kl_likely(klvalue_checktype(indexable, KL_ARRAY))) {       /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          klarray_index(arr, index, val);
        } else if (kl_likely(klvalue_checktype(indexable, KL_MAP))) {  /* is map? */
          KlMap* map = klvalue_getobj(indexable, KlMap*);
          KlMapSlot* slot = klmap_searchint(map, index);
          slot ? klvalue_setvalue(val, &slot->value) : klvalue_setnil(val);
        } else {
          KlValue key;
          klvalue_setint(&key, index);
          klexec_savestate(callinfo->top, pc);
          KlException exception = klexec_doindexmethod(state, val, indexable, &key);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_INDEXASI: {
        KlValue* val = stkbase + KLINST_ABI_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABI_GETB(inst);
        KlInt index = KLINST_ABI_GETI(inst);
        if (kl_likely(klvalue_checktype(indexable, KL_ARRAY))) {         /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          KlException exception = klarray_indexas(arr, index, val);
          if (kl_unlikely(exception)) {
            klexec_savestate(callinfo->top, pc);
            return klexec_handle_arrayindexas_exception(state, exception, arr, index);
          }
        } else {
          if (kl_likely(klvalue_checktype(indexable, KL_MAP))) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            KlMapSlot* itr = klmap_searchint(map, index);
            if (itr) {
              klvalue_checktype(val, KL_NIL) ? klmap_erase(map, itr)
                                             : klvalue_setvalue(&itr->value, val);
            } else if (!klvalue_checktype(val, KL_NIL)) {
              KlValue key;
              klvalue_setint(&key, index);
              klexec_savestate(callinfo->top, pc);
              if (kl_unlikely(!klmap_insert(map, klstate_getmm(state), &key, val)))
                return klstate_throw_oom(state, "inserting a k-v pair to a map");
            }
            break;
          }
          KlValue key;
          klvalue_setint(&key, index);
          klexec_savestate(callinfo->top, pc);
          KlException exception = klexec_doindexasmethod(state, indexable, &key, val);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_INDEX: {
        KlValue* val = stkbase + KLINST_ABC_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABC_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        if (kl_likely(klvalue_checktype(indexable, KL_ARRAY))) {       /* is array? */
          if (kl_unlikely(!klvalue_checktype(key, KL_INT))) { /* only integer can index array */
            klexec_savepc(callinfo, pc);
            return klstate_throw(state, KL_E_TYPE,
                                 "type error occurred when indexing an array: expected %s, got %s.",
                                 klvalue_typename(KL_INT), klexec_typename(state, key));
          }
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          klarray_index(arr, klvalue_getint(key), val);
        } else {
          if (kl_likely(klvalue_checktype(indexable, KL_MAP))) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            if (klvalue_canrawequal(key)) {
              KlMapSlot* itr = klmap_search(map, key);
              itr ? klvalue_setvalue(val, &itr->value) : klvalue_setnil(val);
              break;
            }
          }
          klexec_savestate(callinfo->top, pc);
          KlException exception = klexec_doindexmethod(state, val, indexable, key);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_INDEXAS: {
        KlValue* val = stkbase + KLINST_ABC_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABC_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        if (kl_likely(klvalue_checktype(indexable, KL_ARRAY))) {         /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          klexec_savestate(callinfo->top, pc);
          if (kl_unlikely(!klvalue_checktype(key, KL_INT))) { /* only integer can index array */
            return klstate_throw(state, KL_E_TYPE,
                                 "type error occurred when indexing an array: expected %s, got %s.",
                                 klvalue_typename(KL_INT), klexec_typename(state, key));
          }
          KlException exception = klarray_indexas(arr, klvalue_getint(key), val);
          if (kl_unlikely(exception))
            return klexec_handle_arrayindexas_exception(state, exception, arr, klvalue_getint(key));
        } else {
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            if (klvalue_canrawequal(key)) { /* simple types that can apply raw equal. fast search and set */
              KlMapSlot* itr = klmap_search(map, key);
              if (itr) {
                klvalue_checktype(val, KL_NIL) ? klmap_erase(map, itr)
                                               : klvalue_setvalue(&itr->value, val);
              } else if (!klvalue_checktype(val, KL_NIL)) {
                klexec_savestate(callinfo->top, pc);
                if (kl_unlikely(!klmap_insert(map, klstate_getmm(state), key, val)))
                  return klstate_throw_oom(state, "inserting a k-v pair to a map");
              }
              break;
            } /* else fall through. try operator method */
          }
          klexec_savestate(callinfo->top, pc);
          KlException exception = klexec_doindexasmethod(state, indexable, key, val);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_GETFIELDR: {
        KlValue* val = stkbase + KLINST_ABC_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABC_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_GETFIELDC: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        const KlValue* key = constants + KLINST_ABX_GETX(inst);
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_SETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_SETFIELDC: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        const KlValue* key = constants + KLINST_ABX_GETX(inst);
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_REFGETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* key = stkbase + KLINST_ABX_GETB(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_ABX_GETX(inst)));
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_REFGETFIELDC: {
        KlValue* val = stkbase + KLINST_AXY_GETA(inst);
        const KlValue* key = constants + KLINST_AXY_GETX(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_AXY_GETY(inst)));
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_REFSETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* key = stkbase + KLINST_ABX_GETB(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_ABX_GETX(inst)));
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_REFSETFIELDC: {
        KlValue* val = stkbase + KLINST_AXY_GETA(inst);
        const KlValue* key = constants + KLINST_AXY_GETX(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_AXY_GETY(inst)));
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_NEWLOCAL: {
        KlValue* classval = stkbase + KLINST_AX_GETA(inst);
        const KlValue* fieldname = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_checktype(classval, KL_CLASS), "NEWLOCAL should applied to a class");
        kl_assert(klvalue_checktype(fieldname, KL_STRING), "expected string to index field");

        KlString* keystr = klvalue_getobj(fieldname, KlString*);
        KlClass* klclass = klvalue_getobj(classval, KlClass*);
        klexec_savestate(callinfo->top, pc);  /* add new field */
        KlException exception = klclass_newlocal(klclass, klstate_getmm(state), keystr);
        if (kl_unlikely(exception))
          return klexec_handle_newlocal_exception(state, exception, keystr);
        break;
      }
      case KLOPCODE_NEWMETHODC:
      case KLOPCODE_NEWMETHODR: {
        KlValue* obj;
        KlValue* value;
        const KlValue* fieldname;
        if (opcode == KLOPCODE_NEWMETHODR) {
          obj = stkbase + KLINST_ABC_GETA(inst);
          value = stkbase + KLINST_ABC_GETB(inst);
          fieldname = stkbase + KLINST_ABC_GETC(inst);
        } else {
          obj = stkbase + KLINST_ABX_GETA(inst);
          value = stkbase + KLINST_ABX_GETB(inst);
          fieldname = constants + KLINST_ABX_GETX(inst);
        }
        kl_assert(klvalue_checktype(fieldname, KL_STRING), "");

        KlString* keystr = klvalue_getobj(fieldname, KlString*);
        KlClass* klclass = klexec_getclass(state, obj);
        klexec_savestate(callinfo->top, pc);  /* add new field */
        KlException exception = klclass_newshared_method(klclass, klstate_getmm(state), keystr, value);
        if (kl_unlikely(exception))
          return klexec_handle_newshared_exception(state, exception, keystr);
        break;
      }
      case KLOPCODE_LOADFALSESKIP: {
        KlValue* a = stkbase + KLINST_A_GETA(inst);
        klvalue_setbool(a, KL_FALSE);
        ++pc;
        break;
      }
      case KLOPCODE_TESTSET: {
          KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        if (klexec_satisfy(b, cond)) {
          klvalue_setvalue(a, b);
          pc += KLINST_XI_GETI(extra);
        }
        break;
      }
      case KLOPCODE_TRUEJMP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        int offset = KLINST_AI_GETI(inst);
        if (klexec_satisfy(a, KL_TRUE)) pc += offset;
        break;
      }
      case KLOPCODE_FALSEJMP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        int offset = KLINST_AI_GETI(inst);
        if (klexec_satisfy(a, KL_FALSE)) pc += offset;
        break;
      }
      case KLOPCODE_JMP: {
        int offset = KLINST_I_GETI(inst);
        pc += offset;
        break;
      }
      case KLOPCODE_CONDJMP: {
        /* This instruction must follow a comparison instruction.
         * This instruction can only be executed if the preceding comparison
         * instruction invoked the object's comparison method and the method
         * is not C function, and the method has returned. In this case, the
         * comparison result is stored at 'callinfo->top'.
         */
        bool cond = KLINST_XI_GETX(inst);
        KlValue* val = callinfo->top;
        if (klexec_satisfy(val, cond))
          pc += KLINST_XI_GETI(inst);
        break;
      }
      case KLOPCODE_CLOSEJMP: {
        KlValue* bound = stkbase + KLINST_XI_GETX(inst);
        klreflist_close(&state->reflist, bound, klstate_getmm(state));
        pc += KLINST_XI_GETI(inst);
        break;
      }
      case KLOPCODE_HASFIELD: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* field = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_checktype(field, KL_STRING), "");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        if (!klvalue_checktype(klexec_getfield(state, a, klvalue_getobj(field, KlString*)), KL_NIL)) {
          if (cond) pc += offset;
        } else {
          if (!cond) pc += offset;
        }
        break;
      }
      case KLOPCODE_IS: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        if (klvalue_sametype(a, b) && klvalue_sameinstance(a, b)) {
          if (cond) pc += offset;
        } else {
          if (!cond) pc += offset;
        }
        break;
      }
      case KLOPCODE_EQ: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        klexec_bequal(a, b, offset, cond);
        break;
      }
      case KLOPCODE_NE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        klexec_bnequal(a, b, offset, cond);
        break;
      }
      case KLOPCODE_LT: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(lt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GT: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(gt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_LE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(le, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(ge, a, b, offset, cond);
        break;
      }
      case KLOPCODE_EQC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_canrawequal(b), "something wrong in EQC");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (klvalue_equal(a, b))
          pc += offset;
        break;
      }
      case KLOPCODE_NEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_canrawequal(b), "something wrong in NEC");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (!klvalue_equal(a, b))
          pc += offset;
        break;
      }
      case KLOPCODE_LTC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(lt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GTC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(gt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_LEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(le, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(ge, a, b, offset, cond);
        break;
      }
      case KLOPCODE_EQI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        int imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (klvalue_isnumber(a) && klvalue_getnumber(a) == imm)
          pc += offset;
        break;
      }
      case KLOPCODE_NEI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (!klvalue_isnumber(a) || klvalue_getnumber(a) != imm)
          pc += offset;
        break;
      }
      case KLOPCODE_LTI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(lt, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_GTI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(gt, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_LEI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(le, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_GEI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(ge, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_MATCH: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        const KlValue* b = constants + KLINST_AX_GETX(inst);
        bool matched = klvalue_sametype(a, b)   ? klvalue_sameinstance(a, b) :
                       klvalue_bothnumber(a, b) ? klvalue_getnumber(a) == klvalue_getnumber(b) :
                                                  false;
        if (kl_unlikely(!matched)) {
          klexec_savestate(callinfo->top, pc);
          return klstate_throw(state, KL_E_MISMATCH, "pattern mismatch");
        }
        break;
      }
      case KLOPCODE_PMARR:
      case KLOPCODE_PBARR: {
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        if (!klvalue_checktype(b, KL_ARRAY)) {
          if (kl_unlikely(opcode == KLOPCODE_PBARR)) {  /* is pattern binding? */
            klexec_savestate(callinfo->top, pc);
            return klstate_throw(state, KL_E_TYPE, "pattern binding: not an array");
          }
          /* else jump out */
          KlInstruction extra = *pc++;
          pc += KLINST_XI_GETI(extra);
          break;
        }
        KlInstruction extra = *pc++;
        /* else is array */
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlArray* array = klvalue_getobj(b, KlArray*);
        size_t nfront = KLINST_ABX_GETX(inst);
        size_t arrsize = klarray_size(array);
        KlValue* end = a + (arrsize < nfront ? arrsize : nfront);
        KlValue* begin = a;
        for (KlValue* itr = klarray_iter_begin(array); begin != end; ++itr)
          klvalue_setvalue(begin++, itr);
        while (begin != a + nfront) klvalue_setnil(begin++); /* complete missing values */
        size_t nback = KLINST_XI_GETX(extra);
        begin += nback;
        kl_assert(begin == a + nfront + nback, "");
        end = begin - (arrsize < nback ? arrsize : nback);
        for (KlArrayIter itr = klarray_iter_end(array); begin != end;)
          klvalue_setvalue(--begin, klarray_iter_get(array, --itr));
        while (begin != a + nfront) klvalue_setnil(--begin); /* complete missing values */
        break;
      }
      case KLOPCODE_PMTUP:
      case KLOPCODE_PBTUP: {
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        size_t nwanted = KLINST_ABX_GETX(inst);
        KlArray* array = klvalue_getobj(b, KlArray*);
        if (!klvalue_checktype(b, KL_ARRAY) || klarray_size(array) != nwanted) {
          if (kl_unlikely(opcode == KLOPCODE_PBTUP)) {  /* is pattern binding? */
            klexec_savestate(callinfo->top, pc);
            return klstate_throw(state, KL_E_TYPE, "pattern binding: not an array with %zd elements", nwanted);
          }
          /* else jump out */
          kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
          KlInstruction extra = *pc++;
          pc += KLINST_XI_GETI(extra);
          break;
        }
        /* else is array with exactly 'nwanted' elements */
        if (opcode == KLOPCODE_PMTUP) /* is pattern binding? */
          ++pc; /* skip extra instruction */
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* end = klarray_iter_end(array);
        for (KlValue* itr = klarray_iter_begin(array); itr != end; ++itr)
          klvalue_setvalue(a++, itr);
        break;
      }
      case KLOPCODE_PMMAP:
      case KLOPCODE_PBMAP: {
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        if (!klvalue_checktype(b, KL_MAP)) {
          if (kl_unlikely(opcode == KLOPCODE_PBMAP)) {  /* is pattern binding? */
            klexec_savestate(callinfo->top, pc);
            return klstate_throw(state, KL_E_TYPE, "pattern binding: not an map");
          }
          /* else jump out, pattern matching instruction must be followed by extra information */
          kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_PMAPPOST, "");
          KlInstruction extra = *pc++;
          pc += KLINST_XI_GETI(extra);
          break;
        }
        ++pc; /* skip pmmappost */
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        size_t nwanted = KLINST_ABX_GETX(inst);
        KlMap* map = klvalue_getobj(b, KlMap*);
        KlValue* key = b + 1;
        /* 'b' may be overwritten while do pattern binding,
         * so store 'b' to another position on stack that will not be overwitten
         * so that 'b' would not be collected by garbage collector */
        kl_assert(b + nwanted + 2 < klstack_size(klstate_stack(state)) + klstack_raw(klstate_stack(state)), "compiler error");
        klvalue_setvalue(b + nwanted + 1, b);
        for (size_t i = 0; i < nwanted; ++i) {
          if (kl_likely(klvalue_canrawequal(key + i))) {
            const KlMapSlot* slot = klmap_search(map, key + i);
            slot ? klvalue_setvalue(a + i, &slot->value) : klvalue_setnil(a + i);
          } else {
            klvalue_setint(b + nwanted + 2, i); /* save current index for pmappost */
            klexec_savestate(callinfo->top, pc);
            KlException exception = klexec_doindexmethod(state, a + i, b + nwanted + 1, key + i);
            if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
              KlValue* newbase = state->callinfo->base;
              klexec_updateglobal(newbase);
              --pc; /* do pmmappost instruction */
              break;  /* break to execute new function */
            } else {
              if (kl_unlikely(exception)) return exception;
              /* C function or C closure */
              /* stack may have grown. restore stkbase. */
              stkbase = callinfo->base;
            }
          }
        }
        break;
      }
      case KLOPCODE_PMAPPOST: {
        /* This instruction must follow map pattern binding or matching instruction.
         * This instruction can only be executed if the preceding instruction
         * call the index method of map and the method is not C function, and the
         * method has returned. This instruction will complete things not done by
         * previous instruction.
         */
        KlInstruction previnst = *(pc - 2);
        KlValue* b = stkbase + KLINST_ABX_GETB(previnst);
        KlValue* a = stkbase + KLINST_ABX_GETA(previnst);
        size_t nwanted = KLINST_ABX_GETX(previnst);
        KlMap* map = klvalue_getobj(b + nwanted + 1, KlMap*);
        KlValue* key = b + 1;
        kl_assert(klvalue_checktype(b + nwanted + 1, KL_MAP), "");
        kl_assert(klvalue_checktype(b + nwanted + 2, KL_INT), "");
        for (size_t i = klvalue_getint(b + nwanted + 2); i < nwanted; ++i) {
          if (kl_likely(klvalue_canrawequal(key + i))) {
            KlMapSlot* itr = klmap_search(map, key + i);
            itr ? klvalue_setvalue(a + i, &itr->value) : klvalue_setnil(a + i);
          } else {
            klvalue_setint(b + nwanted + 2, i); /* save current index */
            klexec_savetop(callinfo->top);
            klexec_savepc(callinfo, pc - 1);
            KlException exception = klexec_doindexmethod(state, a + i, b + nwanted + 1, key + i);
            if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
              KlValue* newbase = state->callinfo->base;
              klexec_updateglobal(newbase);
              break;  /* break to execute new function */
            } else {
              if (kl_unlikely(exception)) return exception;
              /* C function or C closure */
              /* stack may have grown. restore stkbase. */
              stkbase = callinfo->base;
            }
          }
        }
        break;
      }
      case KLOPCODE_PMOBJ:
      case KLOPCODE_PBOBJ: {
        KlValue* objonstk = stkbase + KLINST_ABX_GETB(inst);
        KlValue* field = objonstk + 1;
        KlValue obj;
        klvalue_setvalue(&obj, objonstk);
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        size_t nwanted = KLINST_ABX_GETX(inst);
        for (size_t i = 0; i < nwanted; ++i) {
          kl_assert(klvalue_checktype(field + i, KL_STRING), "");
          KlString* fieldname = klvalue_getobj(field + i, KlString*);
          klvalue_setvalue(a + i, klexec_getfield(state, &obj, fieldname));
        }
        break;
      }
      case KLOPCODE_NEWOBJ: {
        KlValue* klclass = stkbase + KLINST_ABC_GETB(inst);
        klexec_savestate(callinfo->top, pc);
        if (kl_unlikely(!klvalue_checktype(klclass, KL_CLASS)))
          return klstate_throw(state, KL_E_TYPE, "%s is not a class", klexec_typename(state, klclass));
        KlException exception = klclass_new_object(klvalue_getobj(klclass, KlClass*), klstate_getmm(state), stkbase + KLINST_ABC_GETA(inst));
        if (kl_unlikely(exception))
          return klexec_handle_newobject_exception(state, exception);
        break;
      }
      case KLOPCODE_ADJUSTARGS: {
        size_t narg = klstate_stktop(state) - stkbase;
        size_t nparam = klkfunc_nparam(closure->kfunc);
        kl_assert(narg >= nparam, "something wrong in callprepare");
        /* a closure having variable arguments needs 'narg'. */
        callinfo->narg = narg;
        if (narg > nparam) {
          /* it's not necessary to grow stack here.
           * callprepare ensures enough stack frame size.
           */
          KlValue* fixed = stkbase;
          KlValue* stktop = klstate_stktop(state);
          while (nparam--)   /* move fixed arguments to top */
            klvalue_setvalue(stktop++, fixed++);
          callinfo->top += narg;
          callinfo->base += narg;
          callinfo->retoff -= narg;
          stkbase += narg;
          klstack_set_top(klstate_stack(state), stktop);
        }
        break;
      }
      case KLOPCODE_VFORPREP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        kl_assert(klvalue_checktype(a, KL_INT), "compiler error");
        KlInt nextra = callinfo->narg - klkfunc_nparam(closure->kfunc);
        KlInt step = klvalue_getint(a);
        kl_assert(step > 0, "compiler error");
        if (nextra != 0) {
          KlValue* varpos = a + 2;
          KlValue* argpos = stkbase - nextra;
          size_t nvalid = step > nextra ? nextra : step;
          klvalue_setint(a + 1, nextra - nvalid);  /* set index for next iteration */
          while (nvalid--)
            klvalue_setvalue(varpos++, argpos++);
          while (step-- > nextra)
            klvalue_setnil(varpos++);
        } else {
          pc += KLINST_AI_GETI(inst);
        }
        break;
      }
      case KLOPCODE_VFORLOOP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        kl_assert(klvalue_checktype(a, KL_INT), "");
        KlInt step = klvalue_getint(a);
        KlInt idx = klvalue_getint(a + 1);
        if (idx != 0) {
          KlValue* varpos = a + 2;
          KlValue* argpos = stkbase - idx;
          size_t nvalid = step > idx ? idx : step;
          klvalue_setint(a + 1, idx - nvalid);
          while (nvalid--)
            klvalue_setvalue(varpos++, argpos++);
          while (step-- > idx)
            klvalue_setnil(varpos++);
          pc += KLINST_AI_GETI(inst);
        }
        break;
      }
      case KLOPCODE_IFORPREP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        int offset = KLINST_AI_GETI(inst);
        klexec_savestate(a + 3, pc);
        KlException exception = klexec_iforprep(state, a, offset);
        if (kl_unlikely(exception)) return exception;
        pc = callinfo->savedpc; /* pc may be changed by klexec_iforprep() */
        break;
      }
      case KLOPCODE_IFORLOOP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        kl_assert(klvalue_checktype(a, KL_INT) && klvalue_checktype(a, KL_INT) && klvalue_checktype(a, KL_INT), "");
        KlInt i = klvalue_getint(a);
        KlInt end = klvalue_getint(a + 1);
        KlInt step = klvalue_getint(a + 2);
        i += step;
        if (i != end) {
          pc += KLINST_AI_GETI(inst);
          klvalue_setint(a, i);
        }
        break;
      }
      case KLOPCODE_GFORLOOP: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        size_t nret = KLINST_AX_GETX(inst);
        KlValue* argbase = a + 1;
        klexec_savestate(argbase + nret, pc);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
        if (kl_unlikely(!newci))
          return klstate_throw_oom(state, "doing generic for loop");
        KlException exception = klexec_callprepare(state, a, nret, NULL);
        if (newci == state->callinfo) { /* is a klang call ? */
          KlValue* newbase = newci->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
          KlInstruction jmp = *pc++;
          kl_assert(KLINST_GET_OPCODE(jmp) == KLOPCODE_TRUEJMP && KLINST_AI_GETA(jmp) == KLINST_AX_GETA(inst) + 3, "");
          /* test the first porgrammer-visible iteration variable */
          KlValue* testval = stkbase + KLINST_AX_GETA(inst) + 3;
          if (kl_likely(klexec_satisfy(testval, KL_TRUE)))
            pc += KLINST_AI_GETI(jmp);
        }
        break;
      }
      case KLOPCODE_ASYNC: {
        KlValue* f = stkbase + KLINST_ABC_GETB(inst);
        klexec_savestate(callinfo->top, pc);
        if (kl_unlikely(!klvalue_checktype(f, KL_KCLOSURE))) {
          return klstate_throw(state, KL_E_TYPE,
                               "async should be applied to a klang closure, got '%s'",
                               klexec_typename(state, f));
        }
        KlState* costate = klco_create(state, klvalue_getobj(f, KlKClosure*));
        if (kl_unlikely(!costate)) return klstate_throw_oom(state, "creating a coroutine");
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        klvalue_setobj(a, costate, KL_COROUTINE);
        break;
      }
      case KLOPCODE_YIELD: {
        if (kl_unlikely(!klco_yield_allowed(&state->coinfo))) {
          klexec_savestate(callinfo->top, pc);
          return klstate_throw(state, KL_E_INVLD, "can not yield from here");
        }
        KlValue* first = stkbase + KLINST_AXY_GETA(inst);
        size_t nres = KLINST_AXY_GETX(inst);
        if (nres == KLEXEC_VARIABLE_RESULTS)
          nres = klstate_stktop(state) - first;
        size_t nwanted = KLINST_AXY_GETY(inst);
        klexec_savestate(first + nres, pc);
        klco_yield(&state->coinfo, first, nres, nwanted);
        return KL_E_NONE;
      }
      case KLOPCODE_VARARG: {
        size_t nwanted = KLINST_AXY_GETX(inst);
        size_t nvarg = callinfo->narg - klkfunc_nparam(closure->kfunc);
        if (nwanted == KLEXEC_VARIABLE_RESULTS)
          nwanted = nvarg;
        klexec_savestate(callinfo->top, pc);
        if (kl_unlikely(klstate_checkframe(state, nwanted)))
          return klstate_currexception(state);
        size_t ncopy = nwanted < nvarg ? nwanted : nvarg;
        KlValue* a = stkbase + KLINST_AXY_GETA(inst);
        KlValue* b = stkbase - nvarg;
        while (ncopy--)
          klvalue_setvalue(a++, b++);
        if (nwanted > nvarg)
          klexec_setnils(a, nwanted - nvarg);
        if (KLINST_AXY_GETX(inst) == KLEXEC_VARIABLE_RESULTS)
          klstack_set_top(klstate_stack(state), a);
        break;
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        break;
      }
    }
  }

  return KL_E_NONE;
}
