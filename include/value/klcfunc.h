#ifndef _KLANG_INCLUDE_VALUE_KLCFUNC_H_
#define _KLANG_INCLUDE_VALUE_KLCFUNC_H_

#include "include/vm/klexception.h"

typedef struct tagKlState KlState;
typedef KlException KlCFunction(KlState* state);

#endif

