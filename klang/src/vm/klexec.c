#include "klang/include/value/klarray.h"
#include "klang/include/value/klcfunc.h"
#include "klang/include/value/klclass.h"
#include "klang/include/value/klclosure.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klexception.h"
#include "klang/include/vm/klexec.h"
#include "klang/include/vm/klinst.h"
#include "klang/include/vm/klstack.h"
#include <stddef.h>
#include <string.h>


/* extra stack frame size for calling operator method */
#define KLEXEC_STACKFRAME_EXTRA             (2)

static KlException klexec_iforprep(KlState* state, KlValue* ctrlvars, int offset);

KlException klexec_hashgeneric(KlState* state, KlValue* key, size_t* hash) {
  switch (klvalue_gettype(key)) {
    case KL_STRING: {
      *hash = klstring_hash(klvalue_getobj(key, KlString*));
      return KL_E_NONE;
    }
    case KL_INT: {
      KlInt val = klvalue_getint(key);
      *hash = val ^ (val >> sizeof (KlInt));
      return KL_E_NONE;
    }
    case KL_BOOL: {
      *hash = klvalue_getbool(key);
      return KL_E_NONE;
    }
    case KL_NIL: {
      *hash = 0;
      return KL_E_NONE;
    }
    case KL_CFUNCTION: {
      size_t addr = (size_t)klvalue_getcfunc(key);
      *hash = addr ^ (addr >> sizeof (KlCFunction*));
      return KL_E_NONE;
    }
    case KL_MAP:
    case KL_ARRAY:
    case KL_OBJECT: {
      KlObject* object = klvalue_getobj(key, KlObject*);
      KlValue* hashmethod = klobject_getfield(object, state->common->string.hash);
      if (!hashmethod) {
        size_t addr = (size_t)klvalue_getgcobj(key);
        *hash = addr ^ (addr >> sizeof (KlGCObject*));
        return KL_E_NONE;
      }
      return klexec_callophash(state, hash, key, hashmethod);
    }
    default: {
      size_t addr = (size_t)klvalue_getgcobj(key);
      *hash = addr ^ (addr >> sizeof (KlGCObject*));
      return KL_E_NONE;
    }
  }

  kl_assert(false, "control flow should not reach here");
}

/* res must be stack value */
KlException klexec_mapsearch(KlState* state, KlMap* map, KlValue* key, KlValue* res) {
  size_t hash = 0;
  ptrdiff_t resdiff = klexec_savestack(state, res);
  ptrdiff_t keydiff = klexec_savestack(state, key);
  KlException exception = klexec_hashgeneric(state, key, &hash);
  if (kl_unlikely(exception)) return exception;
  size_t mask = klmap_mask(map);
  size_t index = mask & hash;
  KlMapIter itr = klmap_bucket(map, index);
  if (!itr) {
    klvalue_setnil(klexec_restorestack(state, resdiff));
    return KL_E_NONE;
  }
  KlMapIter end = klmap_iter_end(map);
  do {
    bool equal;
    KlException exception = klexec_equalgeneric(state, klexec_restorestack(state, keydiff), &itr->key, &equal);
    if (kl_unlikely(exception)) return exception;
    if (equal) {
      klvalue_setvalue(klexec_restorestack(state, resdiff), &itr->value);
      return KL_E_NONE;
    }
    itr = klmap_iter_next(itr);
  } while (itr != end && klmap_inbucket(map, itr, mask, index));
  klvalue_setnil(klexec_restorestack(state, resdiff));
  return KL_E_NONE;
}

/* 'val' must be stack value */
KlException klexec_mapinsert(KlState* state, KlMap* map, KlValue* key, KlValue* val) {
  size_t hash = 0;
  ptrdiff_t valdiff = klexec_savestack(state, val);
  ptrdiff_t keydiff = klexec_savestack(state, key);
  KlException exception = klexec_hashgeneric(state, key, &hash);
  if (kl_unlikely(exception)) return exception;
  size_t mask = klmap_mask(map);
  size_t index = mask & hash;
  KlMapIter itr = klmap_bucket(map, index);
  if (!itr) {
    if (kl_unlikely(!klmap_bucketinsert(map, index, klexec_restorestack(state, keydiff), klexec_restorestack(state, valdiff), hash)))
      return klstate_throw(state, KL_E_OOM, "out of memory when inserting value to a map");
    return KL_E_NONE;
  }
  KlMapIter end = klmap_iter_end(map);
  do {
    bool equal;
    KlException exception = klexec_equalgeneric(state, klexec_restorestack(state, keydiff), &itr->key, &equal);
    if (kl_unlikely(exception)) return exception;
    if (equal) {
      klvalue_setvalue(&itr->value, klexec_restorestack(state, valdiff));
      return KL_E_NONE;
    }
    itr = klmap_iter_next(itr);
  } while (itr != end && klmap_inbucket(map, itr, mask, index));
  if (kl_unlikely(klmap_iter_insert(map, itr, klexec_restorestack(state, keydiff), klexec_restorestack(state, valdiff), hash)))
    return klstate_throw(state, KL_E_OOM,  "out of memory when inserting value to a map");
  return KL_E_NONE;
}


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
  if (exception == KL_E_OOM) {
    return klstate_throw(state, exception, "out of memory when indexing an array");
  } else if (exception == KL_E_RANGE) {
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

KlException klexec_dobinopmethodi(KlState* state, KlValue* a, KlValue* b, KlInt c, KlString* op) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_residual(klstate_stack(state)) >= 2, "stack size not enough for calling binary operator");

  klstack_pushvalue(klstate_stack(state), b);
  klstack_pushint(klstate_stack(state), c);
  ptrdiff_t adiff = klexec_savestack(state, a);
  KlException exception = klexec_callbinopmethod(state, op);
  if (kl_unlikely(exception)) return exception;
  klvalue_setvalue(klexec_restorestack(state, adiff), klstate_getval(state, -1));
  return KL_E_NONE;
}

KlException klexec_dobinopmethod(KlState* state, KlValue* a, KlValue* b, KlValue* c, KlString* op) {
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  kl_assert(klstack_residual(klstate_stack(state)) >= 2, "stack size not enough for calling binary operator");

  klstack_pushvalue(klstate_stack(state), b);
  klstack_pushvalue(klstate_stack(state), c);
  ptrdiff_t adiff = klexec_savestack(state, a);
  KlException exception = klexec_callbinopmethod(state, op);
  if (kl_unlikely(exception)) return exception;
  klvalue_setvalue(klexec_restorestack(state, adiff), klstate_getval(state, -1));
  return KL_E_NONE;
}

KlException klexec_callc(KlState* state, KlCFunction* cfunc, size_t narg, size_t nret) {
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

/* Prepare for calling a callable object (C function, C closure, klang closure).
 * Also perform the actual call for C function and C closure.
 */
KlException klexec_callprepare(KlState* state, KlCallInfo* callinfo, KlValue* callable, size_t narg) {
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
    /* fill callinfo, newci->narg is not necessary for klang closure. */
    callinfo->callable.clo = klmm_to_gcobj(kclo);
    callinfo->status = KLSTATE_CI_STATUS_KCLO;
    callinfo->savedpc = klkfunc_entrypoint(kfunc);
    callinfo->top = klstate_stktop(state) + framesize - narg;
    klexec_push_callinfo(state);
    return KL_E_NONE;
  } else if (kl_likely(klvalue_checktype(callable, KL_CFUNCTION))) {  /* is a C function ? */
    KlCFunction* cfunc = klvalue_getcfunc(callable);
    callinfo->callable.cfunc = cfunc;
    callinfo->status = KLSTATE_CI_STATUS_CFUN;
    callinfo->narg = narg;
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
    klexec_push_callinfo(state);
    /* do the call */
    KlException exception = klexec_callc(state, cclo->cfunc, narg, callinfo->nret);
    klexec_pop_callinfo(state);
    return exception;
  } else {
    /* try opmethod */
    KlValue* method = klexec_getfield(state, callable, state->common->string.call);
    if (kl_unlikely(!method || !klvalue_callable(method))) {
      return klstate_throw(state, KL_E_INVLD, "try to call a non-callable object");
    }
    klvalue_setvalue(&callinfo->env_this, callable);
    /* redo the preparation */
    return klexec_methodprepare(state, callinfo, method, narg);
  }
}

/* Prepare for calling a method (C function, C closure, klang closure) with 'this'.
 * Also perform the actual call for C function and C closure.
 */
KlException klexec_methodprepare(KlState* state, KlCallInfo* callinfo, KlValue* method, size_t narg) {
  if (kl_likely(klvalue_checktype(method, KL_KCLOSURE))) {          /* is a klang closure ? */
    KlKClosure* kclo = klvalue_getobj(method, KlKClosure*);
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
    /* fill callinfo, newci->narg is not necessary for klang closure. */
    callinfo->callable.clo = klmm_to_gcobj(kclo);
    callinfo->status = KLSTATE_CI_STATUS_KCLO;
    callinfo->savedpc = klkfunc_entrypoint(kfunc);
    callinfo->top = klstate_stktop(state) + framesize - narg;
    klexec_push_callinfo(state);
    return KL_E_NONE;
  } else if (kl_likely(klvalue_checktype(method, KL_CFUNCTION))) {  /* is a C function ? */
    KlCFunction* cfunc = klvalue_getcfunc(method);
    callinfo->callable.cfunc = cfunc;
    callinfo->status = KLSTATE_CI_STATUS_CFUN;
    callinfo->narg = narg;
    klexec_push_callinfo(state);
    /* do the call */
    KlException exception = klexec_callc(state, cfunc, narg, callinfo->nret);
    klexec_pop_callinfo(state);
    return exception;
  } else if (kl_likely(klvalue_checktype(method, KL_CCLOSURE))) {   /* is a C closure ? */
    KlCClosure* cclo = klvalue_getobj(method, KlCClosure*);
    callinfo->callable.clo = klmm_to_gcobj(cclo);
    callinfo->status = KLSTATE_CI_STATUS_CCLO;
    callinfo->narg = narg;
    klexec_push_callinfo(state);
    /* do the call */
    KlException exception = klexec_callc(state, cclo->cfunc, narg, callinfo->nret);
    klexec_pop_callinfo(state);
    return exception;
  } else {
    /* try opmethod */
    KlValue* opmethod = klexec_getfield(state, method, state->common->string.call);
    if (kl_unlikely(!opmethod || !klvalue_callable(opmethod))) {
      return klstate_throw(state, KL_E_INVLD, "try to call a non-callable object");
    }
    klvalue_setvalue(&callinfo->env_this, method);
    /* redo the preparation */
    return klexec_methodprepare(state, callinfo, opmethod, narg);
  }
}

KlException klexec_concat(KlState* state, size_t nparam) {
  while (nparam-- != 1) {
    KlValue* l = klstate_getval(state, -2);
    KlValue* r = klstate_getval(state, -1);
    if (klvalue_checktype(l, KL_STRING) && klvalue_checktype(r, KL_STRING)) {
      KlString* res = klstrpool_string_concat(state->strpool, klvalue_getobj(l, KlString*), klvalue_getobj(r, KlString*));
      if (kl_unlikely(!res)) return klstate_throw(state, KL_E_OOM, "out of memory when do concat");
      klvalue_setobj(l, res, KL_STRING);
      klstack_move_top(klstate_stack(state), -1);
    } else {
      KlException exception = klexec_callbinopmethod(state, state->common->string.concat);
      if (kl_unlikely(exception)) return exception;
    }
  }
  return KL_E_NONE;
}

KlException klexec_equal(KlState* state, KlValue* a, KlValue* b) {
  kl_assert(!klvalue_checktype(a, KL_INT) && !klvalue_checktype(a, KL_STRING), "something wrong in instruction BE");
  kl_assert(!klvalue_checktype(b, KL_INT) && !klvalue_checktype(b, KL_STRING), "something wrong in instruction BE");

  KlString* eq = state->common->string.eq;
  KlValue* method = klexec_getfield(state, a, eq);
  if (!method) method = klexec_getfield(state, b, eq);
  if (!method) {
    klstack_pushbool(klstate_stack(state), KL_FALSE);
    return KL_E_NONE;
  }
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  klstack_pushvalue(klstate_stack(state), a);
  klstack_pushvalue(klstate_stack(state), b);
  KlException exception = klexec_call(state, method, 2, 1);
  if (kl_unlikely(exception)) return exception;
  return KL_E_NONE;
}

KlException klexec_notequal(KlState* state, KlValue* a, KlValue* b) {
  kl_assert(!klvalue_checktype(a, KL_INT) && !klvalue_checktype(a, KL_STRING), "something wrong in instruction BNE");
  kl_assert(!klvalue_checktype(b, KL_INT) && !klvalue_checktype(b, KL_STRING), "something wrong in instruction BNE");

  KlString* neq = state->common->string.neq;
  KlValue* method = klexec_getfield(state, a, neq);
  if (!method) method = klexec_getfield(state, b, neq);
  if (!method) {
    klstack_pushbool(klstate_stack(state), KL_TRUE);
    return KL_E_NONE;
  }
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  klstack_pushvalue(klstate_stack(state), a);
  klstack_pushvalue(klstate_stack(state), b);
  KlException exception = klexec_call(state, method, 2, 1);
  if (kl_unlikely(exception)) return exception;
  return KL_E_NONE;
}

KlException klexec_lt(KlState* state, KlValue* a, KlValue* b) {
  kl_assert(!klvalue_checktype(a, KL_INT) , "something wrong in instruction BLT");
  kl_assert(!klvalue_checktype(b, KL_INT) , "something wrong in instruction BLT");

  if (klvalue_checktype(a, KL_STRING) && klvalue_checktype(b, KL_STRING)) {
    if (klstring_compare(klvalue_getobj(a, KlString*), klvalue_getobj(b, KlString*)) < 0) {
      klstack_pushbool(klstate_stack(state), KL_TRUE);
    } else {
      klstack_pushbool(klstate_stack(state), KL_FALSE);
    }
  }
  KlString* lt = state->common->string.lt;
  KlValue* method = klexec_getfield(state, a, lt);
  if (!method) method = klexec_getfield(state, b, lt);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(lt));
  }
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  klstack_pushvalue(klstate_stack(state), a);
  klstack_pushvalue(klstate_stack(state), b);
  KlException exception = klexec_callbinopmethod(state, lt);
  if (kl_unlikely(exception)) return exception;
  return KL_E_NONE;
}

KlException klexec_le(KlState* state, KlValue* a, KlValue* b) {
  kl_assert(!klvalue_checktype(a, KL_INT) , "something wrong in instruction BLE");
  kl_assert(!klvalue_checktype(b, KL_INT) , "something wrong in instruction BLE");

  if (klvalue_checktype(a, KL_STRING) && klvalue_checktype(b, KL_STRING)) {
    if (klstring_compare(klvalue_getobj(a, KlString*), klvalue_getobj(b, KlString*)) <= 0) {
      klstack_pushbool(klstate_stack(state), KL_TRUE);
    } else {
      klstack_pushbool(klstate_stack(state), KL_FALSE);
    }
  }
  KlString* le = state->common->string.le;
  KlValue* method = klexec_getfield(state, a, le);
  if (!method) method = klexec_getfield(state, b, le);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(le));
  }
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  klstack_pushvalue(klstate_stack(state), a);
  klstack_pushvalue(klstate_stack(state), b);
  KlException exception = klexec_callbinopmethod(state, le);
  if (kl_unlikely(exception)) return exception;
  return KL_E_NONE;
}

KlException klexec_gt(KlState* state, KlValue* a, KlValue* b) {
  kl_assert(!klvalue_checktype(a, KL_INT) , "something wrong in instruction BGT");
  kl_assert(!klvalue_checktype(b, KL_INT) , "something wrong in instruction BGT");

  if (klvalue_checktype(a, KL_STRING) && klvalue_checktype(b, KL_STRING)) {
    if (klstring_compare(klvalue_getobj(a, KlString*), klvalue_getobj(b, KlString*)) > 0) {
      klstack_pushbool(klstate_stack(state), KL_TRUE);
    } else {
      klstack_pushbool(klstate_stack(state), KL_FALSE);
    }
  }
  KlString* gt = state->common->string.gt;
  KlValue* method = klexec_getfield(state, a, gt);
  if (!method) method = klexec_getfield(state, b, gt);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(gt));
  }
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  klstack_pushvalue(klstate_stack(state), a);
  klstack_pushvalue(klstate_stack(state), b);
  KlException exception = klexec_callbinopmethod(state, gt);
  if (kl_unlikely(exception)) return exception;
  return KL_E_NONE;
}

KlException klexec_ge(KlState* state, KlValue* a, KlValue* b) {
  kl_assert(!klvalue_checktype(a, KL_INT) , "something wrong in instruction BGE");
  kl_assert(!klvalue_checktype(b, KL_INT) , "something wrong in instruction BGE");

  if (klvalue_checktype(a, KL_STRING) && klvalue_checktype(b, KL_STRING)) {
    if (klstring_compare(klvalue_getobj(a, KlString*), klvalue_getobj(b, KlString*)) >= 0) {
      klstack_pushbool(klstate_stack(state), KL_TRUE);
    } else {
      klstack_pushbool(klstate_stack(state), KL_FALSE);
    }
  }
  KlString* ge = state->common->string.ge;
  KlValue* method = klexec_getfield(state, a, ge);
  if (!method) method = klexec_getfield(state, b, ge);
  if (kl_unlikely(!method)) {
    return klstate_throw(state, KL_E_INVLD, "can not find operator \'%s\'", klstring_content(ge));
  }
  /* Stack is guaranteed extra capacity for calling this operator.
   * See klexec_callprepare and klexec_methodprepare.
   */
  klstack_pushvalue(klstate_stack(state), a);
  klstack_pushvalue(klstate_stack(state), b);
  KlException exception = klexec_callbinopmethod(state, ge);
  if (kl_unlikely(exception)) return exception;
  return KL_E_NONE;
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

static inline KlException klexec_binop_add(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  return klexec_dobinopmethod(state, a, b, c, state->common->string.add);
}

static KlException klexec_binop_sub(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  return klexec_dobinopmethod(state, a, b, c, state->common->string.sub);
}

static KlException klexec_binop_mul(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  return klexec_dobinopmethod(state, a, b, c, state->common->string.mul);
}

static KlException klexec_binop_div(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  return klexec_dobinopmethod(state, a, b, c, state->common->string.div);
}

static KlException klexec_binop_mod(KlState* state, KlValue* a, KlValue* b, KlValue* c) {
  return klexec_dobinopmethod(state, a, b, c, state->common->string.mod);
}

static KlException klexec_binop_addi(KlState* state, KlValue* a, KlValue* b, KlInt c) {
  return klexec_dobinopmethodi(state, a, b, c, state->common->string.add);
}

static KlException klexec_binop_subi(KlState* state, KlValue* a, KlValue* b, KlInt c) {
  return klexec_dobinopmethodi(state, a, b, c, state->common->string.sub);
}

static KlException klexec_binop_muli(KlState* state, KlValue* a, KlValue* b, KlInt c) {
  return klexec_dobinopmethodi(state, a, b, c, state->common->string.mul);
}

static KlException klexec_binop_divi(KlState* state, KlValue* a, KlValue* b, KlInt c) {
  return klexec_dobinopmethodi(state, a, b, c, state->common->string.div);
}

static KlException klexec_binop_modi(KlState* state, KlValue* a, KlValue* b, KlInt c) {
  return klexec_dobinopmethodi(state, a, b, c, state->common->string.mod);
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


#define klexec_intbinop(op, a, b, c) {                                            \
  if (kl_likely(klvalue_checktype((b), KL_INT) &&                                 \
                klvalue_checktype((c), KL_INT))) {                                \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), klvalue_getint((c))));     \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    ptrdiff_t stkdiff = klexec_savestack(state, stkbase);  /* stack may grow */   \
    KlException exception = klexec_binop_##op(state, (a), (b), (c));              \
    stkbase = klexec_restorestack(state, stkdiff);                                \
    if (kl_unlikely(exception)) return exception;                                 \
  }                                                                               \
}

#define klexec_intbinopnon0(op, a, b, c) {                                        \
  if (kl_likely(klvalue_checktype((b), KL_INT) &&                                 \
                klvalue_checktype((c), KL_INT))) {                                \
    KlInt cval = klvalue_getint((c));                                             \
    if (kl_unlikely(cval == 0)) {                                                 \
      return klstate_throw(state, KL_E_ZERO, "divided by zero");                  \
    }                                                                             \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), cval));                    \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    ptrdiff_t stkdiff = klexec_savestack(state, stkbase);  /* stack may grow */   \
    KlException exception = klexec_binop_##op(state, (a), (b), (c));              \
    stkbase = klexec_restorestack(state, stkdiff);                                \
    if (kl_unlikely(exception)) return exception;                                 \
  }                                                                               \
}

#define klexec_intbinop_i(op, a, b, imm) {                                        \
  if (kl_likely(klvalue_checktype((b), KL_INT))) {                                \
    klvalue_setint((a), klop_##op(klvalue_getint((b)), (imm)));                   \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    ptrdiff_t stkdiff = klexec_savestack(state, stkbase);  /* stack may grow */   \
    KlException exception = klexec_binop_##op##i(state, (a), (b), (imm));         \
    stkbase = klexec_restorestack(state, stkdiff);                                \
    if (kl_unlikely(exception)) return exception;                                 \
  }                                                                               \
}

#define klexec_border(order, a, b, offset) {                                      \
  if (klvalue_checktype((a), KL_INT) && klvalue_checktype((b), KL_INT)) {         \
    if (klorder_##order(klvalue_getint((a)), klvalue_getint((b))))                \
      pc += offset;                                                               \
  } else {                                                                        \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    ptrdiff_t stkdiff = klexec_savestack(state, stkbase);                         \
    KlException exception = klexec_##order(state, (a), (b));                      \
    if (kl_unlikely(exception)) return exception;                                 \
    if (klexec_if(klstate_getval(state, -1)))                                     \
      pc += offset;                                                               \
    stkbase = klexec_restorestack(state, stkdiff);                                \
  }                                                                               \
}

#define klexec_borderc(order, a, b, offset) {                                     \
  kl_assert(klvalue_canrawequal((b)), "something wrong in order instruction");    \
                                                                                  \
  if (klvalue_checktype((a), KL_INT)) {                                           \
    if (klorder_##order(klvalue_getint((a)), klvalue_getint((b))))                \
      pc += offset;                                                               \
  } else if (klvalue_checktype((a), KL_STRING)) {                                 \
    KlString* str1 = klvalue_getobj((a), KlString*);                              \
    KlString* str2 = klvalue_getobj((b), KlString*);                              \
    if (klorder_str##order(str1, str2))                                           \
      pc += offset;                                                               \
  }                                                                               \
}

#define klexec_borderi(order, a, imm, offset) {                                   \
  if (klvalue_checktype((a), KL_INT)) {                                           \
    if (klorder_##order(klvalue_getint((a)), (imm)))                              \
      pc += offset;                                                               \
  } else {                                                                        \
    KlValue tmpval;                                                               \
    klvalue_setint(&tmpval, imm);                                                 \
    klexec_savestate(callinfo->top, callinfo); /* in case of error and gc */      \
    ptrdiff_t stkdiff = klexec_savestack(state, stkbase);                         \
    KlException exception = klexec_##order(state, (a), &tmpval);                  \
    if (kl_unlikely(exception)) return exception;                                 \
    if (klexec_if(klstate_getval(state, -1)))                                     \
      pc += offset;                                                               \
    stkbase = klexec_restorestack(state, stkdiff);                                \
  }                                                                               \
}

#define klexec_bequal(a, b, offset) {                                             \
  if (kl_likely(klvalue_sametype((a), (b)))) {                                    \
    if (klvalue_sameinstance((a), (b))) {                                         \
      pc += offset;                                                               \
    } else if (!klvalue_canrawequal((a))) {                                       \
      klexec_savestate(callinfo->top, callinfo);                                  \
      ptrdiff_t stkdiff = klexec_savestack(state, stkbase);                       \
      KlException exception = klexec_equal(state, (a), (b));                      \
      if (kl_unlikely(exception)) return exception;                               \
      if (klexec_if(klstate_getval(state, -1)))                                   \
        pc += offset;                                                             \
      stkbase = klexec_restorestack(state, stkdiff);                              \
    }                                                                             \
  }                                                                               \
}

#define klexec_bnequal(a, b, offset) {                                            \
  if (kl_likely(klvalue_sametype((a), (b)))) {                                    \
    if (klvalue_sameinstance((a), (b))) {                                         \
      /* false, do nothing */                                                     \
    } else if (klvalue_canrawequal((a))) {                                        \
      pc += offset;                                                               \
    } else {                                                                      \
      klexec_savestate(callinfo->top, callinfo);                                  \
      ptrdiff_t stkdiff = klexec_savestack(state, stkbase);                       \
      KlException exception = klexec_notequal(state, (a), (b));                   \
      if (kl_unlikely(exception)) return exception;                               \
      if (klexec_if(klstate_getval(state, -1)))                                   \
        pc += offset;                                                             \
      stkbase = klexec_restorestack(state, stkdiff);                              \
    }                                                                             \
  } else {                                                                        \
    pc += offset;                                                                 \
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
    if (kl_unlikely(!field)) {                                                    \
      klexec_savestate(callinfo->top, callinfo);                                  \
      return klstate_throw(state, KL_E_INVLD,                                     \
          "no such field: %s",                                                    \
          klstring_content(keystr));                                              \
    }                                                                             \
    klvalue_setvalue(field, val);                                                 \
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

static inline KlException klexec_binop_add(KlState* state, KlValue* a, KlValue* b, KlValue* c);
static inline KlException klexec_binop_sub(KlState* state, KlValue* a, KlValue* b, KlValue* c);
static inline KlException klexec_binop_mul(KlState* state, KlValue* a, KlValue* b, KlValue* c);
static inline KlException klexec_binop_div(KlState* state, KlValue* a, KlValue* b, KlValue* c);
static inline KlException klexec_binop_mod(KlState* state, KlValue* a, KlValue* b, KlValue* c);
static inline KlException klexec_binop_addi(KlState* state, KlValue* a, KlValue* b, KlInt c);
static inline KlException klexec_binop_subi(KlState* state, KlValue* a, KlValue* b, KlInt c);
static inline KlException klexec_binop_muli(KlState* state, KlValue* a, KlValue* b, KlInt c);
static inline KlException klexec_binop_divi(KlState* state, KlValue* a, KlValue* b, KlInt c);
static inline KlException klexec_binop_modi(KlState* state, KlValue* a, KlValue* b, KlInt c);


KlException klexec_execute(KlState* state) {
  kl_assert(state->callinfo->status & KLSTATE_CI_STATUS_KCLO, "expected klang closure in klexec_execute()");

  KlCallInfo* callinfo = state->callinfo;
  KlKClosure* closure = klcast(KlKClosure*, callinfo->callable.clo);
  KlInstruction* pc = callinfo->savedpc;
  KlValue* constants = klkfunc_constants(closure->kfunc);
  KlValue* stkbase = callinfo->top - klkfunc_framesize(closure->kfunc);

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
        klexec_intbinop(add, a, b, c);
        break;
      }
      case KLOPCODE_SUB: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_intbinop(sub, a, b, c);
        break;
      }
      case KLOPCODE_MUL: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_intbinop(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIV: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_intbinopnon0(div, a, b, c);
        break;
      }
      case KLOPCODE_MOD: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_intbinopnon0(mod, a, b, c);
        break;
      }
      case KLOPCODE_ADDI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_intbinop_i(add, a, b, c);
        break;
      }
      case KLOPCODE_SUBI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_intbinop_i(sub, a, b, c);
        break;
      }
      case KLOPCODE_MULI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_intbinop_i(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIVI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        if (kl_unlikely(c == 0))
          return klstate_throw(state, KL_E_ZERO, "divided by zero");
        klexec_intbinop_i(div, a, b, c);
        break;
      }
      case KLOPCODE_MODI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        if (kl_unlikely(c == 0))
          return klstate_throw(state, KL_E_ZERO, "divided by zero");
        klexec_intbinop_i(mod, a, b, c);
        break;
      }
      case KLOPCODE_ADDC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_intbinop(add, a, b, c);
        break;
      }
      case KLOPCODE_SUBC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_intbinop(sub, a, b, c);
        break;
      }
      case KLOPCODE_MULC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_intbinop(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIVC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_intbinopnon0(div, a, b, c);
        break;
      }
      case KLOPCODE_MODC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_intbinopnon0(mod, a, b, c);
        break;
      }
      case KLOPCODE_CONCAT: {
        KlValue* first = stkbase + KLINST_ABX_GETB(inst);
        size_t nelem = KLINST_ABX_GETX(inst);
        klexec_savestate(first + nelem, callinfo);
        ptrdiff_t stkdiff = klexec_savestack(state, stkbase);
        KlException exception = klexec_concat(state, nelem);
        if (kl_unlikely(exception)) return exception;
        stkbase = klexec_restorestack(state, stkdiff);
        klvalue_setvalue(stkbase + KLINST_ABX_GETA(inst), klstate_stktop(state) - 1);
        break;
      }
      case KLOPCODE_NEG: {
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        if (kl_likely(klvalue_checktype(b, KL_INT))) {
          klvalue_setint(stkbase + KLINST_ABC_GETA(inst), -klvalue_getint(b));
        } else {
          klexec_savestate(callinfo->top, callinfo);
          ptrdiff_t stkdiff = klexec_savestack(state, stkbase);
          KlValue* method = klexec_getfield(state, b, state->common->string.neg);
          if (kl_unlikely(!method)) {
            return klstate_throw(state, KL_E_TYPE,
                                 "can not apply unary \'-\' to a value with type \'%s\'",
                                 klvalue_typename(klvalue_gettype(b)));
          }
          klstack_pushvalue(klstate_stack(state), b);
          KlException exception = klexec_call(state, method, 1, 1);
          if (kl_unlikely(exception)) return exception;
          stkbase = klexec_restorestack(state, stkdiff);
          klvalue_setvalue(stkbase + KLINST_ABC_GETA(inst), klstate_getval(state, -1));
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
        klvalue_setnil(&newci->env_this);   /* just a function call, no 'this' */
        ptrdiff_t argbasediff = klexec_savestack(state, argbase);
        KlException exception = klexec_callprepare(state, newci, callable, narg);
        if (kl_unlikely(exception)) return exception;
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          klexec_updateglobal(klexec_restorestack(state, argbasediff));
        } else {  /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->top - klkfunc_framesize(closure->kfunc);
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
        KlValue* argbase = thisobj + 1;

        klexec_savestate(argbase + narg, callinfo);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, -1);
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when call a callable object");
        klvalue_setvalue(&newci->env_this, thisobj);  /* set 'this' */
        KlValue* callable = klexec_getfield(state, thisobj, field);
        if (kl_unlikely(!callable))
          return klstate_throw(state, KL_E_INVLD, "can not find method named %s", field);
        ptrdiff_t argbasediff = klexec_savestack(state, argbase);
        KlException exception = klexec_methodprepare(state, newci, callable, narg);
        if (kl_unlikely(exception)) return exception;
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          klexec_updateglobal(klexec_restorestack(state, argbasediff) + 1);
        } else {  /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->top - klkfunc_framesize(closure->kfunc);
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
        klstack_set_top(klstate_stack(state), retpos);
        if (nres < nret)  /* complete missing returned value */
          klstack_pushnil(klstate_stack(state), nret - nres);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        klexec_pop_callinfo(state);
        klexec_updateglobal(state->callinfo->top - klkfunc_framesize(klcast(KlKClosure*, state->callinfo->callable.clo)->kfunc));
        break;
      }
      case KLOPCODE_RETURN0: {
        size_t nret = callinfo->nret;
        klstack_set_top(klstate_stack(state), stkbase + callinfo->retoff);
        klstack_pushnil(klstate_stack(state), nret);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        klexec_pop_callinfo(state);
        klexec_updateglobal(state->callinfo->top - klkfunc_framesize(klcast(KlKClosure*, state->callinfo->callable.clo)->kfunc));
        break;
      }
      case KLOPCODE_RETURN1: {
        size_t nret = callinfo->nret;
        if (nret != 0) {  /* at least one results wanted */
          KlValue* retpos = stkbase + callinfo->retoff;
          KlValue* res = stkbase + KLINST_A_GETA(inst);
          klvalue_setvalue(retpos, res);
          klstack_set_top(klstate_stack(state), retpos + 1);
          klstack_pushnil(klstate_stack(state), nret - 1); /* rest results is nil. */
        }
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        klexec_pop_callinfo(state);
        klexec_updateglobal(state->callinfo->top - klkfunc_framesize(klcast(KlKClosure*, state->callinfo->callable.clo)->kfunc));
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
        while (count--)
          klvalue_setnil(a++);
        break;
      }
      case KLOPCODE_LOADREF: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* ref = klref_getval(closure->refs[KLINST_AX_GETX(inst)]);
        klvalue_setvalue(a, ref);
        break;
      }
      case KLOPCODE_LOADTHIS: {
        KlValue* a = stkbase + KLINST_A_GETA(inst);
        klvalue_setvalue(a, &callinfo->env_this);
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
      case KLOPCODE_STORETHIS: {
        KlValue* a = stkbase + KLINST_A_GETA(inst);
        klvalue_setvalue(&callinfo->env_this, a);
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
        while(nelem--) {  /* fill this array with values in stack */
          klvalue_setvalue(iter, first++);
          iter = klarray_iter_next(iter);
        }
        klvalue_setobj(a, arr, KL_ARRAY);
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
        } else if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
          KlMap* map = klvalue_getobj(indexable, KlMap*);
          klexec_savestate(callinfo->top, callinfo);
          ptrdiff_t stkdiff = klexec_savestack(state, stkbase);
          KlException exception = klexec_mapsearch(state, map, key, val);
          if (kl_unlikely(exception)) return exception;
          stkbase = klexec_restorestack(state, stkdiff);
        } else {  /* else try opmethod */
          klexec_savestate(callinfo->top, callinfo);
          ptrdiff_t stkdiff = klexec_savestack(state, stkbase);
          KlException exception = klexec_callopindex(state, val, indexable, key);
          if (kl_unlikely(exception)) return exception;
          stkbase = klexec_restorestack(state, stkdiff);
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
        } else if (klvalue_checktype(indexable, KL_MAP)) {    /* is map? */
          KlMap* map = klvalue_getobj(indexable, KlMap*);
          klexec_savestate(callinfo->top, callinfo);
          ptrdiff_t stkdiff = klexec_savestack(state, stkbase);
          KlException exception = klexec_mapinsert(state, map, key, val);
          if (kl_unlikely(exception)) return exception;
          stkbase = klexec_restorestack(state, stkdiff);
        } else {  /* else try opmethod */
          klexec_savestate(callinfo->top, callinfo);
          ptrdiff_t stkdiff = klexec_savestack(state, stkbase);
          KlException exception = klexec_callopindexas(state, indexable, key, val);
          if (kl_unlikely(exception)) return exception;
          stkbase = klexec_restorestack(state, stkdiff);
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
      case KLOPCODE_THISSETFIELD: {
        KlValue* val = stkbase + KLINST_AX_GETA(inst);
        KlValue* key = constants + KLINST_AX_GETX(inst);
        KlValue* pthis = &callinfo->env_this;
        klexec_setfieldgeneric(pthis, key, val);
        break;
      }
      case KLOPCODE_THISGETFIELD: {
        KlValue* val = stkbase + KLINST_AX_GETA(inst);
        KlValue* key = constants + KLINST_AX_GETX(inst);
        KlValue* pthis = &callinfo->env_this;
        klexec_getfieldgeneric(pthis, key, val);
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
      case KLOPCODE_JMP: {
        int offset = KLINST_I_GETI(inst);
        pc += offset;
        break;
      }
      case KLOPCODE_BE: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        int offset = KLINST_ABI_GETI(inst);
        klexec_bequal(a, b, offset);
        break;
      }
      case KLOPCODE_BNE: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        int offset = KLINST_ABI_GETI(inst);
        klexec_bnequal(a, b, offset);
        break;
      }
      case KLOPCODE_BL: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        int offset = KLINST_ABI_GETI(inst);
        klexec_border(lt, a, b, offset);
        break;
      }
      case KLOPCODE_BG: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        int offset = KLINST_ABI_GETI(inst);
        klexec_border(gt, a, b, offset);
        break;
      }
      case KLOPCODE_BLE: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = constants + KLINST_ABI_GETB(inst);
        int offset = KLINST_ABI_GETI(inst);
        klexec_border(le, a, b, offset);
        break;
      }
      case KLOPCODE_BGE: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = constants + KLINST_ABI_GETB(inst);
        int offset = KLINST_ABI_GETI(inst);
        klexec_border(ge, a, b, offset);
        break;
      }
      case KLOPCODE_BEC: {
        KlValue* a = stkbase + KLINST_AXI_GETA(inst);
        KlValue* b = constants + KLINST_AXI_GETX(inst);
        int offset = KLINST_AXI_GETI(inst);
        kl_assert(klvalue_canrawequal(b), "something wrong in BEC");
        if (klvalue_equal(a, b))
          pc += offset;
        break;
      }
      case KLOPCODE_BNEC: {
        KlValue* a = stkbase + KLINST_AXI_GETA(inst);
        KlValue* b = constants + KLINST_AXI_GETX(inst);
        int offset = KLINST_AXI_GETI(inst);
        kl_assert(klvalue_canrawequal(b), "something wrong in BNEC");
        if (!klvalue_equal(a, b))
          pc += offset;
        break;
      }
      case KLOPCODE_BLC: {
        KlValue* a = stkbase + KLINST_AXI_GETA(inst);
        KlValue* b = constants + KLINST_AXI_GETX(inst);
        int offset = KLINST_AXI_GETI(inst);
        klexec_borderc(lt, a, b, offset);
        break;
      }
      case KLOPCODE_BGC: {
        KlValue* a = stkbase + KLINST_AXI_GETA(inst);
        KlValue* b = constants + KLINST_AXI_GETX(inst);
        int offset = KLINST_AXI_GETI(inst);
        klexec_borderc(gt, a, b, offset);
        break;
      }
      case KLOPCODE_BLEC: {
        KlValue* a = stkbase + KLINST_AXI_GETA(inst);
        KlValue* b = constants + KLINST_AXI_GETX(inst);
        int offset = KLINST_AXI_GETI(inst);
        klexec_borderc(le, a, b, offset);
        break;
      }
      case KLOPCODE_BGEC: {
        KlValue* a = stkbase + KLINST_AXI_GETX(inst);
        KlValue* b = constants + KLINST_AXI_GETX(inst);
        int offset = KLINST_AXI_GETI(inst);
        klexec_borderc(ge, a, b, offset);
        break;
      }
      case KLOPCODE_BEI: {
        KlValue* a = stkbase + KLINST_AIJ_GETA(inst);
        int imm = KLINST_AIJ_GETI(inst);
        int offset = KLINST_AIJ_GETJ(inst);
        if (klvalue_checktype(a, KL_INT) && klvalue_getint(a) == imm)
          pc += offset;
        break;
      }
      case KLOPCODE_BNEI: {
        KlValue* a = stkbase + KLINST_AIJ_GETA(inst);
        KlInt imm = KLINST_AIJ_GETI(inst);
        int offset = KLINST_AIJ_GETJ(inst);
        if (!klvalue_checktype(a, KL_INT) || klvalue_getint(a) != imm)
          pc += offset;
        break;
      }
      case KLOPCODE_BLI: {
        KlValue* a = stkbase + KLINST_AIJ_GETA(inst);
        KlInt imm = KLINST_AIJ_GETI(inst);
        int offset = KLINST_AIJ_GETJ(inst);
        klexec_borderi(lt, a, imm, offset);
        break;
      }
      case KLOPCODE_BGI: {
        KlValue* a = stkbase + KLINST_AIJ_GETA(inst);
        KlInt imm = KLINST_AIJ_GETI(inst);
        int offset = KLINST_AIJ_GETJ(inst);
        klexec_borderi(gt, a, imm, offset);
        break;
      }
      case KLOPCODE_BLEI: {
        KlValue* a = stkbase + KLINST_AIJ_GETA(inst);
        KlInt imm = KLINST_AIJ_GETI(inst);
        int offset = KLINST_AIJ_GETJ(inst);
        klexec_borderi(le, a, imm, offset);
        break;
      }
      case KLOPCODE_BGEI: {
        KlValue* a = stkbase + KLINST_AIJ_GETA(inst);
        KlInt imm = KLINST_AIJ_GETI(inst);
        int offset = KLINST_AIJ_GETJ(inst);
        klexec_borderi(ge, a, imm, offset);
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
          return klstate_throw(state, KL_E_TYPE, "expected a class, got %s", klvalue_typename(klvalue_gettype(klclass)));
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
        kl_assert(narg >= nparam, "something wrong in call(method)prepare");
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
        /* next instruction must be extra */
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "something wrong in code generation");

        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        size_t nret = KLINST_AX_GETX(inst);
        klexec_savestate(a + nret + 1, callinfo);
        ptrdiff_t stkdiff = klexec_savestack(state, stkbase);
        KlException exception = klexec_call(state, a, nret, nret);
        if (kl_unlikely(exception)) return exception;
        stkbase = klexec_restorestack(state, stkdiff);
        a = stkbase + KLINST_AX_GETA(inst);
        klvalue_checktype(a + 1, KL_NIL) ? ++pc : (pc += KLINST_I_GETI(*pc));
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
