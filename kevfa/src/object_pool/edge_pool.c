#include "kevfa/include/object_pool/edge_pool.h"

#include <stdlib.h>

union KevGraphEdgePool {
  KevGraphEdge graph_edge;
  union KevGraphEdgePool* next;
};

static union KevGraphEdgePool* graph_edge_pool = NULL;

static inline KevGraphEdge* kev_graph_edge_pool_acquire(void);


static inline KevGraphEdge* kev_graph_edge_pool_acquire(void) {
  return (KevGraphEdge*)malloc(sizeof (union KevGraphEdgePool));
}

KevGraphEdge* kev_graph_edge_pool_allocate(void) {
  if (graph_edge_pool) {
    KevGraphEdge* retval = &graph_edge_pool->graph_edge;
    graph_edge_pool = graph_edge_pool->next;
    return retval;
  } else {
    return kev_graph_edge_pool_acquire();
  }
}

void kev_graph_edge_pool_deallocate(KevGraphEdge* graph_edge) {
  if (graph_edge) {
    union KevGraphEdgePool* freed_node = (union KevGraphEdgePool*)graph_edge;
    freed_node->next = graph_edge_pool;
    graph_edge_pool = freed_node;
  }
}

void kev_graph_edge_pool_free(void) {
  union KevGraphEdgePool* pool = graph_edge_pool;
  graph_edge_pool = NULL;
  while (pool) {
    union KevGraphEdgePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
