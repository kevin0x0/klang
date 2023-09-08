#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_SET_CROSS_LIST_NODE_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_SET_CROSS_LIST_NODE_POOL_H

#include "kevfa/include/list/set_cross_list.h"

KevSetCrossListNode* kev_set_cross_list_node_pool_allocate(void);
void kev_set_cross_list_node_pool_deallocate(KevSetCrossListNode* node);
void kev_set_cross_list_node_pool_free(void);

#endif
