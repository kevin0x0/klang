#include "kevfa/include/graph.h"
#include "kevfa/include/object_pool/graph_pool.h"
#include "kevfa/include/object_pool/node_pool.h"
#include "kevfa/include/object_pool/edge_pool.h"
#include "utils/include/hashmap/address_map.h"

#include <stdlib.h>

static void kev_graph_edgelist_delete(KevGraphEdgeList* edges) {
  KevGraphEdgeList* curr_edge = edges;
  while (curr_edge) {
    KevGraphEdgeList* next_edge = curr_edge->next;
    kev_graph_edge_pool_deallocate(curr_edge);
    curr_edge = next_edge;
  }
}

bool kev_graph_init(KevGraph* graph, KevGraphNodeList* nodelist) {
  if (!graph) return false;

  graph->head = nodelist;
  if (nodelist == NULL) {
    graph->tail = NULL;
  } else {
    KevGraphNode* tail = nodelist;
    while (tail->next) tail = tail->next;
    graph->tail = tail;
  }
  return true;
}

void kev_graph_destroy(KevGraph* graph) {
  if (graph) {
    KevGraphNode* curr_node = graph->head;
    while (curr_node) {
      KevGraphNode* next_node = curr_node->next;
      kev_graphnode_delete(curr_node);
      curr_node = next_node;
    }
    graph->tail = NULL;
    graph->head = NULL;
  }
}

KevGraph* kev_graph_create(KevGraphNode* node) {
  KevGraph* graph = kev_graph_pool_allocate();
  if (kev_graph_init(graph, node))
    return graph;
  else
    return NULL;
}

bool kev_graph_create_move(KevGraph* src) {
  KevGraph* graph = kev_graph_pool_allocate();
  if (kev_graph_init_move(graph, src))
    return graph;
  else
    return NULL;
}

void kev_graph_delete(KevGraph* graph) {
  kev_graph_destroy(graph);
  kev_graph_pool_deallocate(graph);
}

bool kev_graph_merge(KevGraph* dest, KevGraph* src) {
  if (!dest || !src) return false;

  if (!dest->head) {
    dest->head = src->head;
    dest->tail = src->tail;
  } else if (src->head) {
    dest->tail->next = src->head;
    dest->tail = src->tail;
  }
  src->head = NULL;
  src->tail = NULL;
  return true;
}

KevGraphNode* kev_graphnode_create(KevGraphNodeId node_id) {
  KevGraphNode* node = kev_graph_node_pool_allocate();
  if (node) {
    node->id = node_id;
    node->edges = NULL;
    node->epsilons = NULL;
    node->next = NULL;
  }
  return node;
}

void kev_graphnode_delete(KevGraphNode* node) {
  if (!node) return;
  kev_graph_edgelist_delete(node->edges);
  kev_graph_edgelist_delete(node->epsilons);
  kev_graph_node_pool_deallocate(node);
}

bool kev_graphnode_connect(KevGraphNode* from, KevGraphNode* to, KevGraphEdgeAttr attr) {
  KevGraphEdgeList* new_edge = kev_graph_edge_pool_allocate();
  if (!new_edge) return false;
  new_edge->next = from->edges;
  new_edge->attr = attr;
  new_edge->node = to;
  from->edges = new_edge;
  return true;
}

bool kev_graphnode_connect_epsilon(KevGraphNode* from, KevGraphNode* to) {
  KevGraphEdgeList* new_edge = kev_graph_edge_pool_allocate();
  if (!new_edge) return false;
  new_edge->next = from->epsilons;
  new_edge->node = to;
  from->epsilons = new_edge;
  return true;
}

static inline void kev_nodelist_free(KevGraphNodeList* lst) {
  while (lst) {
    KevGraphNodeList* tmp = lst->next;
    kev_graph_node_pool_deallocate(lst);
    lst = tmp;
  }
}

bool kev_graph_node_list_copy(KevAddressMap* map, KevGraphNodeList** p_dest, KevGraphNodeList* src) {
  KevGraphNode lst_head;
  KevGraphNodeList* tail = &lst_head;
  while (src) {
    KevGraphNode* new_node = kev_graph_node_pool_allocate();
    if (!new_node || !kev_addressmap_insert(map, src, new_node)) {  /* handle error */
      kev_graph_node_pool_deallocate(new_node);
      tail->next = NULL;
      kev_nodelist_free(lst_head.next);
      return false;
    }

    new_node->id = src->id;
    new_node->epsilons = NULL;
    new_node->edges = NULL;
    tail->next = new_node;
    tail = new_node;
    src = src->next;
  }
  tail->next = NULL;
  *p_dest = lst_head.next;
  return true;
}

static bool kev_graph_edge_list_copy(KevAddressMap* map, KevGraphEdgeList** p_dest, KevGraphEdgeList* src) {
  KevGraphEdgeList lst_head;
  KevGraphEdgeList* tail = &lst_head;
  while (src) {
    KevGraphEdgeList* new_node = kev_graph_edge_pool_allocate();
    KevAddressMapNode* corresponding_node = kev_addressmap_search(map, src->node);
    if (!new_node || !corresponding_node) {   /* handle error */
      kev_graph_edge_pool_deallocate(new_node);
      tail->next = NULL;
      kev_graph_edgelist_delete(lst_head.next);
      return false;
    }

    new_node->attr = src->attr;
    new_node->node = (KevGraphNode*)(corresponding_node->value);
    tail->next = new_node;
    tail = new_node;
    src = src->next;
  }
  tail->next = NULL;
  *p_dest = lst_head.next;
  return true;
}

bool kev_graph_init_copy(KevGraph* graph, KevGraph* src) {
  /* TODO: optimize this function */
  if (!graph) return false;
  graph->head = NULL;
  graph->tail = NULL;
  if (!src) return false;
  if (!src->head) return true;

  KevAddressMap corresponding_node_map;
  KevGraphNode* head = NULL;
  if (!kev_addressmap_init(&corresponding_node_map, 32))
    return false;
  /* copy node list and record the mapping in 'map'
   * between nodes in 'src->head' and nodes in 'head' */
  if (!kev_graph_node_list_copy(&corresponding_node_map, &head, src->head)) {
    kev_addressmap_destroy(&corresponding_node_map);
    return false;
  }

  KevGraphNode* node = head;
  KevGraphNode* src_node = src->head;
  KevGraphNode* tail = NULL;
  while (node) {  /* copy edges for every node */
   if (!kev_graph_edge_list_copy(&corresponding_node_map, &node->edges, src_node->edges) ||
       !kev_graph_edge_list_copy(&corresponding_node_map, &node->epsilons, src_node->epsilons)) {
     /* handle error */
     kev_addressmap_destroy(&corresponding_node_map);
     KevGraphNode* free_node = head;
     while (free_node) {
       KevGraphNode* tmp = free_node->next;
       kev_graphnode_delete(free_node);
       free_node = tmp;
     }
     return false;
   }
   tail = node;
   node = node->next;
   src_node = src_node->next;
  }

  kev_addressmap_destroy(&corresponding_node_map);
  graph->head = head;
  graph->tail = tail;
  return true;
}
