#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCOROUTINE_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCOROUTINE_H

#include "klang/include/misc/klutils.h"
#include "klang/include/mm/klmm.h"
#include "klang/include/value/klclosure.h"
#include "klang/include/value/klstate.h"
#include "klang/include/vm/klexception.h"
#include <stddef.h>

typedef struct tagKlState KlState;

typedef enum tagKlCoStatus {
  KLCO_RUNNING,
  KLCO_BLOCKED,
  KLCO_NORMAL,
  KLCO_DEAD,
} KlCoStatus;

typedef struct tagKlCoroutine {
  KlGCObject gcbase;
  KlState* state;         /* virtual machine */
  KlKClosure* kclo;       /* to be executed function */
  KlCoStatus status;      /* state of this coroutine */
  size_t nyield;          /* number of yielded value */
  size_t nwanted;         /* number of wanted arguments when resuming */
  ptrdiff_t respos_save;  /* save the offset of to be returned values */
} KlCoroutine;

KlCoroutine* klco_create(KlMM* klmm, KlKClosure* kclo, KlState* state);

static inline KlCoStatus klco_status(KlCoroutine* co);
static inline KlState* klco_state(KlCoroutine* co);

KlException klco_start(KlCoroutine* co, KlState* caller, size_t narg, size_t nret);
KlException klco_resume(KlCoroutine* co, KlState* caller, size_t narg, size_t nret);

static inline KlException klco_call(KlCoroutine* co, KlState* caller, size_t narg, size_t nret);

static inline KlCoStatus klco_status(KlCoroutine* co) {
  return co->status;
}

static inline KlState* klco_state(KlCoroutine* co) {
  return co->state;
}

static inline KlException klco_call(KlCoroutine* co, KlState* caller, size_t narg, size_t nret) {
  if (kl_unlikely(klco_status(co) == KLCO_DEAD || klco_status(co) == KLCO_RUNNING))
    return klstate_throw(caller, KL_E_INVLD, "can not call a dead or running coroutine");
  if (kl_likely(klco_status(co) == KLCO_BLOCKED)) {
    return klco_resume(co, caller, narg, nret);
  } else {  /* else is KLCO_NORMAL */
    kl_assert(klco_status(co) == KLCO_NORMAL, "should be normal coroutine");
    return klco_start(co, caller, narg, nret);
  }
}


#endif
