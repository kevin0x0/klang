#ifndef KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_ITEMSETGOTO_POOL_H
#define KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_ITEMSETGOTO_POOL_H

#include "pargen/include/lr/itemset_def.h"

KevItemSetGoto* kev_itemsetgoto_pool_acquire(void);
KevItemSetGoto* kev_itemsetgoto_pool_allocate(void);
void kev_itemsetgoto_pool_deallocate(KevItemSetGoto* itemsetgoto);
void kev_itemsetgoto_pool_free(void);

#endif
