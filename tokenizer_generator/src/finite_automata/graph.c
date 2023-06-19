#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/finite_automata/object_pool/graph_pool.h"
#include "tokenizer_generator/include/finite_automata/object_pool/node_pool.h"
#include "tokenizer_generator/include/finite_automata/object_pool/edge_pool.h"
#include "tokenizer_generator/include/finite_automata/hashmap/address_map.h"

#include <stdlib.h>


static void kev_graph_edgelist_delete(KevGraphEdgeList* edges) {
  KevGraphEdgeList* curr_edge = edges;
  while (curr_edge) {
    KevGraphEdgeList* next_edge = curr_edge->next;
    kev_graph_edge_pool_deallocate(curr_edge);
    curr_edge = next_edge;
  }
}

inline bool kev_graph_init(KevGraph* graph, KevGraphNodeList* nodelist) {
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

void kev_graph_delete(KevGraph* graph) {
  kev_graph_destroy(graph);
  kev_graph_pool_deallocate(graph);
}

bool kev_graph_merge(KevGraph* to, KevGraph* from) {
  if (!to || !from) return false;

  if (!to->head) {
    to->head = from->head;
    to->tail = from->tail;
  } else if (from->head) {
    to->tail->next = from->head;
    to->tail = from->tail;
  }
  from->head = NULL;
  from->tail = NULL;
  return true;
}

inline KevGraphNode* kev_graphnode_create(KevGraphNodeAttr attr) {
  KevGraphNode* node = kev_graph_node_pool_allocate();
  if (node) {
    node->attr = attr;
    node->edges = NULL;
    node->next = NULL;
  }
  return node;
}

inline void kev_graphnode_delete(KevGraphNode* node) {
  if (!node) return;
  kev_graph_edgelist_delete(node->edges);
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

inline static void kev_nodelist_free(KevGraphNodeList* lst) {
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
    if (!new_node || !kev_address_map_insert(map, src, new_node)) {
      kev_graph_node_pool_deallocate(new_node);
      tail->next = NULL;
      kev_nodelist_free(lst_head.next);
      return false;
    }

    new_node->attr = src->attr;
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
    KevAddressMapNode* corresponding_node = kev_address_map_search(map, src->node);
    if (!new_node || !corresponding_node) {
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
  if (!graph) return false;
  if (!src) {
    graph->head = NULL;
    graph->tail = NULL;
    return false;
  }

  graph->head = NULL;
  graph->tail = NULL;
  if (!src->head) return true;

  KevAddressMap map;
  if (!kev_address_map_init(&map, 32))
    return false;
  
  KevGraphNode* head = NULL;
  if (!kev_graph_node_list_copy(&map, &head, src->head)) {
    kev_address_map_destroy(&map);
    return false;
  }

  KevGraphNode* node = head;
  KevGraphNode* src_node = src->head;
  KevGraphNode* tail = NULL;
  while (node) {
   if (!kev_graph_edge_list_copy(&map, &node->edges, src_node->edges)) {
     kev_address_map_destroy(&map);
     KevGraphNode* free_node = head;
     while (free_node != node) {
       KevGraphNode* tmp = free_node->next;
       kev_graphnode_delete(free_node);
       free_node = tmp;
     }
     while (free_node) {
       KevGraphNode* tmp = free_node->next;
       kev_graph_node_pool_deallocate(free_node);
       free_node = tmp;
     }
     return false;
   }
   tail = node;
   node = node->next;
   src_node = src_node->next;
  }

  kev_address_map_destroy(&map);
  
  graph->head = head;
  graph->tail = tail;

  return true;
}
