#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_GRAPH_POOL_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_GRAPH_POOL_H

#include "tokenizer_generator/include/finite_automata/graph.h"


KevGraph* kev_graph_pool_acquire(void);
KevGraph* kev_graph_pool_allocate(void);
void kev_graph_pool_deallocate(KevGraph* graph);
void kev_graph_pool_free(void);


#endif
