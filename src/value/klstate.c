#include "include/value/klstate.h"
#include "include/misc/klutils.h"
#include "include/vm/klexception.h"
#include "include/vm/klstack.h"
#include <stdarg.h>


static KlGCObject* propagate(KlState* state, KlMM* klmm, KlGCObject* gclist);
static void delete(KlState* state, KlMM* klmm);

static const KlGCVirtualFunc gcvfunc = { .propagate = (KlGCProp)propagate, .destructor = (KlGCDestructor)delete, .after = NULL };

static void correct_callinfo(KlState* state, ptrdiff_t diff);


KlState* klstate_create(KlMM* klmm, KlMap* global, KlCommon* common, KlStrPool* strpool, KlKClosure* kclo) {
  KlState* state = (KlState*)klmm_alloc(klmm, sizeof (KlState));
  if (!state) return NULL;

  if (kl_unlikely(!klstack_init(klstate_stack(state), klmm))) {
    klmm_free(klmm, state, sizeof (KlState));
    return NULL;
  }
  if (kl_unlikely(!(state->reflist = klreflist_create(klmm)))) {
    klstack_destroy(klstate_stack(state), klmm);
    klmm_free(klmm, state, sizeof (KlState));
    return NULL;
  }
  if (kl_unlikely(!klthrow_init(&state->throwinfo, klmm, 128))) {
    klreflist_delete(*klstate_reflist(state), klmm);
    klstack_destroy(klstate_stack(state), klmm);
    klmm_free(klmm, state, sizeof (KlState));
    return NULL;
  }
  klco_init(&state->coinfo, kclo);
  state->klmm = klmm;
  state->strpool = strpool;
  state->global = global;
  state->common = common;
  klcommon_pin(common);
  state->callinfo = NULL;
  state->callinfo = &state->baseci;
  state->baseci.callable.cfunc = NULL;
  state->baseci.next = NULL;
  state->baseci.prev = NULL;
  state->baseci.base = klstack_raw(klstate_stack(state));
  state->baseci.top = klstack_raw(klstate_stack(state));
  state->baseci.status = KLSTATE_CI_STATUS_NORM;

  klmm_gcobj_enable(klmm, klmm_to_gcobj(state), &gcvfunc);
  return state;
}

static void delete(KlState* state, KlMM* klmm) {
  klstack_destroy(klstate_stack(state), klmm);
  klreflist_delete(state->reflist, klmm);
  klthrow_destroy(&state->throwinfo, klmm);
  klcommon_unpin(state->common, klmm);
  KlCallInfo* ci = state->baseci.next;
  while (ci) {
    KlCallInfo* tmp = ci->next;
    klmm_free(klmm, ci, sizeof (KlCallInfo));
    ci = tmp;
  }
  klmm_free(klmm, state, sizeof (KlState));
}

static KlGCObject* propagate(KlState* state, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  gclist = klstack_propagate(klstate_stack(state), gclist);
  gclist = klcommon_propagate(state->common, gclist);
  gclist = klthrow_propagate(&state->throwinfo, gclist);
  gclist = klco_propagate(&state->coinfo, gclist);
  KlCallInfo* callinfo = state->callinfo;
  while (callinfo) {
    if ((callinfo->status & KLSTATE_CI_STATUS_KCLO) || callinfo->status & KLSTATE_CI_STATUS_CCLO)
      klmm_gcobj_mark(callinfo->callable.clo, gclist);
    callinfo = callinfo->prev;
  }

  klmm_gcobj_mark((KlGCObject*)state->global, gclist);
  klmm_gcobj_mark((KlGCObject*)state->strpool, gclist);
  return gclist;
}

static void correct_callinfo(KlState* state, ptrdiff_t diff) {
  if (diff == 0) return;
  KlCallInfo* callinfo = state->callinfo;
  while (callinfo) {
    callinfo->top += diff;
    callinfo->base += diff;
    callinfo = callinfo->prev;
  }
}

KlException klstate_growstack(KlState* state, size_t framesize) {
  KlValue* oristk = klstack_raw(klstate_stack(state));
  size_t expected_cap = klstack_size(klstate_stack(state)) + framesize;
  KlException exception = klstack_expand(klstate_stack(state), klstate_getmm(state), expected_cap);
  if (kl_unlikely(exception)) {
    if (exception == KL_E_OOM) {
      return klstate_throw_oom(state, "growing stack");
    } else {
      kl_assert(exception == KL_E_RANGE, "");
      return klstate_throw(state, KL_E_RANGE, "stack overflow");
    }

  }
  KlValue* newstk = klstack_raw(klstate_stack(state));
  ptrdiff_t diff = newstk - oristk;
  klreflist_correct(state->reflist, diff);
  correct_callinfo(state, diff);
  return KL_E_NONE;
}

KlException klstate_throw(KlState* state, KlException type, const char* format, ...) {
  va_list args;
  va_start(args, format);
  KlException exception = klthrow_internal(&state->throwinfo, type, format, args);
  va_end(args);
  return exception;
}

KlException klstate_throw_oom(KlState* state, const char* when) {
  return klstate_throw(state, KL_E_OOM, "out of memory when %s", when);
}

KlException klstate_throw_link(KlState* state, KlState* src) {
  return klthrow_link(&state->throwinfo, src);
}
