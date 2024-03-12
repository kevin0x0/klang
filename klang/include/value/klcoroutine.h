#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCOROUTINE_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCOROUTINE_H

#include "klang/include/mm/klmm.h"
#include "klang/include/value/klclosure.h"
#include "klang/include/vm/klexception.h"

typedef struct tagKlState KlState;

typedef enum tagKlCoStatus {
  KLCO_RUNNING,
  KLCO_BLOCKED,
  KLCO_NORMAL,
  KLCO_DEAD,
} KlCoStatus;

typedef struct tagKlCoroutine {
  KlGCObject gcbase;
  KlState* state;       /* virtual machine */
  KlKClosure* kclo;     /* to be executed function */
  KlCoStatus costatus;  /* state of this coroutine */
} KlCoroutine;

KlCoroutine* klco_create(KlMM* klmm, KlKClosure* kclo, KlState* state);

static inline KlCoStatus klco_status(KlCoroutine* co);
KlException klco_call(KlCoroutine* co, KlState* caller, size_t narg, size_t nret);
KlException klco_resume(KlCoroutine* co, KlState* caller, size_t narg, size_t nret);

static inline KlCoStatus klco_status(KlCoroutine* co) {
  return co->costatus;
}


#endif
