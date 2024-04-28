#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCFUNC_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCFUNC_H

#include "include/vm/klexception.h"

typedef struct tagKlState KlState;
typedef KlException KlCFunction(KlState* state);

#endif

