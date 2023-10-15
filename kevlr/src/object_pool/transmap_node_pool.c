#include "kevlr/include/object_pool/transmap_node_pool.h"

#include <stdlib.h>

union KlrTranMapNodePool {
  KlrTransMapNode transmap_node;
  union KlrTranMapNodePool* next;
};

static union KlrTranMapNodePool* transmap_node_pool = NULL;

static inline KlrTransMapNode* klr_transmap_node_pool_acquire(void);


static inline KlrTransMapNode* klr_transmap_node_pool_acquire(void) {
  return (KlrTransMapNode*)malloc(sizeof (union KlrTranMapNodePool));
}

KlrTransMapNode* klr_transmap_node_pool_allocate(void) {
  if (transmap_node_pool) {
    KlrTransMapNode* retval = &transmap_node_pool->transmap_node;
    transmap_node_pool = transmap_node_pool->next;
    return retval;
  } else {
    return klr_transmap_node_pool_acquire();
  }
}

void klr_transmap_node_pool_deallocate(KlrTransMapNode* transmap_node) {
  if (transmap_node) {
    union KlrTranMapNodePool* freed_node = (union KlrTranMapNodePool*)transmap_node;
    freed_node->next = transmap_node_pool;
    transmap_node_pool = freed_node;
  }
}

void klr_transmap_node_pool_free(void) {
  union KlrTranMapNodePool* pool = transmap_node_pool;
  transmap_node_pool = NULL;
  while (pool) {
    union KlrTranMapNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
