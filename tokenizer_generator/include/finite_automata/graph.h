#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_GRAPH_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_GRAPH_H

#include "tokenizer_generator/include/general/int_type.h"

struct tagKevGraphEdgeList;

typedef int64_t KevGraphNodeAttr;
typedef int64_t KevGraphEdgeAttr;

typedef struct tagKevGraphNodeList {
  KevGraphNodeAttr attr;
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
bool kev_graph_init_copy(KevGraph* graph, KevGraph* from);
void kev_graph_destroy(KevGraph* graph);
KevGraph* kev_graph_create(KevGraphNode* node);
void kev_graph_delete(KevGraph* graph);
/* Merge 'from' to 'to'. Then 'from' would be an empty graph */
bool kev_graph_merge(KevGraph* to, KevGraph* from);
static inline void kev_graph_add_node(KevGraph* graph, KevGraphNode* node);
static inline KevGraphNodeList* kev_graph_get_nodes(KevGraph* graph);

KevGraphNode* kev_graphnode_create(KevGraphNodeAttr attr);
void kev_graphnode_delete(KevGraphNode* node);
bool kev_graphnode_connect(KevGraphNode* from, KevGraphNode* to, KevGraphEdgeAttr attr);
static inline KevGraphEdgeList* kev_graphnode_get_edges(KevGraphNode* node);

static inline KevGraphNodeList* kev_graphnode_list_insert(KevGraphNodeList* lst, KevGraphNode* node) {
  if (!node) return NULL;
  node->next = lst;
  return node;
}

static inline void kev_graph_add_node(KevGraph* graph, KevGraphNode* node) {
  if (!graph || !node) return;
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
    return graph ? graph->head : NULL;
}

static inline KevGraphEdgeList* kev_graphnode_get_edges(KevGraphNode* node) {
  return node ? node->edges : NULL;
}

void kev_graph_pool_free(void);





#endif
