#include "kevfa/include/list/set_cross_list.h"
#include "kevfa/include/object_pool/set_cross_list_node_pool.h"
#include "utils/include/general/global_def.h"

void kev_setcrosslist_destroy(KevSetCrossList* crosslist) {
  if (!crosslist) return;
  
  KevSetCrossListNode* curr = crosslist->head_sentinel.p_next;
  KevSetCrossListNode* end = &crosslist->tail_sentinel;
  while (curr != end) {
    KevSetCrossListNode* tmp = curr->p_next;
    kev_set_cross_list_node_pool_deallocate(curr);
    curr = tmp;
  }
  crosslist->head_sentinel.p_next = &crosslist->tail_sentinel;
  crosslist->head_sentinel.w_next = &crosslist->tail_sentinel;
  crosslist->tail_sentinel.p_prev = &crosslist->head_sentinel;
  crosslist->tail_sentinel.w_prev = &crosslist->head_sentinel;
}

KevSetCrossListNode* kev_setcrosslist_insert(KevSetCrossListNode* position, KevPartitionSet* set) {
  KevSetCrossListNode* new_node = kev_set_cross_list_node_pool_allocate();
  if (!new_node) return new_node;
  new_node->set = set;
  new_node->w_next = NULL;
  new_node->p_prev = position->p_prev;
  new_node->p_next = position;
  position->p_prev = new_node;
  new_node->p_prev->p_next = new_node;
  return new_node;
}

size_t kev_setcrosslist_size(KevSetCrossList* crosslist) {
  size_t count = 0;
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(crosslist);
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(crosslist);
      itr != end;
      itr = kev_setcrosslist_iterate_next(itr)) {
    ++count;
  }
  return count;
}
