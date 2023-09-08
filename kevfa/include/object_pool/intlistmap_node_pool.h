#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_INTLISTMAP_NODE_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_INTLISTMAP_NODE_POOL_H

#include "kevfa/include/hashmap/intlist_map.h"

KevIntListMapNode* kev_intlistmap_node_pool_allocate(void);
void kev_intlistmap_node_pool_deallocate(KevIntListMapNode* intlistmap_node);
void kev_intlistmap_node_pool_free(void);

#endif
