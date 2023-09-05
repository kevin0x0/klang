#include "kevfa/include/list/node_list.h"
#include "kevfa/include/object_pool/fa_pool.h"
#include "kevfa/include/object_pool/nodelist_node_pool.h"

void kev_nodelist_delete(KevNodeList* list) {
  while (list) {
    kev_nodelist_node_pool_deallocate(list);
    list = list->next;
  }
}

KevNodeList* kev_nodelist_insert(KevNodeList* list, KevGraphNode* element) {
  KevNodeListNode* new_node = kev_nodelist_node_pool_allocate();
  if (!new_node) return NULL;
  new_node->element = element;
  new_node->next = list;
  return new_node;
}

