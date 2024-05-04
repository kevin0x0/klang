#include "include/value/klcoroutine.h"
#include "include/value/klclosure.h"
#include "include/value/klstate.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include "include/vm/klexec.h"
#include "include/vm/klstack.h"
#include "include/mm/klmm.h"
#include <stddef.h>




void klco_init(KlCoroutine* co, KlKClosure* kclo) {
  co->kclo = kclo;
  co->status = KLCO_NORMAL;
  co->allow_yield = kclo != NULL;
}

KlState* klco_create(KlState* state, KlKClosure* kclo) {
  KlMM* klmm = klstate_getmm(state);
  KlState* costate = klstate_create(klmm, klstate_global(state), klstate_common(state), klstate_strpool(state), klstate_mapnodepool(state), kclo);
  if (kl_unlikely(!costate)) return NULL;
  return costate;
}

KlException klco_start(KlState* costate, KlState* caller, size_t narg, size_t nret) {
  kl_assert(klco_status(&costate->coinfo) == KLCO_NORMAL, "should be normal coroutine");
  kl_assert(narg != KLINST_VARRES && klstack_size(klstate_stack(caller)) >= narg, "not enough number of argument");
  kl_assert(costate->coinfo.kclo != NULL, "no executable");

  if (kl_unlikely(klstate_checkframe(costate, narg)))
    return klstate_throw(caller, KL_E_OOM, "out of memory when starting a coroutine");

  /* move arguments to execution environment of coroutine */
  KlValue* argbase = klstate_getval(caller, -narg);
  KlValue* costktop = klstate_stktop(costate);
  costate->coinfo.respos_save = klexec_savestack(costate, costktop);
  size_t count = narg;
  KlValue* argpos = argbase;
  while (count--)
    klvalue_setvalue(costktop++, argpos++);
  klstack_set_top(klstate_stack(costate), costktop);

  KlValue kclo;
  klvalue_setobj(&kclo, costate->coinfo.kclo, KL_KCLOSURE);
  klco_setstatus(&costate->coinfo, KLCO_RUNNING); /* now we run this coroutine */
  if (setjmp(costate->coinfo.yieldpos) == KLCOJMP_NORMAL) {
    KlException exception = klexec_call(costate, &kclo, narg, KLINST_VARRES, argbase);
    if (kl_unlikely(exception)) {
      klco_setstatus(&costate->coinfo, KLCO_DEAD);
      return exception;
    }
    KlValue* firstres = klexec_restorestack(costate, costate->coinfo.respos_save);
    size_t nres = (size_t)(klstate_stktop(costate) - firstres);
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

static KlException klco_resume_execute(KlState* costate) {
  KlException exception;
  while (klstate_isrunning(costate)) {
    KlCallInfo* ci = klstate_currci(costate);
    if (ci->status & KLSTATE_CI_STATUS_KCLO) {
      exception = klexec_execute(costate);
      if (kl_unlikely(exception)) return exception;
    } else if (ci->status & KLSTATE_CI_STATUS_CFUN) {
      if (kl_unlikely(exception = ci->callable.cfunc(costate)))
        return exception;
      klexec_pop_callinfo(costate);
    } else if (ci->status & KLSTATE_CI_STATUS_CCLO) {
      if (kl_unlikely(exception = klcast(KlCClosure*, ci->callable.clo)->cfunc(costate)))
        return exception;
      klexec_pop_callinfo(costate);
    }
  }
  return KL_E_NONE;
}

KlException klco_resume(KlState* costate, KlState* caller, size_t narg, size_t nret) {
  kl_assert(klco_status(&costate->coinfo) == KLCO_BLOCKED, "should be blocked coroutine");
  kl_assert(klstack_size(klstate_stack(caller)) >= narg, "not enough number of argument");
  kl_assert(costate->coinfo.kclo != NULL, "no executable");

  if (kl_unlikely(klstate_checkframe(costate, narg)))
    return klstate_throw(caller, KL_E_OOM, "out of memory when resuming a coroutine");

  /* move arguments to execution environment of coroutine */
  kl_assert(klstack_residual(klstate_stack(costate)) >= costate->coinfo.nwanted, "not enough number of argument");
  KlValue* argbase = klstate_getval(caller, -narg);
  KlValue* costktop = klstate_stktop(costate);
  kl_assert(KLINST_VARRES == 255 && narg < KLINST_VARRES, "");
  size_t nwanted = costate->coinfo.nwanted;
  size_t ncopy = narg < nwanted ? narg : nwanted;
  KlValue* argpos = argbase;
  while (ncopy--)
    klvalue_setvalue(costktop++, argpos++);
  if (nwanted == KLINST_VARRES)
    nwanted = narg;
  if (narg < nwanted)
    klexec_setnils(costktop, nwanted - narg);
  klstack_set_top(klstate_stack(costate), costktop + nwanted - narg);
  klstack_set_top(klstate_stack(caller), argbase);

  klco_setstatus(&costate->coinfo, KLCO_RUNNING);
  if (setjmp(costate->coinfo.yieldpos) == KLCOJMP_NORMAL) {
    KlException exception = klco_resume_execute(costate);
    if (kl_unlikely(exception)) {
      klco_setstatus(&costate->coinfo, KLCO_DEAD);
      return exception;
    }
    KlValue* firstres = klexec_restorestack(costate, costate->coinfo.respos_save);
    size_t nres = (size_t)(klstate_stktop(costate) - firstres);
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

KlException klco_call(KlState* costate, KlState* caller, size_t narg, size_t nret) {
  if (kl_unlikely(costate->coinfo.kclo == NULL))
    return klstate_throw(caller, KL_E_INVLD, "invalid coroutine");
  if (kl_unlikely(klco_status(&costate->coinfo) == KLCO_DEAD || klco_status(&costate->coinfo) == KLCO_RUNNING))
    return klstate_throw(caller, KL_E_INVLD, "can not call a dead or running coroutine");
  KlException exception;
  if (kl_likely(klco_status(&costate->coinfo) == KLCO_BLOCKED)) {
    exception = klco_resume(costate, caller, narg, nret);
  } else {  /* else is KLCO_NORMAL */
    kl_assert(klco_status(&costate->coinfo) == KLCO_NORMAL, "should be normal coroutine");
    exception = klco_start(costate, caller, narg, nret);
  }
  if (kl_unlikely(exception))
    return klstate_throw_link(caller, costate);
  return KL_E_NONE;
}
