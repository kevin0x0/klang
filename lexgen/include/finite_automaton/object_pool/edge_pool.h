#ifndef KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_EDGE_POOL_H
#define KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_EDGE_POOL_H

#include "lexgen/include/finite_automaton/graph.h"

KevGraphEdge* kev_graph_edge_pool_acquire(void);
KevGraphEdge* kev_graph_edge_pool_allocate(void);
void kev_graph_edge_pool_deallocate(KevGraphEdge* graph_edge);
void kev_graph_edge_pool_free(void);

#endif
