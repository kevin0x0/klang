#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCFUNC_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCFUNC_H

#include "klang/include/vm/klexception.h"
#include <stdint.h>

typedef struct tagKlState KlState;
typedef KlException KlCFunction(KlState* state);

#endif

