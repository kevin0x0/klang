#ifndef KEVCC_KEVLR_INCLUDE_OBJECT_POOL_GOTOMAP_NODE_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJECT_POOL_GOTOMAP_NODE_POOL_H

#include "kevlr/include/hashmap/goto_map.h"

KevGotoMapNode* kev_gotomap_node_pool_acquire(void);
KevGotoMapNode* kev_gotomap_node_pool_allocate(void);
void kev_gotomap_node_pool_deallocate(KevGotoMapNode* gotomap_node);
void kev_gotomap_node_pool_free(void);

#endif
