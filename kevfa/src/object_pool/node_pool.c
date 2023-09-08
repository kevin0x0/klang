#include "kevfa/include/object_pool/node_pool.h"

#include <stdlib.h>

union KevGraphNodePool {
  KevGraphNode graph_node;
  union KevGraphNodePool* next;
};

static union KevGraphNodePool* graph_node_pool = NULL;

static inline KevGraphNode* kev_graph_node_pool_acquire(void);


static inline KevGraphNode* kev_graph_node_pool_acquire(void) {
  return (KevGraphNode*)malloc(sizeof (union KevGraphNodePool));
}

KevGraphNode* kev_graph_node_pool_allocate(void) {
  if (graph_node_pool) {
    KevGraphNode* retval = &graph_node_pool->graph_node;
    graph_node_pool = graph_node_pool->next;
    return retval;
  } else {
    return kev_graph_node_pool_acquire();
  }
}

void kev_graph_node_pool_deallocate(KevGraphNode* graph_node) {
  if (graph_node) {
    union KevGraphNodePool* freed_node = (union KevGraphNodePool*)graph_node;
    freed_node->next = graph_node_pool;
    graph_node_pool = freed_node;
  }
}

void kev_graph_node_pool_free(void) {
  union KevGraphNodePool* pool = graph_node_pool;
  graph_node_pool = NULL;
  while (pool) {
    union KevGraphNodePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
