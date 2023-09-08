#include "kevfa/include/object_pool/intlistmap_node_pool.h"

#include <stdlib.h>

union KevIntListMapNodePool {
  KevIntListMapNode intlistmap_node;
  union KevIntListMapNodePool* next;
};

static union KevIntListMapNodePool* intlistmap_node_pool = NULL;

static inline KevIntListMapNode* kev_intlistmap_node_pool_acquire(void);


static inline KevIntListMapNode* kev_intlistmap_node_pool_acquire(void) {
  return (KevIntListMapNode*)malloc(sizeof (union KevIntListMapNodePool));
}

KevIntListMapNode* kev_intlistmap_node_pool_allocate(void) {
  if (intlistmap_node_pool) {
    KevIntListMapNode* retval = &intlistmap_node_pool->intlistmap_node;
    intlistmap_node_pool = intlistmap_node_pool->next;
    return retval;
  } else {
    return kev_intlistmap_node_pool_acquire();
  }
}

void kev_intlistmap_node_pool_deallocate(KevIntListMapNode* intlistmap_node) {
  if (intlistmap_node) {
    union KevIntListMapNodePool* freed_node = (union KevIntListMapNodePool*)intlistmap_node;
    freed_node->next = intlistmap_node_pool;
    intlistmap_node_pool = freed_node;
  }
}

void kev_intlistmap_node_pool_free(void) {
  union KevIntListMapNodePool* pool = intlistmap_node_pool;
  intlistmap_node_pool = NULL;
  while (pool) {
    union KevIntListMapNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
