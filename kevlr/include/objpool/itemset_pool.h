#ifndef KEVCC_KEVLR_INCLUDE_OBJPOOL_ITEMSET_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJPOOL_ITEMSET_POOL_H

#include "kevlr/include/itemset_def.h"

KlrItemSet* klr_itemset_pool_allocate(void);
void klr_itemset_pool_deallocate(KlrItemSet* itemset);
void klr_itemset_pool_free(void);

#endif
