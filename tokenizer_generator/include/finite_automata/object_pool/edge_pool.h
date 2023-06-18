#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_EDGE_POOL_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_EDGE_POOL_H

#include "tokenizer_generator/include/finite_automata/graph.h"


KevGraphEdge* kev_graph_edge_pool_acquire(void);
KevGraphEdge* kev_graph_edge_pool_allocate(void);
void kev_graph_edge_pool_deallocate(KevGraphEdge* graph_edge);
void kev_graph_edge_pool_free(void);


#endif
