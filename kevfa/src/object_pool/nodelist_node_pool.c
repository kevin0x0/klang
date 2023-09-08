#include "kevfa/include/object_pool/nodelist_node_pool.h"

#include <stdlib.h>

union KevNodeListNodePool {
  KevNodeListNode nodelist_node;
  union KevNodeListNodePool* next;
};

static union KevNodeListNodePool* nodelist_node_pool = NULL;

static inline KevNodeListNode* kev_nodelist_node_pool_acquire(void);


static inline KevNodeListNode* kev_nodelist_node_pool_acquire(void) {
  return (KevNodeListNode*)malloc(sizeof (union KevNodeListNodePool));
}

KevNodeListNode* kev_nodelist_node_pool_allocate(void) {
  if (nodelist_node_pool) {
    KevNodeListNode* retval = &nodelist_node_pool->nodelist_node;
    nodelist_node_pool = nodelist_node_pool->next;
    return retval;
  } else {
    return kev_nodelist_node_pool_acquire();
  }
}

void kev_nodelist_node_pool_deallocate(KevNodeListNode* nodelist_node) {
  if (nodelist_node) {
    union KevNodeListNodePool* freed_node = (union KevNodeListNodePool*)nodelist_node;
    freed_node->next = nodelist_node_pool;
    nodelist_node_pool = freed_node;
  }
}

void kev_nodelist_node_pool_free(void) {
  union KevNodeListNodePool* pool = nodelist_node_pool;
  nodelist_node_pool = NULL;
  while (pool) {
    union KevNodeListNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
