#ifndef KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_NODELIST_NODE_POOL_H
#define KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_OBJECT_POOL_NODELIST_NODE_POOL_H

#include "lexgen/include/finite_automaton/list/node_list.h"


KevNodeListNode* kev_nodelist_node_pool_acquire(void);
KevNodeListNode* kev_nodelist_node_pool_allocate(void);
void kev_nodelist_node_pool_deallocate(KevNodeListNode* intlist_node);
void kev_nodelist_node_pool_free(void);


#endif
