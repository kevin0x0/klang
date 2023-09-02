#ifndef KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_ITEM_POOL_H
#define KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_ITEM_POOL_H

#include "pargen/include/lr/item_def.h"

KevItem* kev_item_pool_acquire(void);
KevItem* kev_item_pool_allocate(void);
void kev_item_pool_deallocate(KevItem* item);
void kev_item_pool_free(void);

#endif
