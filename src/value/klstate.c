#include "include/value/klstate.h"
#include "include/misc/klutils.h"
#include "include/vm/klexception.h"
#include "include/vm/klstack.h"
#include <stdarg.h>


static KlGCObject* klstate_propagate(KlState* state, KlMM* klmm, KlGCObject* gclist);
static void klstate_delete(KlState* state, KlMM* klmm);

static KlGCVirtualFunc klstate_gcvfunc = { .propagate = (KlGCProp)klstate_propagate, .destructor = (KlGCDestructor)klstate_delete, .after = NULL};

static void klstate_correct_callinfo(KlState* state, ptrdiff_t diff);


KlState* klstate_create(KlMM* klmm, KlMap* global, KlCommon* common, KlStrPool* strpool, KlMapNodePool* mapnodepool, KlKClosure* kclo) {
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
  klthrow_init(&state->throwinfo, klmm, 128);
  klco_init(&state->coinfo, kclo);
  state->klmm = klmm;
  state->strpool = strpool;
  state->global = global;
  state->mapnodepool = mapnodepool;
  klmapnodepool_pin(mapnodepool);
  state->common = common;
  klcommon_pin(common);
  state->callinfo = NULL;
  state->callinfo = &state->baseci;
  state->baseci.callable.cfunc = NULL;
  state->baseci.next = NULL;
  state->baseci.prev = NULL;
  state->baseci.top = klstack_raw(klstate_stack(state));
  state->baseci.status = KLSTATE_CI_STATUS_NORM;

  klmm_gcobj_enable(klmm, klmm_to_gcobj(state), &klstate_gcvfunc);
  return state;
}

static void klstate_delete(KlState* state, KlMM* klmm) {
  klstack_destroy(klstate_stack(state), klmm);
  klreflist_delete(state->reflist, klmm);
  klthrow_destroy(&state->throwinfo, klmm);
  klmapnodepool_unpin(state->mapnodepool);
  klcommon_unpin(state->common, klmm);
  KlCallInfo* ci= state->baseci.next;
  while (ci) {
    KlCallInfo* tmp = ci->next;
    klmm_free(klmm, ci, sizeof (KlCallInfo));
    ci = tmp;
  }
  klmm_free(klmm, state, sizeof (KlState));
}

static KlGCObject* klstate_propagate(KlState* state, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  gclist = klstack_propagate(klstate_stack(state), gclist);
  gclist = klcommon_propagate(state->common, gclist);
  gclist = klthrow_propagate(&state->throwinfo, gclist);
  gclist = klco_propagate(&state->coinfo, gclist);
  KlCallInfo* callinfo = state->callinfo;
  while (callinfo) {
    if ((callinfo->status & KLSTATE_CI_STATUS_KCLO) || callinfo->status & KLSTATE_CI_STATUS_CCLO)
      klmm_gcobj_mark_accessible(callinfo->callable.clo, gclist);
    callinfo = callinfo->prev;
  }

  klmm_gcobj_mark_accessible((KlGCObject*)state->global, gclist);
  klmm_gcobj_mark_accessible((KlGCObject*)state->strpool, gclist);
  klmapnodepool_shrink(state->mapnodepool); /* shrink nodepool once for each iteration of gc */
  return gclist;
}

static void klstate_correct_callinfo(KlState* state, ptrdiff_t diff) {
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
  KlException exception = KL_E_NONE;
  do {
    if (kl_unlikely(!klstack_expand(klstate_stack(state), klstate_getmm(state)))) {
      exception = KL_E_OOM;
      break;
    }
  } while (klstack_capacity(klstate_stack(state)) <= expected_cap);
  KlValue* newstk = klstack_raw(klstate_stack(state));
  ptrdiff_t diff = newstk - oristk;
  klreflist_correct(state->reflist, diff);
  klstate_correct_callinfo(state, diff);
  return exception;
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
