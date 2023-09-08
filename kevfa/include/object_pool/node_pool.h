#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_NODE_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_NODE_POOL_H

#include "kevfa/include/graph.h"

KevGraphNode* kev_graph_node_pool_allocate(void);
void kev_graph_node_pool_deallocate(KevGraphNode* node);
void kev_graph_node_pool_free(void);

#endif
