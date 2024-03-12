#include "klang/include/value/klcoroutine.h"
#include "klang/include/mm/klmm.h"
#include "klang/include/value/klstate.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klexception.h"
#include "klang/include/vm/klexec.h"
#include "klang/include/vm/klstack.h"
#include <math.h>
#include <stddef.h>


static KlGCObject* klco_propagate(KlCoroutine* co, KlGCObject* gclist);
static void klco_delete(KlCoroutine* co);

static KlGCVirtualFunc klco_gcvfunc = { .destructor = (KlGCDestructor)klco_delete, .propagate = (KlGCProp)klco_propagate };


static inline void klco_setstatus(KlCoroutine* co, KlCoStatus status) {
  co->status = status;
}

KlCoroutine* klco_create(KlMM* klmm, KlKClosure* kclo, KlState* state) {
  KlCoroutine* co = (KlCoroutine*)klmm_alloc(klmm, sizeof (KlCoroutine));
  if (kl_unlikely(!co)) return NULL;
  klmm_newlevel(klmm, klmm_to_gcobj(co));
  KlState* newstate = klstate_create(klmm, klstate_global(state), klstate_common(state),
                                     klstate_strpool(state), klstate_mapnodepool(state));
  if (kl_unlikely(!newstate || klstate_checkframe(newstate, 1))) {
    klmm_free(klmm, co, sizeof (KlCoroutine));
    klmm_newlevel_abort(klmm);
    return NULL;
  }
  co->status = KLCO_NORMAL;
  co->state = newstate;
  co->kclo = kclo;
  klstack_pushobj(klstate_stack(newstate), co, KL_COROUTINE);
  klmm_newlevel_done(klmm, &klco_gcvfunc);
  return co;
}

KlException klco_start(KlCoroutine* co, KlState* caller, size_t narg, size_t nret) {
  kl_assert(klco_status(co) == KLCO_NORMAL, "should be normal coroutine");
  kl_assert(klstack_size(klstate_stack(caller)) >= narg, "not enough number of argument");
  KlState* costate = co->state;
  if (kl_unlikely(klstate_checkframe(costate, narg)))
    return klstate_throw(caller, KL_E_OOM, "out of memory when starting a coroutine");
  /* move arguments to execution environment of coroutine */
  KlValue* argbase = klstate_getval(caller, -narg);
  KlValue* costktop = klstate_stktop(costate);
  size_t count = narg;
  KlValue* argpos = argbase;
  while (count--)
    klvalue_setvalue(costktop++, argpos++);
  klstack_set_top(klstate_stack(costate), costktop);
  KlValue kclo;
  klvalue_setobj(&kclo, co->kclo, KL_KCLOSURE);
  klco_setstatus(co, KLCO_RUNNING); /* now we run this coroutine */
  KlException exception = klexec_call(costate, &kclo, narg, nret);
  if (kl_unlikely(exception)) {
    klco_setstatus(co, KLCO_DEAD);
    return exception;
  }
  /* the caller will never be called until the this function return, so the argbase should not change. */
  kl_assert(argbase == klstate_getval(caller, -narg), "stack reallocated!");
  if (klstate_isrunning(costate)) { /* yield ? */
    size_t nres = co->nyield;
    KlValue* firstres = klstate_getval(costate, -nres);
    size_t ncopy = nres > nret ? nret : nres;
    while (ncopy--)   /* copy results */
      klvalue_setvalue(argbase++, firstres++);
    klstack_set_top(klstate_stack(caller), argbase);
    if (nret > nres)  /* completing missing returned value */
      klstack_pushnil(klstate_stack(caller), nret - nres);
    klstack_move_top(klstate_stack(costate), -nres);
    klco_setstatus(co, KLCO_BLOCKED);
    return KL_E_NONE;
  } else {  /* else is dead */
    KlValue* firstres = klstate_getval(costate, -nret);
    size_t ncopy = nret;
    while (ncopy--)   /* copy results */
      klvalue_setvalue(argbase++, firstres++);
    klstack_set_top(klstate_stack(caller), argbase);
    klstack_move_top(klstate_stack(costate), -nret);
    klco_setstatus(co, KLCO_DEAD);
    return KL_E_NONE;
  }
}

KlException klco_resume(KlCoroutine* co, KlState* caller, size_t narg, size_t nret) {
  kl_assert(klco_status(co) == KLCO_BLOCKED, "should be blocked coroutine");
  kl_assert(klstack_size(klstate_stack(caller)) >= narg, "not enough number of argument");
  KlState* costate = co->state;
  /* framesize of co->state is guaranteed.
   * move arguments to execution environment of coroutine */
  kl_assert(klstack_residual(klstate_stack(costate)) >= co->nwanted, "not enough number of argument");
  KlValue* argbase = klstate_getval(caller, -narg);
  KlValue* costktop = klstate_stktop(costate);
  size_t nwanted = co->nwanted;
  size_t ncopy = narg < nwanted ? narg : nwanted;
  KlValue* argpos = argbase;
  while (ncopy--)
    klvalue_setvalue(costktop++, argpos++);
  klstack_set_top(klstate_stack(costate), costktop);
  if (narg < nwanted)
    klstack_pushnil(klstate_stack(costate), nwanted - narg);
  klco_setstatus(co, KLCO_RUNNING);
  KlException exception = klexec_execute(costate);
  if (kl_unlikely(exception)) {
    klco_setstatus(co, KLCO_DEAD);
    return exception;
  }
  /* the caller will never be called until the this function return, so the argbase should not change. */
  kl_assert(argbase == klstate_getval(caller, -narg), "stack reallocated!");
  if (klstate_isrunning(costate)) { /* yield ? */
    size_t nres = co->nyield;
    KlValue* firstres = klstate_getval(costate, -nres);
    size_t ncopy = nres > nret ? nret : nres;
    while (ncopy--)   /* copy results */
      klvalue_setvalue(argbase++, firstres++);
    klstack_set_top(klstate_stack(caller), argbase);
    if (nret > nres)  /* completing missing returned value */
      klstack_pushnil(klstate_stack(caller), nret - nres);
    klstack_move_top(klstate_stack(costate), -nres);
    klco_setstatus(co, KLCO_BLOCKED);
    return KL_E_NONE;
  } else {  /* else is dead */
    KlValue* firstres = klstate_getval(costate, -nret);
    size_t ncopy = nret;
    while (ncopy--)   /* copy results */
      klvalue_setvalue(argbase++, firstres++);
    klstack_set_top(klstate_stack(caller), argbase);
    klstack_move_top(klstate_stack(costate), -nret);
    klco_setstatus(co, KLCO_DEAD);
    return KL_E_NONE;
  }
}




static KlGCObject* klco_propagate(KlCoroutine* co, KlGCObject* gclist) {
  klmm_gcobj_mark_accessible(klmm_to_gcobj(co->state), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(co->kclo), gclist);
  return gclist;
}

static void klco_delete(KlCoroutine* co) {
  klmm_free(klmm_gcobj_getmm(klmm_to_gcobj(co)), co, sizeof (KlCoroutine));
}
