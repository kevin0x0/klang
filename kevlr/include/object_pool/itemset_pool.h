#ifndef KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEMSET_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEMSET_POOL_H

#include "kevlr/include/itemset_def.h"

KevItemSet* kev_itemset_pool_acquire(void);
KevItemSet* kev_itemset_pool_allocate(void);
void kev_itemset_pool_deallocate(KevItemSet* itemset);
void kev_itemset_pool_free(void);

#endif
