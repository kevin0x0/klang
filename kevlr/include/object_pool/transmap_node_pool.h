#ifndef KEVCC_KEVLR_INCLUDE_OBJECT_POOL_TRANSMAP_NODE_POOL_H
#define KEVCC_KEVLR_INCLUDE_OBJECT_POOL_TRANSMAP_NODE_POOL_H

#include "kevlr/include/hashmap/trans_map.h"

KlrTransMapNode* klr_transmap_node_pool_allocate(void);
void klr_transmap_node_pool_deallocate(KlrTransMapNode* gotomap_node);
void klr_transmap_node_pool_free(void);

#endif
