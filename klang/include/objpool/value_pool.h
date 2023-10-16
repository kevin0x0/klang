#ifndef KEVCC_KLANG_INCLUDE_OBJPOOL_VALUE_POOL_H
#define KEVCC_KLANG_INCLUDE_OBJPOOL_VALUE_POOL_H

#include "klang/include/value/value.h"

KlValue* klvalue_pool_allocate(void);
void klvalue_pool_deallocate(KlValue* value);
void klvalue_pool_free(void);

#endif
