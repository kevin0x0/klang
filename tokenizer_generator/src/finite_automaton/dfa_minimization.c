#include "tokenizer_generator/include/finite_automaton/bitset/bitset.h"
#include "tokenizer_generator/include/finite_automaton/fa.h"
#include "tokenizer_generator/include/finite_automaton/graph.h"
#include "tokenizer_generator/include/finite_automaton/list/set_cross_list.h"
#include "tokenizer_generator/include/finite_automaton/hashmap/intset_map.h"
#include "tokenizer_generator/include/general/global_def.h"
#include <stdint.h>

static bool kev_compute_all_transition(KevIntSetMap* transition_map, KevFA* dfa, KevBitSet* target_set, uint64_t state_number);
static void kev_destroy_transition_map(KevIntSetMap* map);
static inline  void kev_destroy_transition(KevIntSetMap* map);
static void kev_destroy_partition(KevSetCrossList* partition);
static inline bool kev_do_partition_for_all_transition(KevSetCrossList* partition, KevIntSetMap* transition_map);
static bool kev_do_partition_for_single_transition(KevSetCrossList* partition, KevBitSet* workset);
static KevFA* kev_form_dfa(KevSetCrossList* partition, KevFA* dfa, uint64_t* accept_state_mapping);

KevFA* kev_dfa_minimization(KevFA* dfa, uint64_t* accept_state_mapping) {
  KevSetCrossList partition;
  KevIntSetMap transition_map;
  kev_setcrosslist_init(&partition);
  kev_intsetmap_init(&transition_map, 48);
  uint64_t state_number = kev_fa_state_assign_id(dfa, 0);
  
  /* TODO: initialize partition set */

  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(&partition);
  KevSetCrossListNode* worknode = NULL;
  /* Hopcroft algorithm */
  while ((worknode = kev_setcrosslist_get_workset(&partition)) != end) {
    KevBitSet* target_set = worknode->set;
    if (!kev_compute_all_transition(&transition_map, dfa, target_set, state_number) ||
        !kev_do_partition_for_all_transition(&partition, &transition_map)) {
      kev_destroy_transition_map(&transition_map);
      kev_destroy_partition(&partition);
      return NULL;
    }
    kev_destroy_transition(&transition_map);
    kev_intsetmap_make_empty(&transition_map);
  }
  kev_intsetmap_destroy(&transition_map);
  
  KevFA* min_dfa = kev_form_dfa(&partition, dfa, accept_state_mapping);
  kev_setcrosslist_destroy(&partition);
  return min_dfa;
}

static void kev_destroy_transition_map(KevIntSetMap* map) {
  kev_destroy_transition(map);
  kev_intsetmap_destroy(map);
}

static inline void kev_destroy_transition(KevIntSetMap* map) {
  for (KevIntSetMapNode* itr = kev_intsetmap_iterate_begin(map);
      itr != NULL;
      itr = kev_intsetmap_iterate_next(map, itr)) {
    kev_bitset_delete(itr->value);
  }
}

static bool kev_compute_all_transition(KevIntSetMap* transition_map, KevFA* dfa, KevBitSet* target_set, uint64_t state_number) {
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    while (edge) {
      if (kev_bitset_has_element(target_set, edge->node->id)) {
        KevIntSetMapNode* map_node = kev_intsetmap_search(transition_map, edge->attr);
        KevBitSet* set = NULL;
        if (map_node == NULL) {
          if (!(set = kev_bitset_create(state_number)) ||
              !kev_intsetmap_insert(transition_map, edge->attr, set)) {
            kev_bitset_delete(set);
            return false;
          }
        } else {
          set = map_node->value;
        }
        kev_bitset_set(set, node->id);
      }
      edge = edge->next;
    }
    node = node->next;
  }
  return true;
}

static void kev_destroy_partition(KevSetCrossList* partition) {
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(partition);
      itr != NULL;
      itr = kev_setcrosslist_iterate_next(itr)) {
    kev_bitset_delete(itr->set);
  }
  kev_setcrosslist_destroy(partition);
}

static inline bool kev_do_partition_for_all_transition(KevSetCrossList* partition, KevIntSetMap* transition_map) {
  for (KevIntSetMapNode* itr = kev_intsetmap_iterate_begin(transition_map);
      itr != NULL;
      itr = kev_intsetmap_iterate_next(transition_map, itr)) {
    if (!kev_do_partition_for_single_transition(partition, itr->value))
      return false;
  }
  return true;
}

static bool kev_do_partition_for_single_transition(KevSetCrossList* partition, KevBitSet* workset) {
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(partition);
      itr != NULL;
      itr = kev_setcrosslist_iterate_next(itr)) {
    KevBitSet* intersection = itr->set;
    KevBitSet* difference = kev_bitset_create_copy(intersection);
    if (!difference) return false;
    kev_bitset_difference(difference, workset);
    uint64_t dif_size = kev_bitset_size(difference);
    uint64_t int_size = kev_bitset_size(intersection) - dif_size;
    if (dif_size == 0 || int_size == 0) {
      kev_bitset_delete(difference);
      continue;
    }
    kev_bitset_intersection(intersection, workset);
    if (!kev_setcrosslist_insert(itr, difference)) {
      kev_bitset_delete(difference);
      return false;
    }
    if (kev_setcrosslist_node_in_worklist(itr) || int_size > dif_size)
      kev_setcrosslist_add_to_worklist(partition, itr->p_prev);
    else
      kev_setcrosslist_add_to_worklist(partition, itr);
  }
  return true;
}

static KevFA* kev_form_dfa(KevSetCrossList* partition, KevFA* dfa, uint64_t* accept_state_mapping) {
  /* TODO: iterate all sets in 'partition', create minimized DFA and modify mapping array */
  return NULL;
}
