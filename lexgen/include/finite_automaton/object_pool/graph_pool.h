#ifndef KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_GRAPH_POOL_H
#define KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_GRAPH_POOL_H

#include "lexgen/include/finite_automaton/graph.h"


KevGraph* kev_graph_pool_acquire(void);
KevGraph* kev_graph_pool_allocate(void);
void kev_graph_pool_deallocate(KevGraph* graph);
void kev_graph_pool_free(void);


#endif
