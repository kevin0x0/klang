#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_EDGE_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_EDGE_POOL_H

#include "kevfa/include/graph.h"

KevGraphEdge* kev_graph_edge_pool_allocate(void);
void kev_graph_edge_pool_deallocate(KevGraphEdge* graph_edge);
void kev_graph_edge_pool_free(void);

#endif
