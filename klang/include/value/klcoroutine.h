#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCOROUTINE_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCOROUTINE_H

#include "klang/include/misc/klutils.h"
#include "klang/include/mm/klmm.h"
#include "klang/include/value/klclosure.h"
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
  KlKClosure* kclo;       /* to be executed function */
  KlCoStatus status;      /* state of this coroutine */
  size_t nyield;          /* number of yielded value */
  size_t nwanted;         /* number of wanted arguments when resuming */
  ptrdiff_t respos_save;  /* save the offset of to be returned values */
} KlCoroutine;

void klco_init(KlCoroutine* co, KlKClosure* kclo);
KlState* klco_create(KlState* state, KlKClosure* kclo);
static inline KlGCObject* klco_propagate(KlCoroutine* co, KlGCObject* gclist);

static inline KlCoStatus klco_status(KlCoroutine* co);
static inline void klco_setstatus(KlCoroutine* co, KlCoStatus status);
static inline bool klco_valid(KlCoroutine* co);
static inline void klco_yield(KlCoroutine* co, size_t nyield, size_t nwanted);

KlException klco_start(KlState* co, KlState* caller, size_t narg, size_t nret);
KlException klco_resume(KlState* co, KlState* caller, size_t narg, size_t nret);

KlException klco_call(KlState* co, KlState* caller, size_t narg, size_t nret);

static inline KlCoStatus klco_status(KlCoroutine* co) {
  return co->status;
}

static inline void klco_setstatus(KlCoroutine* co, KlCoStatus status) {
  co->status = status;
}

static inline bool klco_valid(KlCoroutine* co) {
  return co->kclo != NULL;
}

static inline void klco_yield(KlCoroutine* co, size_t nyield, size_t nwanted) {
  kl_assert(klco_status(co) == KLCO_RUNNING, "can only yield running coroutine");
  co->nyield = nyield;
  co->nwanted = nwanted;
  klco_setstatus(co, KLCO_BLOCKED);
}

static KlGCObject* klco_propagate(KlCoroutine* co, KlGCObject* gclist) {
  if (co->kclo)
    klmm_gcobj_mark_accessible(klmm_to_gcobj(co->kclo), gclist);
  return gclist;
}

#endif
