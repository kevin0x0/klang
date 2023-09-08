#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_GRAPH_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_GRAPH_POOL_H

#include "kevfa/include/graph.h"

KevGraph* kev_graph_pool_allocate(void);
void kev_graph_pool_deallocate(KevGraph* graph);
void kev_graph_pool_free(void);

#endif
