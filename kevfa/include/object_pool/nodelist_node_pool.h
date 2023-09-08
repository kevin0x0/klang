#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_NODELIST_NODE_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_NODELIST_NODE_POOL_H

#include "kevfa/include/list/node_list.h"

KevNodeListNode* kev_nodelist_node_pool_allocate(void);
void kev_nodelist_node_pool_deallocate(KevNodeListNode* intlist_node);
void kev_nodelist_node_pool_free(void);

#endif
