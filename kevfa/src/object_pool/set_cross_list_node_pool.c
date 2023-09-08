#include "kevfa/include/object_pool/set_cross_list_node_pool.h"

#include <stdlib.h>

union KevSetCrossListNodePool {
  KevSetCrossListNode set_cross_list_node;
  union KevSetCrossListNodePool* next;
};

static union KevSetCrossListNodePool* set_cross_list_node_pool = NULL;

static inline KevSetCrossListNode* kev_set_cross_list_node_pool_acquire(void);

static inline KevSetCrossListNode* kev_set_cross_list_node_pool_acquire(void) {
  return (KevSetCrossListNode*)malloc(sizeof (union KevSetCrossListNodePool));
}

KevSetCrossListNode* kev_set_cross_list_node_pool_allocate(void) {
  if (set_cross_list_node_pool) {
    KevSetCrossListNode* retval = &set_cross_list_node_pool->set_cross_list_node;
    set_cross_list_node_pool = set_cross_list_node_pool->next;
    return retval;
  } else {
    return kev_set_cross_list_node_pool_acquire();
  }
}

void kev_set_cross_list_node_pool_deallocate(KevSetCrossListNode* set_cross_list_node) {
  if (set_cross_list_node) {
    union KevSetCrossListNodePool* freed_node = (union KevSetCrossListNodePool*)set_cross_list_node;
    freed_node->next = set_cross_list_node_pool;
    set_cross_list_node_pool = freed_node;
  }
}

void kev_set_cross_list_node_pool_free(void) {
  union KevSetCrossListNodePool* pool = set_cross_list_node_pool;
  set_cross_list_node_pool = NULL;
  while (pool) {
    union KevSetCrossListNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
