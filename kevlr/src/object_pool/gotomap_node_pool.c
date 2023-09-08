#include "kevlr/include/object_pool/gotomap_node_pool.h"

#include <stdlib.h>

union KevGotoMapNodePool {
  KevGotoMapNode gotomap_node;
  union KevGotoMapNodePool* next;
};

static union KevGotoMapNodePool* gotomap_node_pool = NULL;

static inline KevGotoMapNode* kev_gotomap_node_pool_acquire(void);


static inline KevGotoMapNode* kev_gotomap_node_pool_acquire(void) {
  return (KevGotoMapNode*)malloc(sizeof (union KevGotoMapNodePool));
}

KevGotoMapNode* kev_gotomap_node_pool_allocate(void) {
  if (gotomap_node_pool) {
    KevGotoMapNode* retval = &gotomap_node_pool->gotomap_node;
    gotomap_node_pool = gotomap_node_pool->next;
    return retval;
  } else {
    return kev_gotomap_node_pool_acquire();
  }
}

void kev_gotomap_node_pool_deallocate(KevGotoMapNode* gotomap_node) {
  if (gotomap_node) {
    union KevGotoMapNodePool* freed_node = (union KevGotoMapNodePool*)gotomap_node;
    freed_node->next = gotomap_node_pool;
    gotomap_node_pool = freed_node;
  }
}

void kev_gotomap_node_pool_free(void) {
  union KevGotoMapNodePool* pool = gotomap_node_pool;
  gotomap_node_pool = NULL;
  while (pool) {
    union KevGotoMapNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
