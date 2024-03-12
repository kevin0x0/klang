#include "klang/include/value/klcoroutine.h"
#include "klang/include/mm/klmm.h"
#include "klang/include/value/klstate.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klexception.h"
#include "klang/include/vm/klexec.h"

static KlGCObject* klco_propagate(KlCoroutine* co, KlGCObject* gclist);
static void klco_delete(KlCoroutine* co);

static KlGCVirtualFunc klco_gcvfunc = { .destructor = (KlGCDestructor)klco_delete, .propagate = (KlGCProp)klco_propagate };

KlCoroutine* klco_create(KlMM* klmm, KlKClosure* kclo, KlState* state) {
  KlCoroutine* co = (KlCoroutine*)klmm_alloc(klmm, sizeof (KlCoroutine));
  if (kl_unlikely(!co)) return NULL;
  klmm_newlevel(klmm, klmm_to_gcobj(co));
  KlState* newstate = klstate_create(klmm, klstate_global(state), klstate_common(state),
                                     klstate_strpool(state), klstate_mapnodepool(state));
  if (kl_unlikely(!newstate)) {
    klmm_free(klmm, co, sizeof (KlCoroutine));
    klmm_newlevel_abort(klmm);
    return NULL;
  }
  co->costatus = KLCO_NORMAL;
  co->state = newstate;
  co->kclo = kclo;
  klmm_newlevel_done(klmm, &klco_gcvfunc);
  return co;
}

KlException klco_call(KlCoroutine* co, KlState* caller, size_t narg, size_t nret) {
  kl_assert(klstack_size(klstate_stack(caller)) >= narg, "not enough number of argument");
  KlValue* argbase = klstate_getval(caller, -narg);
}

KlException klco_resume(KlCoroutine* co, KlState* caller, size_t narg, size_t nret) {
}




static KlGCObject* klco_propagate(KlCoroutine* co, KlGCObject* gclist) {
  klmm_gcobj_mark_accessible(klmm_to_gcobj(co->state), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(co->kclo), gclist);
  return gclist;
}

static void klco_delete(KlCoroutine* co) {
  klmm_free(klmm_gcobj_getmm(klmm_to_gcobj(co)), co, sizeof (KlCoroutine));
}
