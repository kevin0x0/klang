#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_NODE_POOL_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_NODE_POOL_H

#include "tokenizer_generator/include/finite_automata/graph.h"


KevGraphNode* kev_graph_node_pool_acquire(void);
KevGraphNode* kev_graph_node_pool_allocate(void);
void kev_graph_node_pool_deallocate(KevGraphNode* node);
void kev_graph_node_pool_free(void);


#endif
