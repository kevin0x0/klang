#ifndef KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEM_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEM_POOL_H

#include "kevlr/include/itemset_def.h"

KevItem* kev_item_pool_acquire(void);
KevItem* kev_item_pool_allocate(void);
void kev_item_pool_deallocate(KevItem* item);
void kev_item_pool_free(void);

#endif
