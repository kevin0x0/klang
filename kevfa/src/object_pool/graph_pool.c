#include "kevfa/include/object_pool/graph_pool.h"

#include <stdlib.h>

union KevGraphPool {
  KevGraph graph;
  union KevGraphPool* next;
};

static union KevGraphPool* graph_pool = NULL;

static inline KevGraph* kev_graph_pool_acquire(void);


static inline KevGraph* kev_graph_pool_acquire(void) {
  return (KevGraph*)malloc(sizeof (union KevGraphPool));
}

KevGraph* kev_graph_pool_allocate(void) {
  if (graph_pool) {
    KevGraph* retval = &graph_pool->graph;
    graph_pool = graph_pool->next;
    return retval;
  } else {
    return kev_graph_pool_acquire();
  }
}

void kev_graph_pool_deallocate(KevGraph* graph) {
  if (graph) {
    union KevGraphPool* freed_node = (union KevGraphPool*)graph;
    freed_node->next = graph_pool;
    graph_pool = freed_node;
  }
}

void kev_graph_pool_free(void) {
  union KevGraphPool* pool = graph_pool;
  graph_pool = NULL;
  while (pool) {
    union KevGraphPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
