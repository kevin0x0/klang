#include "klang/include/misc/klutils.h"
#include "klang/include/value/klarray.h"
#include "klang/include/value/klcfunc.h"
#include "klang/include/value/klclass.h"
#include "klang/include/value/klclosure.h"
#include "klang/include/value/klcoroutine.h"
#include "klang/include/value/klstate.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klexception.h"
#include "klang/include/vm/klexec.h"
#include "klang/include/vm/klinst.h"
#include "klang/include/vm/klstack.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/* extra stack frame size for calling operator method */
#define KLEXEC_STACKFRAME_EXTRA             (4)

static KlException klexec_handle_setfield_exception(KlState* state, KlException exception, KlString* key) {
  if (exception == KL_E_OOM)
    return klstate_throw(state, exception, "out of memory when setting a new field: %s", klstring_content(key));
  else if (exception == KL_E_INVLD)
    return klstate_throw(state, exception, "\'%s\' is not a shared field", klstring_content(key));
  kl_assert(false, "control flow should not reach here");
  return KL_E_NONE;
}

static KlException klexec_handle_newlocal_exception(KlState* state, KlException exception, KlString* key) {
  if (exception == KL_E_OOM)
    return klstate_throw(state, exception, "out of memory when adding a new field: %s", klstring_content(key));
  else if (exception == KL_E_INVLD)
    return klstate_throw(state, exception, "\'%s\' alreay exists", klstring_content(key));
  kl_assert(false, "control flow should not reach here");
  return KL_E_NONE;
}

static KlException klexec_handle_arrayindexas_exception(KlState* state, KlException exception, KlArray* arr, KlValue* key) {
  if (exception == KL_E_RANGE) {
    return klstate_throw(state, exception,
                         "index out of range: index = %zd, array size = %zu.",
                         klvalue_getint(key), klarray_size(arr));
  }
  kl_assert(false, "control flow should not reach here");
  return KL_E_NONE;
}

KlCallInfo* klexec_alloc_callinfo(KlState* state) {
  KlCallInfo* callinfo = (KlCallInfo*)klmm_alloc(klstate_getmm(state), sizeof (KlCallInfo));
  if (kl_unlikely(!callinfo)) return NULL;
  state->callinfo->next = callinfo;
  callinfo->prev = state->callinfo;
  callinfo->next = NULL;
  return callinfo;
}

KlException klexec_callc(KlState* state, KlCFunction* cfunc, size_t narg, size_t nret) {
  kl_assert(false, "deprecated");
  size_t stkbase = klstack_size(klstate_stack(state)) - narg;
  KlException exception = cfunc(state);
  size_t currtop = klstack_size(klstate_stack(state));

  kl_assert(currtop >= stkbase, "stack operation error in C function(or closure)");

  size_t nres = currtop - stkbase;
  if (nres < nret) {
    /* Complete missing returned values */
    klstack_pushnil(klstate_stack(state), nret - nres);
  } else {
    /* Pop excess returned value */
    klstack_move_top(klstate_stack(state), nres - nret);
  }
  return exception;
}

KlException klexec_call(KlState* state, KlValue* callable, size_t narg, size_t nret) {
  KlCallInfo* prevci = state->callinfo;
  KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
  newci->status = KLSTATE_CI_STATUS_NORM;
  ptrdiff_t stktop_save = klexec_savestack(state, klstate_stktop(state) - narg + nret);
  KlException exception = klexec_callprepare(state, callable, narg, NULL);
  if (exception) return exception;
  if (prevci != state->callinfo) {  /* to be executed klang call */
    state->callinfo->status |= KLSTATE_CI_STATUS_STOP;
    exception = klexec_execute(state);
    if (kl_unlikely(exception)) return exception;
  }
  klstack_set_top(klstate_stack(state), klexec_restorestack(state, stktop_save));
  return KL_E_NONE;
}

static bool klexec_is_method(KlValue* callable) {
  if (kl_likely(klvalue_checktype(callable, KL_KCLOSURE))) {
    return klkfunc_ismethod(klvalue_getobj(callable, KlKClosure*)->kfunc);
  } else if (klvalue_checktype(callable, KL_COROUTINE)) {
    return klco_ismethod(&klvalue_getobj(callable, KlState*)->coinfo);
  } else  {
    /* else take all the C function and C closure as method */
    return klvalue_checktype(callable, KL_CCLOSURE) ||
           klvalue_checktype(callable, KL_CCLOSURE);
  }
}

/* insert 'thisobj' before all parameters.
 * stack size is guaranteed by caller.
 */
static void klexec_addthis(KlState* state, KlValue* thisobj, size_t narg) {
  kl_assert(klstack_residual(klstate_stack(state)) >= 1, "");
  KlValue* stktop = klstate_stktop(state);
  while (narg--) {
    klvalue_setvalue(stktop, stktop - 1);
      --stktop;
  }
  klvalue_setvalue(stktop, thisobj);
  klstack_move_top(klstate_stack(state), 1);
}

/* Prepare for calling a callable object (C function, C closure, klang closure).
 * Also perform the actual call for C function and C closure.
 */
KlException klexec_callprepare(KlState* state, KlValue* callable, size_t narg, KlCallPrepCallBack callback) {
  KlCallInfo* callinfo = klexec_newed_callinfo(state);
  if (kl_likely(klvalue_checktype(callable, KL_KCLOSURE))) {          /* is a klang closure ? */
    /* get closure in advance, in case of dangling pointer due to stack grow */
    KlKClosure* kclo = klvalue_getobj(callable, KlKClosure*);
    KlKFunction* kfunc = kclo->kfunc;
    size_t framesize = klkfunc_framesize(kfunc);
    /* ensure enough stack size for this call.
     * (precisely, we expect residual size of stack >= framesize - narg + KLEXEC_STACKFRAME_EXTRA,
     * but it's shorter to write this way) */
    if (kl_unlikely(klstate_checkframe(state, framesize + KLEXEC_STACKFRAME_EXTRA))) {
      return klstate_throw(state, KL_E_OOM, "out of memory when calling a klang closure");
    }
    size_t nparam = klkfunc_nparam(kfunc);
    if (narg < nparam) {  /* complete missing arguments */
      klstack_pushnil(klstate_stack(state), nparam - narg);
      narg = nparam;
    }
    /* fill callinfo, callinfo->narg is not necessary for klang closure. */
    callinfo->callable.clo = klmm_to_gcobj(kclo);
    callinfo->status = KLSTATE_CI_STATUS_KCLO;
    callinfo->savedpc = klkfunc_entrypoint(kfunc);
    KlValue* stktop = klstate_stktop(state);
    callinfo->top = stktop + framesize - narg;
    callinfo->base = stktop - narg;
    klexec_push_callinfo(state);
    return KL_E_NONE;
  } else if (kl_likely(klvalue_checktype(callable, KL_COROUTINE))) {  /* is a coroutine ? */
    return klco_call(klvalue_getobj(callable, KlState*), state, narg, callinfo->nret);
  } else if (kl_likely(klvalue_checktype(callable, KL_CFUNCTION))) {  /* is a C function ? */
    KlCFunction* cfunc = klvalue_getcfunc(callable);
    callinfo->callable.cfunc = cfunc;
    callinfo->status = KLSTATE_CI_STATUS_CFUN;
    callinfo->narg = narg;
    callinfo->base = klstate_stktop(state) - narg;
    klexec_push_callinfo(state);
    /* do the call */
    KlException exception = klexec_callc(state, cfunc, narg, callinfo->nret);
    klexec_pop_callinfo(state);
    return exception;
  } else if (kl_likely(klvalue_checktype(callable, KL_CCLOSURE))) {   /* is a C closure ? */
    KlCClosure* cclo = klvalue_getobj(callable, KlCClosure*);
    callinfo->callable.clo = klmm_to_gcobj(cclo);
    callinfo->status = KLSTATE_CI_STATUS_CCLO;
    callinfo->narg = narg;
    callinfo->base = klstate_stktop(state) - narg;
    klexec_push_callinfo(state);
    /* do the call */
    KlException exception = klexec_callc(state, cclo->cfunc, narg, callinfo->nret);
    klexec_pop_callinfo(state);
    return exception;
  } else {
    if (callback)
      return callback(state, callable, narg);
    return klstate_throw(state, KL_E_TYPE,
           "try to call a non-callable object(method '%s' can not be called from here)",
           state->common->string.call);
  }
}

static KlException klexec_callprep_callback_for_call(KlState* state, KlValue* callable, size_t narg) {
  KlCallInfo* callinfo = klexec_newed_callinfo(state);
  KlValue* method = klexec_getfield(state, callable, state->common->string.call);
  if (kl_unlikely(!method || !klvalue_callable(method)))
    return klstate_throw(state, KL_E_TYPE, "try to call a non-callable object");
  if (klexec_is_method(method)) {
    ++narg;
    klvalue_setvalue(klstate_getval(state, -narg), callable);
    kl_assert(callinfo->retoff == -1, "");
    callinfo->retoff = 0;
  }
  return klexec_callprepare(state, method, narg, NULL);
}

static KlException klexec_callprep_callback_for_method(KlState* state, KlValue* callable, size_t narg) {
  KlCallInfo* callinfo = klexec_newed_callinfo(state);
  KlValue* method = klexec_getfield(state, callable, state->common->string.call);
  if (kl_unlikely(!method || !klvalue_callable(method)))
    return klstate_throw(state, KL_E_TYPE, "try to call a non-callable object");
  if (klexec_is_method(method)) {
    ++narg;
    klvalue_setvalue(klstate_getval(state, -narg), callable);
    callinfo->retoff = 0;
  }
  return klexec_callprepare(state, method, narg, NULL);
}

KlValue* klexec_getfield(KlState* state, KlValue* object, KlString* field) {
  if (klvalue_dotable(object)) {
    KlObject* obj = klvalue_getobj(object, KlObject*);
    return klobject_getfield(obj, field);
  } else {
    KlClass* phony = klvalue_checktype(object, KL_CLASS)
                   ? klvalue_getobj(object, KlClass*)
                   : state->common->klclass.phony[klvalue_gettype(object)];
    /* phony class should have only shared field */
    KlClassCeil* ceil = klclass_findshared(phony, field);
    return ceil != NULL ? &ceil->value : NULL;
  }
}

static KlException klexec_dopreopmethod(KlState* state, KlValue* a, KlValue* b, KlString* op) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_instack(klstate_stack(state), a), "'a' must be on stack");
  kl_assert(klstack_residual(klstate_stack(state)) >= 1, "stack size not enough for calling prefix operator");

  KlValue* method = klexec_getfield(state, b, op);
  if (kl_unlikely(!method || !klexec_is_method(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply prefix '%s' to value with type '%s'",
                         klstring_content(op), klvalue_typename(klvalue_gettype(b)));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 1, a - klstate_stktop(state));
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling a prefix operator method");
  klstack_pushvalue(klstate_stack(state), b);
  return klexec_callprepare(state, method, 1, NULL);
}

KlException klexec_dobinopmethod(KlState* state, KlValue* a, KlValue* b, KlValue* c, KlString* op) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_instack(klstate_stack(state), a), "'a' must be on stack");
  kl_assert(klstack_residual(klstate_stack(state)) >= 2, "stack size not enough for calling binary operator");

  KlValue* method = klexec_getfield(state, b, op);
  if (kl_unlikely(!method || !klexec_is_method(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply binary '%s' to values with type '%s' and '%s'",
                         klstring_content(op), klvalue_typename(klvalue_gettype(b)), klvalue_typename(klvalue_gettype(c)));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 1, a - klstate_stktop(state));
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling a binary operator method");
  klstack_pushvalue(klstate_stack(state), b);
  klstack_pushvalue(klstate_stack(state), c);
  return klexec_callprepare(state, method, 2, NULL);
}

static KlException klexec_doindexmethod(KlState* state, KlValue* val, KlValue* indexable, KlValue* key) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_instack(klstate_stack(state), val), "'a' must be on stack");
  kl_assert(klstack_residual(klstate_stack(state)) >= 1, "stack size not enough for calling binary operator");

  KlString* index = state->common->string.index;
  KlValue* method = klexec_getfield(state, indexable, index);
  if (kl_unlikely(!method || !klexec_is_method(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply '%s' to values with type '%s' for key with type '%s'",
                         klstring_content(index), klvalue_typename(klvalue_gettype(indexable)), klvalue_typename(klvalue_gettype(key)));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 1, val - klstate_stktop(state));
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling index operator method");
  klstack_pushvalue(klstate_stack(state), indexable);
  klstack_pushvalue(klstate_stack(state), key);
  return klexec_callprepare(state, method, 2, NULL);
}

static KlException klexec_doindexasmethod(KlState* state, KlValue* indexable, KlValue* key, KlValue* val) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_residual(klstate_stack(state)) >= 2, "stack size not enough for calling indexas operator");

  KlString* indexas = state->common->string.indexas;
  KlValue* method = klexec_getfield(state, indexable, indexas);
  if (kl_unlikely(!method || !klexec_is_method(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply '%s' to values with type '%s' for key with type '%s'",
                         klstring_content(indexas), klvalue_typename(klvalue_gettype(indexable)), klvalue_typename(klvalue_gettype(key)));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 0, 0);
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling indexas operator method");
  klstack_pushvalue(klstate_stack(state), indexable);
  klstack_pushvalue(klstate_stack(state), key);
  klstack_pushvalue(klstate_stack(state), val);
  return klexec_callprepare(state, method, 3, NULL);
}

static KlException klexec_domultiargsmethod(KlState* state, KlValue* obj, KlValue* res, size_t narg, KlString* op) {
  KlValue* method = klexec_getfield(state, obj, op);
  if (kl_unlikely(!method || !klexec_is_method(method))) {
    return klstate_throw(state, KL_E_INVLD, "can not apply '%s' to values with type '%s'",
                         klstring_content(op), klvalue_typename(klvalue_gettype(obj)));
  }
  KlCallInfo* newci = klexec_new_callinfo(state, 1, res - (klstate_stktop(state) - narg));
  if (kl_unlikely(!newci))
    return klstate_throw(state, KL_E_OOM, "out of memory when calling indexas operator method");
  klexec_addthis(state, obj, narg);
  return klexec_callprepare(state, method, narg, NULL);
}

static KlException klexec_dolt(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res < 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.lt);
}

static KlException klexec_dole(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res <= 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.le);
}

static KlException klexec_dogt(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res > 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.gt);
}

static KlException klexec_doge(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  if (klvalue_checktype(b, KL_STRING) && klvalue_checktype(c, KL_STRING)) {
    int res = klstring_compare(klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
    klvalue_setbool(a, res >= 0);
    return KL_E_NONE;
  }
  return klexec_dobinopmethod(state, a, b, c, state->common->string.lt);
}

static KlException klexec_iforprep(KlState* state, KlValue* ctrlvars, int offset) {
  if (kl_unlikely(!klvalue_checktype(ctrlvars, KL_INT)      ||
                  !klvalue_checktype(ctrlvars + 1, KL_INT)  ||
                  !(klvalue_checktype(ctrlvars + 2, KL_INT) ||
                    klvalue_checktype(ctrlvars + 2, KL_NIL)))) {
    return klstate_throw(state, KL_E_TYPE, "expected integer for integer loop, got %s, %s, %s",
                         klvalue_typename(klvalue_gettype(ctrlvars)),
                         klvalue_typename(klvalue_gettype(ctrlvars + 1)),
                         klvalue_typename(klvalue_gettype(ctrlvars + 2)));
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
    end -= (end - begin + step - 1) % step;
    klvalue_setint(ctrlvars + 1, end);
  } else if (begin > end) {
    if (step >= 0)
      return klstate_throw(state, KL_E_INVLD, "for loop: infinite loop: begin = %zd, end = %zd, step = %zd", begin, end, step);
    end += (begin - end - step - 1) % (-step);
    klvalue_setint(ctrlvars + 1, end);
  } else {
    state->callinfo->savedpc += offset;
  }
  return KL_E_NONE;
}


/* macros for klstate_execute() */

#define klop_add(a, b)              ((a) + (b))
#define klop_sub(a, b)              ((a) - (b))
#define klop_mul(a, b)              ((a) * (b))
#define klop_div(a, b)              ((a) / (b))
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

#define klexec_savestate(top, callinfo) {                                         \
  klstack_set_top(klstate_stack(state), top);                                     \
  (callinfo)->savedpc = pc;                                                       \
}


#define klexec_binop(op, a, b, c) {                                               \
  if (kl_likely(klvalue_bothinteger((b), (c)))) {                                 \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), klvalue_getint((c))));     \
  } else if (kl_likely(klvalue_bothnumber((b), (c)))) {                           \
    klvalue_setfloat((a), klop_##op(klvalue_getnumber((b)),                       \
                                    klvalue_getnumber((c))));                     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), (c), opname);   \
    if (kl_unlikely(exception)) return exception;                                 \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {  /* C function or C closure */                                       \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_bindiv(op, a, b, c) {                                              \
  if (kl_likely(klvalue_bothinteger((b), (c)))) {                                 \
    KlInt cval = klvalue_getint((c));                                             \
    if (kl_unlikely(cval == 0)) {                                                 \
      return klstate_throw(state, KL_E_ZERO, "divided by zero");                  \
    }                                                                             \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), cval));                    \
  } else if (kl_likely(klvalue_bothnumber((b), (c)))) {                           \
    klvalue_setfloat((a), klop_##op(klvalue_getnumber((b)),                       \
                                    klvalue_getnumber((c))));                     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), (c), opname);   \
    if (kl_unlikely(exception)) return exception;                                 \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {  /* C function or C closure */                                       \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binmod(op, a, b, c) {                                              \
  if (kl_likely(klvalue_bothinteger((b), (c)))) {                                 \
    KlInt cval = klvalue_getint((c));                                             \
    if (kl_unlikely(cval == 0)) {                                                 \
      return klstate_throw(state, KL_E_ZERO, "divided by zero");                  \
    }                                                                             \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), cval));                    \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    KlString* opname = state->common->string.op;                                  \
    KlException exception = klexec_dobinopmethod(state, (a), (b), (c), opname);   \
    if (kl_unlikely(exception)) return exception;                                 \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {  /* C function or C closure */                                       \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binop_i(op, a, b, imm) {                                           \
  if (kl_likely(klvalue_checktype((b), KL_INT))) {                                \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), (imm)));                   \
  } else if (kl_likely(klvalue_checktype((b), KL_FLOAT))) {                       \
    klvalue_setfloat((a), klop_##op(klvalue_getnumber((b)), (KlFloat)(imm)));     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    KlString* opname = state->common->string.op;                                  \
    KlValue tmp;                                                                  \
    klvalue_setint(&tmp, imm);                                                    \
    KlException exception = klexec_dobinopmethod(state, (a), (b), &tmp, opname);  \
    if (kl_unlikely(exception)) return exception;                                 \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {  /* C function or C closure */                                       \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
    }                                                                             \
  }                                                                               \
}

#define klexec_binmod_i(op, a, b, imm) {                                          \
  if (kl_likely(klvalue_checktype((b), KL_INT))) {                                \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), (imm)));                   \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    KlString* opname = state->common->string.op;                                  \
    KlValue tmp;                                                                  \
    klvalue_setint(&tmp, imm);                                                    \
    KlException exception = klexec_dobinopmethod(state, (a), (b), &tmp, opname);  \
    if (kl_unlikely(exception)) return exception;                                 \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {  /* C function or C closure */                                       \
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
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    KlValue* respos = callinfo->top;                                              \
    KlException exception = klexec_do##order(state, respos, (a), (b));            \
    if (kl_unlikely(exception)) return exception;                                 \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      callinfo->savedpc -= 1; /* redo condition jump later */                     \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {  /* C function or C closure */                                       \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
      if (klexec_if(callinfo->top) == cond) pc += offset;                         \
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
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    KlValue* respos = callinfo->top;                                              \
    KlException exception = klexec_do##order(state, respos, (a), &tmpval);        \
    if (kl_unlikely(exception)) return exception;                                 \
    if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */         \
      callinfo->savedpc -= 1; /* redo condition jump later */                     \
      KlValue* newbase = state->callinfo->base;                                   \
      klexec_updateglobal(newbase);                                               \
    } else {  /* C function or C closure */                                       \
      /* stack may have grown. restore stkbase. */                                \
      stkbase = callinfo->base;                                                   \
      if (klexec_if(callinfo->top) == cond) pc += offset;                         \
    }                                                                             \
  }                                                                               \
}

#define klexec_bequal(a, b, offset, cond) {                                       \
  if (kl_likely(klvalue_sametype((a), (b)))) {                                    \
    if (klvalue_sameinstance((a), (b))) {                                         \
      if (cond) pc += offset;                                                     \
    } else if (!klvalue_canrawequal((a))) {                                       \
      klexec_savestate(callinfo->top, callinfo);                                  \
      KlValue* respos = callinfo->top;                                            \
      KlString* op = state->common->string.eq;                                    \
      KlException exception = klexec_dobinopmethod(state, respos, (a), (b), op);  \
      if (kl_unlikely(exception)) return exception;                               \
      if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */       \
        callinfo->savedpc -= 1; /* redo condition jump later */                   \
        KlValue* newbase = state->callinfo->base;                                 \
        klexec_updateglobal(newbase);                                             \
      } else {  /* C function or C closure */                                     \
        /* stack may have grown. restore stkbase. */                              \
        stkbase = callinfo->base;                                                 \
        if (klexec_if(callinfo->top) == cond) pc += offset;                       \
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
      klexec_savestate(callinfo->top, callinfo);                                  \
      KlValue* respos = callinfo->top;                                            \
      KlString* op = state->common->string.neq;                                   \
      KlException exception = klexec_dobinopmethod(state, respos, (a), (b), op);  \
      if (kl_unlikely(exception)) return exception;                               \
      if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */       \
        callinfo->savedpc -= 1; /* redo condition jump later */                   \
        KlValue* newbase = state->callinfo->base;                                 \
        klexec_updateglobal(newbase);                                             \
      } else {  /* C function or C closure */                                     \
        /* stack may have grown. restore stkbase. */                              \
        stkbase = callinfo->base;                                                 \
        if (klexec_if(callinfo->top) == cond) pc += offset;                       \
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

#define klexec_getfieldgeneric(dotable, key, val) {                               \
  /* this is ensured by klang compiler */                                         \
  kl_assert(klvalue_checktype(key, KL_STRING), "expected string to index field"); \
                                                                                  \
  KlString* keystr = klvalue_getobj(key, KlString*);                              \
  if (klvalue_dotable(dotable)) {                                                 \
    /* values with type KL_OBJECT(including map and array). */                    \
    KlObject* object = klvalue_getobj(dotable, KlObject*);                        \
    KlValue* field = klobject_getfield(object, keystr);                           \
    kl_likely(field != NULL) ? klvalue_setvalue(val, field) : klvalue_setnil(val);\
  } else {  /* other types. search their phony class */                           \
    KlClass* phony = klvalue_checktype(dotable, KL_CLASS)                         \
      ? klvalue_getobj(dotable, KlClass*)                                         \
      : state->common->klclass.phony[klvalue_gettype(dotable)];                   \
    /* phony class should have only shared field */                               \
    KlClassCeil* ceil = klclass_findshared(phony, keystr);                        \
    ceil != NULL ? klvalue_setvalue(val, &ceil->value) : klvalue_setnil(val);     \
  }                                                                               \
}

#define klexec_setfieldgeneric(dotable, key, val) {                               \
  /* this is ensured by klang compiler */                                         \
  kl_assert(klvalue_checktype(key, KL_STRING), "expected string to index field"); \
                                                                                  \
  KlString* keystr = klvalue_getobj(key, KlString*);                              \
  if (klvalue_dotable(dotable)) {                                                 \
    /* values with type KL_OBJECT(including map and array). */                    \
    KlObject* object = klvalue_getobj(dotable, KlObject*);                        \
    KlValue* field = klobject_getfield(object, keystr);                           \
    if (field) {                                                                  \
      klvalue_setvalue(field, val);                                               \
    } else {                                                                      \
      KlClass* klclass = klobject_class(object);                                  \
      KlException exception = klclass_newshared(klclass, keystr, val);            \
      if (kl_unlikely(exception))                                                 \
        return klexec_handle_setfield_exception(state, exception, keystr);        \
    }                                                                             \
  } else {  /* other types. search their phony class */                           \
    KlClass* phony = klvalue_checktype(dotable, KL_CLASS)                         \
      ? klvalue_getobj(dotable, KlClass*)                                         \
      : state->common->klclass.phony[klvalue_gettype(dotable)];                   \
    klexec_savestate(callinfo->top, callinfo);  /* may add new field */           \
    KlException exception = klclass_newshared(phony, keystr, val);                \
    if (kl_unlikely(exception))                                                   \
      return klexec_handle_setfield_exception(state, exception, keystr);          \
  }                                                                               \
}

KlException klexec_execute(KlState* state) {
  kl_assert(state->callinfo->status & KLSTATE_CI_STATUS_KCLO, "expected klang closure in klexec_execute()");

  KlCallInfo* callinfo = state->callinfo;
  KlKClosure* closure = klcast(KlKClosure*, callinfo->callable.clo);
  KlInstruction* pc = callinfo->savedpc;
  KlValue* constants = klkfunc_constants(closure->kfunc);
  KlValue* stkbase = callinfo->top - klkfunc_framesize(closure->kfunc);
  kl_assert(callinfo->top >= klstate_stktop(state), "stack size not enough");

  while (true) {
    KlInstruction inst = *pc++;
    uint8_t opcode = KLINST_GET_OPCODE(inst);
    switch (opcode) {
      case KLOPCODE_MOVE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        klvalue_setvalue(a, b);
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
      case KLOPCODE_CONCAT: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_savestate(callinfo->top, callinfo);
        if (kl_likely(klvalue_checktype((b), KL_STRING) && klvalue_checktype((c), KL_STRING))) {
          KlString* res = klstrpool_string_concat(state->strpool, klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
          if (kl_unlikely(!res))
            return klstate_throw(state, KL_E_OOM, "out of memory when do concat");
          klvalue_setobj(a, res, KL_STRING);
        } else {
          KlString* op = state->common->string.concat;
          KlException exception = klexec_dobinopmethod(state, a, b, c, op);
          if (kl_unlikely(exception)) return exception;
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = callinfo->top;
            klexec_updateglobal(newbase);
          } else {  /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->top - klkfunc_framesize(closure->kfunc);
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
        KlInt c = KLINST_ABI_GETI(inst);
        if (kl_unlikely(c == 0))
          return klstate_throw(state, KL_E_ZERO, "divided by zero");
        klexec_binop_i(div, a, b, c);
        break;
      }
      case KLOPCODE_MODI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        if (kl_unlikely(c == 0))
          return klstate_throw(state, KL_E_ZERO, "divided by zero");
        klexec_binmod_i(mod, a, b, c);
        break;
      }
      case KLOPCODE_ADDC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(add, a, b, c);
        break;
      }
      case KLOPCODE_SUBC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(sub, a, b, c);
        break;
      }
      case KLOPCODE_MULC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIVC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_bindiv(div, a, b, c);
        break;
      }
      case KLOPCODE_MODC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binmod(mod, a, b, c);
        break;
      }
      case KLOPCODE_NEG: {
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        if (kl_likely(klvalue_checktype(b, KL_INT))) {
          klvalue_setint(stkbase + KLINST_ABC_GETA(inst), -klvalue_getint(b));
        } else {
          klexec_savestate(callinfo->top, callinfo);
          KlException exception = klexec_dopreopmethod(state, stkbase + KLINST_ABC_GETA(inst), b, state->common->string.neg);
          if (kl_unlikely(exception)) return exception;
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = callinfo->top;
            klexec_updateglobal(newbase);
          } else {  /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->top - klkfunc_framesize(closure->kfunc);
          }
        }
        break;
      }
      case KLOPCODE_CALL: {
        KlValue* callable = stkbase + KLINST_AXY_GETA(inst);
        size_t narg = KLINST_AXY_GETX(inst);
        size_t nret = KLINST_AXY_GETY(inst);
        KlValue* argbase = callable + 1;
        klexec_savestate(argbase + narg, callinfo);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, -1);
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
        newci->status = KLSTATE_CI_STATUS_NORM;
        KlException exception = klexec_callprepare(state, callable, narg, klexec_callprep_callback_for_call);
        if (kl_unlikely(exception)) return exception;
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = state->callinfo->base;
          klexec_updateglobal(newbase);
        } else {  /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_METHOD: {
        kl_assert(klvalue_checktype(constants + KLINST_AX_GETX(inst), KL_STRING), "field name should be a string");

        KlValue* thisobj = stkbase + KLINST_AX_GETA(inst);
        KlString* field = klvalue_getobj(constants + KLINST_AX_GETX(inst), KlString*);
        KlInstruction extra = *pc++;
        kl_assert(KLINST_GET_OPCODE(extra) == KLOPCODE_EXTRA, "something wrong in code generation");
        size_t narg = KLINST_XYZ_GETX(extra);
        size_t nret = KLINST_XYZ_GETY(extra);
        bool ismethod = klexec_is_method(thisobj);
        KlValue* argbase = ismethod ? thisobj : thisobj + 1;
        if (ismethod) ++narg;

        klexec_savestate(argbase + narg, callinfo);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, ismethod ? 0 : -1);
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when call a callable object");
        KlValue* callable = klexec_getfield(state, thisobj, field);
        if (kl_unlikely(!callable))
          return klstate_throw(state, KL_E_INVLD, "can not find method named %s", field);
        KlException exception = klexec_callprepare(state, callable, narg, klexec_callprep_callback_for_method);
        if (kl_unlikely(exception)) return exception;
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = state->callinfo->base;
          klexec_updateglobal(newbase);
        } else {  /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_RETURN: {
        size_t nret = callinfo->nret;
        KlValue* res = stkbase + KLINST_AX_GETA(inst);
        size_t nres = KLINST_AX_GETX(inst);
        size_t ncopy = nres < nret ? nres : nret;
        KlValue* retpos = stkbase + callinfo->retoff;
        while (ncopy--) /* copy results to their position. */
          klvalue_setvalue(retpos++, res++);
        if (nres < nret)  /* complete missing returned value */
          klexec_setnils(retpos, nret - nres);
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_RETURN0: {
        size_t nret = callinfo->nret;
        klexec_setnils(stkbase + callinfo->retoff, nret);
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_RETURN1: {
        size_t nret = callinfo->nret;
        KlValue* retpos = stkbase + callinfo->retoff;
        if (nret != 0) {  /* at least one results wanted */
          KlValue* res = stkbase + KLINST_A_GETA(inst);
          klvalue_setvalue(retpos, res);
          klexec_setnils(retpos + 1, nret - 1);   /* rest results is nil. */
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
        KlValue* c = constants + KLINST_AX_GETX(inst);
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
        KlMapIter itr = klmap_searchstring(state->global, varname);
        itr ? klvalue_setvalue(a, &itr->value) : klvalue_setnil(a);
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
        KlMapIter itr = klmap_searchstring(state->global, varname);
        if (kl_likely(itr)) {
          klvalue_setvalue(&itr->value, a);
        } else {
          klexec_savestate(callinfo->top, callinfo);
          if (kl_unlikely(!klmap_insertstring(state->global, varname, a)))
            return klstate_throw(state, KL_E_OOM, "out of memory when setting a global variable");
        }
        break;
      }
      case KLOPCODE_MKMAP: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* this instruction tells us current stack top */
        KlValue* stktop = stkbase + KLINST_ABX_GETB(inst);
        size_t capacity = KLINST_ABX_GETX(inst);
        klexec_savestate(stktop, callinfo); /* creating map may trigger gc */
        KlMap* map = klmap_create(state->common->klclass.map, capacity, state->mapnodepool);
        if (kl_unlikely(!map))
          return klstate_throw(state, KL_E_OOM, "out of memory when creating a map");
        klvalue_setobj(a, map, KL_MAP);
        break;
      }
      case KLOPCODE_MKARRAY: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* first value to be inserted to the array */
        KlValue* first = stkbase + KLINST_ABX_GETB(inst);
        size_t nelem = KLINST_ABX_GETX(inst);
        /* now stack top is first + nelem */
        klexec_savestate(first + nelem, callinfo);  /* creating array may trigger gc */
        KlArray* arr = klarray_create(state->common->klclass.array, nelem);
        if (kl_unlikely(!arr))
          return klstate_throw(state, KL_E_OOM, "out of memory when creating an array");
        KlArrayIter iter = klarray_iter_begin(arr);
        while(nelem--) {  /* fill this array with values on stack */
          klvalue_setvalue(iter, first++);
          iter = klarray_iter_next(iter);
        }
        klvalue_setobj(a, arr, KL_ARRAY);
        break;
      }
      case KLOPCODE_APPEND: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* first value to be inserted to the array */
        KlValue* first = stkbase + KLINST_ABX_GETB(inst);
        size_t nelem = KLINST_ABX_GETX(inst);
        /* now stack top is first + nelem */
        klexec_savestate(first + nelem, callinfo);  /* creating array may trigger gc */
        if (kl_likely(klvalue_checktype(a, KL_ARRAY))) {
          klarray_push_back(klvalue_getobj(a, KlArray*), first, nelem);
        } else {
          KlException exception = klexec_domultiargsmethod(state, a, a, nelem, state->common->string.append);
          if (kl_unlikely(exception)) return exception;
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {  /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_MKCLASS: {
        KlValue* a = stkbase + KLINST_ABTX_GETA(inst);
        /* this instruction tells us current stack top */
        KlValue* stktop = stkbase + KLINST_ABTX_GETB(inst);
        size_t capacity = KLINST_ABTX_GETX(inst);
        KlClass* klclass = NULL;
        if (KLINST_ABTX_GETT(inst)) { /* is stktop base class ? */
          if (kl_unlikely(!klvalue_checktype(stktop, KL_CLASS))) {
            return klstate_throw(state, KL_E_OOM, "inherit a non-class value, type: ", klvalue_typename(klvalue_gettype(stktop)));
          }
          klexec_savestate(stktop + 1, callinfo);   /* creating class may trigger gc */
          klclass = klclass_inherit(klstate_getmm(state), klvalue_getobj(stktop, KlClass*));
        } else {  /* this class has no base */
          klexec_savestate(stktop, callinfo); /* creating class may trigger gc */
          klclass = klclass_create(klstate_getmm(state), capacity, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
        }
        if (kl_unlikely(!klclass))
          return klstate_throw(state, KL_E_OOM, "out of memory when creating a class");
        klvalue_setobj(a, klclass, KL_CLASS);
        break;
      }
      case KLOPCODE_INDEXI: {
        KlValue* val = stkbase + KLINST_ABI_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABI_GETB(inst);
        KlInt index = KLINST_ABI_GETI(inst);
        if (klvalue_checktype(indexable, KL_ARRAY)) {       /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          klarray_index(arr, index, val);
        } else {
          KlValue key;
          klvalue_setint(&key, index);
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            KlMapIter itr = klmap_search(map, &key);
            itr ? klvalue_setvalue(val, &itr->value) : klvalue_setnil(val);
          } 
          klexec_savestate(callinfo->top, callinfo);
          KlException exception = klexec_doindexmethod(state, val, indexable, &key);
          if (kl_unlikely(exception)) return exception;
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {  /* C function or C closure */
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
        if (klvalue_checktype(indexable, KL_ARRAY)) {         /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          KlException exception = klarray_indexas(arr, index, val);
          if (kl_unlikely(exception)) {
            KlValue key;
            klvalue_setint(&key, index);
            klexec_savestate(callinfo->top, callinfo);
            return klexec_handle_arrayindexas_exception(state, exception, arr, &key);
          }
        } else {
          KlValue key;
          klvalue_setint(&key, index);
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            KlMapIter itr = klmap_search(map, &key);
            if (itr) {
              klvalue_setvalue(&itr->value, val);
            } else {
              klexec_savestate(callinfo->top, callinfo);
              if (kl_unlikely(!klmap_insert(map, &key, val)))
                return klstate_throw(state, KL_E_OOM, "out of memory when inserting a k-v pair to a map");
            }
            break;
          }
          klexec_savestate(callinfo->top, callinfo);
          KlException exception = klexec_doindexasmethod(state, val, indexable, &key);
          if (kl_unlikely(exception)) return exception;
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {  /* C function or C closure */
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
        if (klvalue_checktype(indexable, KL_ARRAY)) {       /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          if (kl_unlikely(!klvalue_checktype(key, KL_INT))) { /* only integer can index array */
            klexec_savestate(callinfo->top, callinfo);
            return klstate_throw(state, KL_E_TYPE,
                                 "type error occurred when indexing an array: expected %s, got %s.",
                                 klvalue_typename(KL_INT), klvalue_typename(klvalue_gettype(key)));
          }
          klarray_index(arr, klvalue_getint(key), val);
        } else {
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            if (klvalue_canrawequal(key)) {
              KlMapIter itr = klmap_search(map, key);
              itr ? klvalue_setvalue(val, &itr->value) : klvalue_setnil(val);
              break;
            }
          }
          klexec_savestate(callinfo->top, callinfo);
          KlException exception = klexec_doindexmethod(state, val, indexable, key);
          if (kl_unlikely(exception)) return exception;
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {  /* C function or C closure */
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
        if (klvalue_checktype(indexable, KL_ARRAY)) {         /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          klexec_savestate(callinfo->top, callinfo);
          if (kl_unlikely(!klvalue_checktype(key, KL_INT))) { /* only integer can index array */
            return klstate_throw(state, KL_E_TYPE,
                                 "type error occurred when indexing an array: expected %s, got %s.",
                                 klvalue_typename(KL_INT), klvalue_typename(klvalue_gettype(key)));
          }
          KlException exception = klarray_indexas(arr, klvalue_getint(key), val);
          if (kl_unlikely(exception))
            return klexec_handle_arrayindexas_exception(state, exception, arr, key);
        } else {
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            if (klvalue_canrawequal(key)) { /* simple types that can apply raw equal. fast search and set */
              KlMapIter itr = klmap_search(map, key);
              if (itr) {
                klvalue_setvalue(&itr->value, val);
              } else {
                klexec_savestate(callinfo->top, callinfo);
                if (kl_unlikely(!klmap_insert(map, key, val)))
                  return klstate_throw(state, KL_E_OOM, "out of memory when inserting a k-v pair to a map");
              }
              break;
            } /* else fall through. try operator method */
          }
          klexec_savestate(callinfo->top, callinfo);
          KlException exception = klexec_doindexasmethod(state, val, indexable, key);
          if (kl_unlikely(exception)) return exception;
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {  /* C function or C closure */
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
        klexec_getfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_GETFIELDC: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        KlValue* key = constants + KLINST_ABX_GETX(inst);
        klexec_getfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_SETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        klexec_setfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_SETFIELDC: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        KlValue* key = constants + KLINST_ABX_GETX(inst);
        klexec_setfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_REFGETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* key = stkbase + KLINST_ABX_GETB(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_ABX_GETX(inst)));
        klexec_getfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_REFGETFIELDC: {
        KlValue* val = stkbase + KLINST_AXY_GETA(inst);
        KlValue* key = constants + KLINST_AXY_GETX(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_AXY_GETY(inst)));
        klexec_getfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_REFSETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* key = stkbase + KLINST_ABX_GETB(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_ABX_GETX(inst)));
        klexec_setfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_REFSETFIELDC: {
        KlValue* val = stkbase + KLINST_AXY_GETA(inst);
        KlValue* key = constants + KLINST_AXY_GETX(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_AXY_GETY(inst)));
        klexec_setfieldgeneric(dotable, key, val);
        break;
      }
      case KLOPCODE_NEWLOCAL: {
        KlValue* classval = stkbase + KLINST_AX_GETA(inst);
        KlValue* fieldname = constants + KLINST_AX_GETX(inst);
        /* these are ensured by klang compiler */
        kl_assert(klvalue_checktype(classval, KL_CLASS), "NEWLOCAL should applied to a class");
        kl_assert(klvalue_checktype(fieldname, KL_STRING), "expected string to index field");

        KlString* keystr = klvalue_getobj(fieldname, KlString*);
        KlClass* klclass = klvalue_getobj(classval, KlClass*);
        klexec_savestate(callinfo->top, callinfo);  /* add new field */
        KlException exception = klclass_newlocal(klclass, keystr);
        if (kl_unlikely(exception))
          return klexec_handle_newlocal_exception(state, exception, keystr);
        break;
      }
      case KLOPCODE_TRUEJMP: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        int offset = KLINST_XI_GETI(inst);
        if (klexec_if(a)) pc += offset;
        break;
      }
      case KLOPCODE_FALSEJMP: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        int offset = KLINST_XI_GETI(inst);
        if (!klexec_if(a)) pc += offset;
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
        int offset = KLINST_XI_GETI(inst);
        if (klexec_if(callinfo->top) == cond)
          pc += offset;
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
        KlValue* b = constants + KLINST_AX_GETX(inst);
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
        KlValue* b = constants + KLINST_AX_GETX(inst);
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
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(lt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GTC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(gt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_LEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(le, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
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
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (klvalue_checktype(a, KL_INT) && klvalue_getint(a) == imm)
          pc += offset;
        break;
      }
      case KLOPCODE_NEI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (!klvalue_checktype(a, KL_INT) || klvalue_getint(a) != imm)
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
      case KLOPCODE_CLOSE: {
        KlValue* bound = stkbase + KLINST_X_GETX(inst);
        klreflist_close(&state->reflist, bound, klstate_getmm(state));
        break;
      }
      case KLOPCODE_NEWOBJ: {
        KlValue* klclass = stkbase + KLINST_ABC_GETB(inst);
        if (kl_unlikely(!klvalue_checktype(klclass, KL_CLASS)))
          return klstate_throw(state, KL_E_TYPE, "%s is not a class", klvalue_typename(klvalue_gettype(klclass)));
        klexec_savestate(callinfo->top, callinfo);
        KlObject* object = klclass_new_object(klvalue_getobj(klclass, KlClass*));
        if (kl_unlikely(!object))
          return klstate_throw(state, KL_E_OOM, "out of memory when constructing object");
        klvalue_setobj(stkbase + KLINST_ABC_GETA(inst), object, KL_OBJECT);
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
        klexec_savestate(a + 3, callinfo);
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
        klexec_savestate(argbase + nret, callinfo);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when do generic for loop");
        newci->status = KLSTATE_CI_STATUS_NORM;
        KlException exception = klexec_callprepare(state, a, nret, NULL);
        if (kl_unlikely(exception)) return exception;
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = state->callinfo->base;
          klexec_updateglobal(newbase);
        } else {  /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_ASYNC: {
        KlValue* f = stkbase + KLINST_ABC_GETA(inst);
        klexec_savestate(callinfo->top, callinfo);
        if (kl_unlikely(klvalue_checktype(f, KL_KCLOSURE))) {
          return klstate_throw(state, KL_E_TYPE,
                               "async should be applied to a klang closure, got '%s'",
                               klvalue_typename(klvalue_gettype(f)));
        }
        KlState* costate = klco_create(state, klvalue_getobj(f, KlKClosure*));
        if (kl_unlikely(!costate)) return klstate_throw(state, KL_E_OOM, "out of memory when creating a coroutine");
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        klvalue_setobj(a, costate, KL_COROUTINE);
        break;
      }
      case KLOPCODE_YIELD: {
        if (kl_unlikely(!klco_valid(&state->coinfo)))   /* is this 'state' a valid coroutine? */
          return klstate_throw(state, KL_E_INVLD, "can not yield from outside a coroutine");
        KlValue* res = stkbase + KLINST_AXY_GETA(inst);
        size_t nres = KLINST_AXY_GETX(inst);
        klexec_savestate(res + nres, callinfo);
        klco_yield(&state->coinfo, res, nres, KLINST_AXY_GETY(inst));
        return KL_E_NONE;
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        break;
      }
    }
  }

  return KL_E_NONE;
}
