#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_NFA_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_NFA_POOL_H

#include "kevfa/include/finite_automaton.h"

KevFA* kev_fa_pool_allocate(void);
void kev_fa_pool_deallocate(KevFA* fa);
void kev_fa_pool_free(void);

#endif
