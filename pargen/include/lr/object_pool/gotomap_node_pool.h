#ifndef KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_GOTOMAP_NODE_POOL_H
#define KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_GOTOMAP_NODE_POOL_H

#include "pargen/include/lr/hashmap/gotomap.h"

KevGotoMapNode* kev_gotomap_node_pool_acquire(void);
KevGotoMapNode* kev_gotomap_node_pool_allocate(void);
void kev_gotomap_node_pool_deallocate(KevGotoMapNode* gotomap_node);
void kev_gotomap_node_pool_free(void);

#endif
