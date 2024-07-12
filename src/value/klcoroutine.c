#include "include/value/klcoroutine.h"
#include "include/value/klclosure.h"
#include "include/value/klstate.h"
#include "include/mm/klmm.h"
#include <stddef.h>




void klco_init(KlCoroutine* co, KlKClosure* kclo) {
  co->kclo = kclo;
  co->status = KLCO_NORMAL;
  co->allow_yield = kclo != NULL;
}

KlState* klco_create(KlState* state, KlKClosure* kclo) {
  KlMM* klmm = klstate_getmm(state);
  KlState* costate = klstate_create(klmm, klstate_global(state), klstate_common(state), klstate_strpool(state), kclo);
  if (kl_unlikely(!costate)) return NULL;
  return costate;
}

