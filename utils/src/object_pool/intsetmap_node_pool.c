#include "utils/include/object_pool/intsetmap_node_pool.h"

#include <stdlib.h>

union KevIntSetMapNodePool {
  KevIntSetMapNode intsetmap_node;
  union KevIntSetMapNodePool* next;
};

static union KevIntSetMapNodePool* intsetmap_node_pool = NULL;

static inline KevIntSetMapNode* kev_intsetmap_node_pool_acquire(void);


static inline KevIntSetMapNode* kev_intsetmap_node_pool_acquire(void) {
  return (KevIntSetMapNode*)malloc(sizeof (union KevIntSetMapNodePool));
}

KevIntSetMapNode* kev_intsetmap_node_pool_allocate(void) {
  if (intsetmap_node_pool) {
    KevIntSetMapNode* retval = &intsetmap_node_pool->intsetmap_node;
    intsetmap_node_pool = intsetmap_node_pool->next;
    return retval;
  } else {
    return kev_intsetmap_node_pool_acquire();
  }
}

void kev_intsetmap_node_pool_deallocate(KevIntSetMapNode* intsetmap_node) {
  if (intsetmap_node) {
    union KevIntSetMapNodePool* freed_node = (union KevIntSetMapNodePool*)intsetmap_node;
    freed_node->next = intsetmap_node_pool;
    intsetmap_node_pool = freed_node;
  }
}

void kev_intsetmap_node_pool_free(void) {
  union KevIntSetMapNodePool* pool = intsetmap_node_pool;
  intsetmap_node_pool = NULL;
  while (pool) {
    union KevIntSetMapNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
