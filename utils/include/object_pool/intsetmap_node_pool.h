#ifndef KEVCC_UTILS_INCLUDE_OBJECT_POOL_INTSETMAP_NODE_POOL_H
#define KEVCC_UTILS_INCLUDE_OBJECT_POOL_INTSETMAP_NODE_POOL_H

#include "utils/include/hashmap/intset_map.h"

KevIntSetMapNode* kev_intsetmap_node_pool_allocate(void);
void kev_intsetmap_node_pool_deallocate(KevIntSetMapNode* intsetmap_node);
void kev_intsetmap_node_pool_free(void);

#endif
