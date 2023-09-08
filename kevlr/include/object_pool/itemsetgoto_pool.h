#ifndef KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEMSETGOTO_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEMSETGOTO_POOL_H

#include "kevlr/include/itemset_def.h"

KevItemSetGoto* kev_itemsetgoto_pool_allocate(void);
void kev_itemsetgoto_pool_deallocate(KevItemSetGoto* itemsetgoto);
void kev_itemsetgoto_pool_free(void);

#endif
