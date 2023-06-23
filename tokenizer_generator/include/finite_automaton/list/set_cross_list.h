#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_LIST_BITSET_CROSS_LIST_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_LIST_BITSET_CROSS_LIST_H
/* used in kev_dfa_minimization(hopcroft algorithm) */

#include "tokenizer_generator/include/finite_automaton/bitset/bitset.h"
#include "tokenizer_generator/include/general/global_def.h"
#include <stdint.h>

typedef struct tagKevSetCrossListNode {
  KevBitSet* set;
  uint64_t set_size;
  struct tagKevSetCrossListNode* p_prev;
  struct tagKevSetCrossListNode* p_next;
  struct tagKevSetCrossListNode* w_prev;
  struct tagKevSetCrossListNode* w_next;
} KevSetCrossListNode;

typedef struct tagKevBitSetCrossList {
  KevSetCrossListNode head_sentinel;
  KevSetCrossListNode tail_sentinel;
} KevSetCrossList;

static inline bool kev_setcrosslist_init(KevSetCrossList* crosslist);
void kev_setcrosslist_destroy(KevSetCrossList* crosslist);
KevSetCrossListNode* kev_setcrosslist_insert(KevSetCrossListNode* position, KevBitSet* set, uint64_t set_size);
//KevSetCrossListNode* kev_setcrosslist_remove(KevSetCrossListNode* position);
static inline bool kev_setcrosslist_node_in_worklist(KevSetCrossListNode* node);
static inline void kev_setcrosslist_add_to_worklist(KevSetCrossList* crosslist, KevSetCrossListNode* position);
static inline KevSetCrossListNode* kev_setcrosslist_get_workset(KevSetCrossList* crosslist);
uint64_t kev_setcrosslist_size(KevSetCrossList* crosslist);
static inline KevSetCrossListNode* kev_setcrosslist_iterate_begin(KevSetCrossList* crosslist);
static inline KevSetCrossListNode* kev_setcrosslist_iterate_end(KevSetCrossList* crosslist);
static inline KevSetCrossListNode* kev_setcrosslist_iterate_next(KevSetCrossListNode* current);


static inline bool kev_setcrosslist_init(KevSetCrossList* crosslist) {
  if (!crosslist) return false;
  crosslist->head_sentinel.p_prev = NULL;
  crosslist->head_sentinel.w_prev = NULL;
  crosslist->head_sentinel.p_next = &crosslist->tail_sentinel;
  crosslist->head_sentinel.w_next = &crosslist->tail_sentinel;
  crosslist->tail_sentinel.p_next = NULL;
  crosslist->tail_sentinel.w_next = NULL;
  crosslist->tail_sentinel.p_prev = &crosslist->head_sentinel;
  crosslist->tail_sentinel.w_prev = &crosslist->head_sentinel;
  return true;
}

static inline bool kev_setcrosslist_node_in_worklist(KevSetCrossListNode* node) {
  return node->w_next;  /* node->w_prev is NULL if and only if node is in worklist */
}

static inline void kev_setcrosslist_add_to_worklist(KevSetCrossList* crosslist, KevSetCrossListNode* position) {
  position->w_next = &crosslist->tail_sentinel;
  position->w_prev = crosslist->tail_sentinel.w_prev;
  crosslist->tail_sentinel.w_prev = position;
  position->w_prev->w_next = position;
}

static inline KevSetCrossListNode* kev_setcrosslist_get_workset(KevSetCrossList* crosslist) {
  KevSetCrossListNode* ret = crosslist->head_sentinel.w_next;
  if (crosslist->head_sentinel.w_next == &crosslist->tail_sentinel)
    return ret;
  crosslist->head_sentinel.w_next = ret->w_next;
  ret->w_next->w_prev = &crosslist->head_sentinel;
  ret->w_next = NULL;
  return ret;
}

static inline KevSetCrossListNode* kev_setcrosslist_iterate_begin(KevSetCrossList* crosslist) {
  return crosslist->head_sentinel.p_next;
}

static inline KevSetCrossListNode* kev_setcrosslist_iterate_end(KevSetCrossList* crosslist) {
  return &crosslist->tail_sentinel;
}

static inline KevSetCrossListNode* kev_setcrosslist_iterate_next(KevSetCrossListNode* current) {
  return current->p_next;
}

#endif
