#ifndef KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEMSETTRANS_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJECT_POOL_ITEMSETTRANS_POOL_H

#include "kevlr/include/itemset_def.h"

KlrItemSetTransition* klr_itemsettrans_pool_allocate(void);
void klr_itemsettrans_pool_deallocate(KlrItemSetTransition* itemsetgoto);
void klr_itemsettrans_pool_free(void);

#endif
