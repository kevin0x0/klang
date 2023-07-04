#ifndef KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_NODE_POOL_H
#define KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_NODE_POOL_H

#include "lexgen/include/finite_automaton/graph.h"


KevGraphNode* kev_graph_node_pool_acquire(void);
KevGraphNode* kev_graph_node_pool_allocate(void);
void kev_graph_node_pool_deallocate(KevGraphNode* node);
void kev_graph_node_pool_free(void);


#endif