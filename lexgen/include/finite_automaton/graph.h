#ifndef KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_GRAPH_H
#define KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_GRAPH_H

#include "lexgen/include/general/global_def.h"

struct tagKevGraphEdgeList;

typedef int KevGraphNodeId;
typedef int64_t KevGraphEdgeAttr;

typedef struct tagKevGraphNodeList {
  KevGraphNodeId id;
  struct tagKevGraphEdgeList* edges;
  struct tagKevGraphNodeList* next;
} KevGraphNodeList;

typedef KevGraphNodeList KevGraphNode;

typedef struct tagKevGraphEdgeList {
  KevGraphEdgeAttr attr;
  struct tagKevGraphNodeList* node;
  struct tagKevGraphEdgeList* next;
} KevGraphEdgeList;

typedef KevGraphEdgeList KevGraphEdge;

typedef struct tagKevGraph {
  KevGraphNodeList* head;
  KevGraphNodeList* tail;
} KevGraph;

bool kev_graph_init(KevGraph* graph, KevGraphNodeList* nodelist);
static inline bool kev_graph_init_set(KevGraph* graph, KevGraphNode* head, KevGraphNode* tail);
/* copy 'src' with order of states unchanged */
bool kev_graph_init_copy(KevGraph* graph, KevGraph* src);
static inline bool kev_graph_init_move(KevGraph* graph, KevGraph* src);
void kev_graph_destroy(KevGraph* graph);
KevGraph* kev_graph_create(KevGraphNode* node);
bool kev_graph_create_move(KevGraph* src);
void kev_graph_delete(KevGraph* graph);
/* Merge 'src' to 'dest'. 'src' would be appended to the end of 'dest' */
bool kev_graph_merge(KevGraph* dest, KevGraph* src);
static inline void kev_graph_insert_node(KevGraph* graph, KevGraphNode* node);
static inline void kev_graph_append_node(KevGraph* graph, KevGraphNode* node);
static inline KevGraphNodeList* kev_graph_get_nodes(KevGraph* graph);

KevGraphNode* kev_graphnode_create(KevGraphNodeId id);
void kev_graphnode_delete(KevGraphNode* node);
bool kev_graphnode_connect(KevGraphNode* from, KevGraphNode* to, KevGraphEdgeAttr attr);
static inline KevGraphEdgeList* kev_graphnode_get_edges(KevGraphNode* node);

static inline bool kev_graph_init_set(KevGraph* graph, KevGraphNode* head, KevGraphNode* tail) {
  if (!graph) return false;
  graph->head = head;
  graph->tail = tail;
  return true;
}
static inline bool kev_graph_init_move(KevGraph* graph, KevGraph* src) {
  if (!graph || !src) return false;
  graph->head = src->head;
  graph->tail = src->tail;
  src->head = NULL;
  src->tail = NULL;
  return true;
}

static inline KevGraphNodeList* kev_graphnodelist_insert(KevGraphNodeList* lst, KevGraphNode* node) {
  if (!node) return lst;
  node->next = lst;
  return node;
}

static inline void kev_graph_insert_node(KevGraph* graph, KevGraphNode* node) {
  node->next = graph->head;
  graph->head = node;
  if (!graph->tail) graph->tail = node;
}

static inline void kev_graph_append_node(KevGraph* graph, KevGraphNode* node) {
  node->next = NULL;
  if (graph->tail) {
    graph->tail->next = node;
    graph->tail = node;
  } else {
    graph->head = node;
    graph->tail = node;
  }
}

static inline KevGraphNodeList* kev_graph_get_nodes(KevGraph* graph) {
    return graph->head;
}

static inline KevGraphEdgeList* kev_graphnode_get_edges(KevGraphNode* node) {
  return node->edges;
}

#endif
