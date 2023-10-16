#include "klang/include/objpool/klmapnode_pool.h"

#include <stdlib.h>

union KlMapNodePool {
  KlMapNode mapnode;
  union KlMapNodePool* next;
};

static union KlMapNodePool* klmapnode_pool = NULL;

static inline KlMapNode* klmapnode_pool_acquire(void);


static inline KlMapNode* klmapnode_pool_acquire(void) {
  return (KlMapNode*)malloc(sizeof (union KlMapNodePool));
}

KlMapNode* klmapnode_pool_allocate(void) {
  if (klmapnode_pool) {
    KlMapNode* retval = &klmapnode_pool->mapnode;
    klmapnode_pool = klmapnode_pool->next;
    return retval;
  } else {
    return klmapnode_pool_acquire();
  }
}

void klmapnode_pool_deallocate(KlMapNode* node) {
  if (node) {
    union KlMapNodePool* freed_node = (union KlMapNodePool*)node;
    freed_node->next = klmapnode_pool;
    klmapnode_pool = freed_node;
  }
}

void klmapnode_pool_free(void) {
  union KlMapNodePool* pool = klmapnode_pool;
  klmapnode_pool = NULL;
  while (pool) {
    union KlMapNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
