#ifndef KEVCC_KEVFA_INCLUDE_NODE_LIST_LIST_H
#define KEVCC_KEVFA_INCLUDE_NODE_LIST_LIST_H
/* used in kev_dfa_minimization(hopcroft algorithm) */

#include "kevfa/include/graph.h"
#include "utils/include/general/global_def.h"

typedef struct tagKevNodeListNode {
  KevGraphNode* element;
  struct tagKevNodeListNode* next;
} KevNodeListNode;

typedef KevNodeListNode KevNodeList;

static inline KevNodeList* kev_nodelist_create(void);
void kev_nodelist_delete(KevNodeList* list);

KevNodeList* kev_nodelist_insert(KevNodeList* list, KevGraphNode* element);

static inline KevNodeList* kev_nodelist_create(void) {
  return NULL;
}

#endif
