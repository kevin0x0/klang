#ifndef KEVCC_KLANG_INCLUDE_KS_STATE_H
#define KEVCC_KLANG_INCLUDE_KS_STATE_H

#include "klang/include/klmm.h"
typedef struct tagKlState {
  KlMM* mm;
  KlArray* stack;
} KlState;

#endif
