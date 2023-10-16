#ifndef KEVCC_KEVLR_INCLUDE_OBJPOOL_ITEM_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJPOOL_ITEM_POOL_H

#include "kevlr/include/itemset_def.h"

KlrItem* klr_item_pool_allocate(void);
void klr_item_pool_deallocate(KlrItem* item);
void klr_item_pool_free(void);

#endif
