#include "tokenizer_generator/include/finite_automaton/fa.h"
#include "tokenizer_generator/include/finite_automaton/graph.h"
#include "tokenizer_generator/include/finite_automaton/bitset/bitset.h"
#include "tokenizer_generator/include/finite_automaton/list/set_cross_list.h"
#include "tokenizer_generator/include/finite_automaton/hashmap/intset_map.h"
#include "tokenizer_generator/include/general/global_def.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define KEV_MIN_DFA_UNKNOWN_STATE_MARK  (-1)

static bool kev_compute_all_transition(KevIntSetMap* transition_map, KevFA* dfa, KevBitSet* target_set, uint64_t state_number);
static void kev_destroy_transition_map(KevIntSetMap* map);
static inline  void kev_destroy_transition(KevIntSetMap* map);
static void kev_destroy_partition(KevSetCrossList* partition);
static inline bool kev_do_partition_for_all_transition(KevSetCrossList* partition, KevIntSetMap* transition_map);
static bool kev_do_partition_for_single_transition(KevSetCrossList* partition, KevBitSet* workset);
static KevFA* kev_form_dfa(KevSetCrossList* partition, KevFA* dfa, uint64_t* accept_state_mapping, uint64_t state_number);
static uint64_t* kev_indeces_state_ownership(KevSetCrossList* partition, KevGraphNode** dfa_states, uint64_t state_number);
static KevGraphNode** kev_get_min_dfa_states_array(uint64_t state_number);
static KevGraphNode** kev_get_dfa_states_array(KevFA* dfa, uint64_t state_number);
static void kev_destroy_min_dfa_states(KevGraphNode** min_dfa_states, uint64_t state_number);
static KevFA* kev_organize_states(KevGraphNode** min_dfa_states, KevFA* dfa, uint64_t* accept_state_mapping,
                                  uint64_t* state_ownership, uint64_t min_dfa_state_number);
static bool kev_initialize_partition_set(KevSetCrossList* partition, KevFA* dfa, uint64_t* accept_state_mappin, uint64_t state_number);
static void kev_destroy_accept_state_partition_set(KevBitSet** partition_set, uint64_t* partition_set_size, uint64_t set_number);
static uint64_t kev_get_partition_set_number(KevFA* dfa, uint64_t* accept_state_mapping);

KevFA* kev_dfa_minimization(KevFA* dfa, uint64_t* accept_state_mapping) {
  KevSetCrossList partition;
  KevIntSetMap transition_map;
  kev_setcrosslist_init(&partition);
  kev_intsetmap_init(&transition_map, 48);
  uint64_t state_number = kev_fa_state_assign_id(dfa, 0);
  
  /* initialize partition set */
  if (!kev_initialize_partition_set(&partition, dfa, accept_state_mapping, state_number)) {
    kev_setcrosslist_destroy(&partition);
    kev_intsetmap_destroy(&transition_map);
    return NULL;
  }

  /* Hopcroft algorithm */
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(&partition);
  KevSetCrossListNode* worknode = NULL;
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
  
  KevFA* min_dfa = kev_form_dfa(&partition, dfa, accept_state_mapping, state_number);
  kev_destroy_partition(&partition);
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
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(partition);
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(partition);
      itr != end;
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
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(partition);
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(partition);
      itr != end;
      itr = kev_setcrosslist_iterate_next(itr)) {
    KevBitSet* intersection = itr->set;
    KevBitSet* difference = kev_bitset_create_copy(intersection);
    if (!difference) return false;
    kev_bitset_difference(difference, workset);
    uint64_t dif_size = kev_bitset_size(difference);
    uint64_t int_size = itr->set_size - dif_size;
    if (dif_size == 0 || int_size == 0) {
      kev_bitset_delete(difference);
      continue;
    }
    kev_bitset_intersection(intersection, workset);
    itr->set_size = int_size;
    if (!kev_setcrosslist_insert(itr, difference, dif_size)) {
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

static KevFA* kev_form_dfa(KevSetCrossList* partition, KevFA* dfa, uint64_t* accept_state_mapping, uint64_t state_number) {
  KevGraphNode** dfa_states = kev_get_dfa_states_array(dfa, state_number);
  if (!dfa_states) return NULL;
  uint64_t* state_ownership = kev_indeces_state_ownership(partition, dfa_states, state_number);
  uint64_t min_dfa_state_number = kev_setcrosslist_size(partition);
  KevGraphNode** min_dfa_states_array = kev_get_min_dfa_states_array(min_dfa_state_number);
  if (!state_ownership || !min_dfa_states_array) {
    kev_destroy_min_dfa_states(min_dfa_states_array, state_number);
    free(state_ownership);
    free(dfa_states);
    return NULL;
  }
  /* construct edge for each transition */
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(partition);
  uint64_t i = 0;
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(partition);
      itr != end;
      itr = kev_setcrosslist_iterate_next(itr), ++i) {
    uint64_t representative = kev_bitset_iterate_begin(itr->set);
    KevGraphEdge* edge = kev_graphnode_get_edges(dfa_states[representative]);
    while (edge) {
      if (!kev_graphnode_connect(min_dfa_states_array[i], min_dfa_states_array[state_ownership[edge->node->id]], edge->attr)) {
        kev_destroy_min_dfa_states(min_dfa_states_array, state_number);
        free(state_ownership);
        return NULL;
      }
      edge = edge->next;
    }
  }
  free(dfa_states);
  
  KevFA* min_dfa = kev_organize_states(min_dfa_states_array, dfa, accept_state_mapping, state_ownership, min_dfa_state_number);
  free(state_ownership);
  if (!min_dfa) {
    kev_destroy_min_dfa_states(min_dfa_states_array, state_number);
  } else {
    free(min_dfa_states_array);
  }
  return min_dfa;
}

static uint64_t* kev_indeces_state_ownership(KevSetCrossList* partition, KevGraphNode** dfa_states, uint64_t state_number) {
  /* this proccedure is O(n^2) */
  uint64_t* ownership = (uint64_t*)malloc(sizeof (uint64_t) * state_number);
  if (!ownership) return NULL;
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(partition);
  uint64_t i = 0;
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(partition);
      itr != end;
      itr = kev_setcrosslist_iterate_next(itr), ++i) {
    uint64_t state_in_set = 0;
    uint64_t next_state = kev_bitset_iterate_begin(itr->set);
    do {
      state_in_set = next_state;
      ownership[state_in_set] = i;
    next_state = kev_bitset_iterate_begin(itr->set);
    } while (next_state != state_in_set);
  }
  return ownership;
}

static KevGraphNode** kev_get_min_dfa_states_array(uint64_t state_number) {
  KevGraphNode** min_dfa_states = (KevGraphNode**)malloc(sizeof (KevGraphNode*) * state_number);
  if (min_dfa_states) {
    for (uint64_t i = 0; i < state_number; ++i) {
      if (!(min_dfa_states[i] = kev_graphnode_create(KEV_MIN_DFA_UNKNOWN_STATE_MARK))) {
        for (uint64_t j = 0; j < i; ++j)
          kev_graphnode_delete(min_dfa_states[j]);
        free(min_dfa_states);
        return NULL;
      }
    }
  }
  return min_dfa_states;
}

static void kev_destroy_min_dfa_states(KevGraphNode** min_dfa_states, uint64_t state_number) {
  if (!min_dfa_states) return;
  for (uint64_t i = 0; i < state_number; ++i)
    kev_graphnode_delete(min_dfa_states[i]);
  free(min_dfa_states);
  return;
}

static KevGraphNode** kev_get_dfa_states_array(KevFA* dfa, uint64_t state_number) {
  KevGraphNode* node = kev_fa_get_states(dfa);
  KevGraphNode** states_array = (KevGraphNode**)malloc(sizeof (KevGraphNode*) * state_number);
  if (!states_array) return NULL;
  while (node) {
    states_array[node->id] = node;
    node = node->next;
  }
  return states_array;
}

static KevFA* kev_organize_states(KevGraphNode** min_dfa_states, KevFA* dfa, uint64_t* accept_state_mapping, uint64_t* state_ownership, uint64_t min_dfa_state_number) {
  KevGraphNode* accept_states = NULL;
  KevGraphNode* all_states = NULL;
  KevGraphNode* accept_state_in_dfa = kev_fa_get_accept_state(dfa);
  uint64_t accept_state_number = 0;
  /* find all accept state for min DFA and assign id */
  while (accept_state_in_dfa) {
    KevGraphNode* corresponding_state = min_dfa_states[state_ownership[accept_state_in_dfa->id]];
    if (corresponding_state->id == KEV_MIN_DFA_UNKNOWN_STATE_MARK) {
      corresponding_state->id = ++accept_state_number;
      corresponding_state->next = accept_states;
      accept_states = corresponding_state;
      accept_state_in_dfa = accept_state_in_dfa->next;
    }
  }
  /* construct accept_state_mapping for min DFA */
  if (accept_state_mapping) {
    uint64_t* min_dfa_accept_state_mapping = (uint64_t*)malloc(sizeof (uint64_t) * accept_state_number);
    if (!min_dfa_accept_state_mapping) return NULL;
    accept_state_in_dfa = kev_fa_get_accept_state(dfa);
    uint64_t i = 0;
    while (accept_state_in_dfa) {
      KevGraphNode* corresponding_state = min_dfa_states[state_ownership[accept_state_in_dfa->id]];
      min_dfa_accept_state_mapping[accept_state_number - corresponding_state->id] = accept_state_mapping[i++];
      accept_state_in_dfa = accept_state_in_dfa->next;
    }
    memcpy(accept_state_mapping, min_dfa_accept_state_mapping, min_dfa_state_number * sizeof (uint64_t));
    free(min_dfa_accept_state_mapping);
  }
  
  /* add all states */
  all_states = accept_states;
  for (uint64_t i = 0; i < min_dfa_state_number; ++i) {
    KevGraphNode* state = min_dfa_states[i];
    if (state->id != KEV_MIN_DFA_UNKNOWN_STATE_MARK)
      continue;
    state->next = all_states;
    all_states = state;
  }
  
  KevGraphNode* start_state = min_dfa_states[state_ownership[kev_fa_get_start_state(dfa)->id]];
  return kev_fa_create_set(all_states, start_state, accept_states);
}

static bool kev_initialize_partition_set(KevSetCrossList* partition, KevFA* dfa, uint64_t* accept_state_mapping, uint64_t state_number) {
  uint64_t partition_set_number = kev_get_partition_set_number(dfa, accept_state_mapping);
  KevBitSet** accept_state_partition = (KevBitSet**)malloc(sizeof (KevBitSet*) * partition_set_number);
  uint64_t* partition_set_size = (uint64_t*)malloc(sizeof (uint64_t) * partition_set_number);
  if (!partition_set_size || !accept_state_partition) {
    free(accept_state_partition);
    free(partition_set_size);
    return false;
  }
  /* initialize partition_set */
  for (uint64_t i = 0; i < partition_set_number; ++i) {
    partition_set_size[i] = 0;
    if (!(accept_state_partition[i] = kev_bitset_create(state_number))) {
      kev_destroy_accept_state_partition_set(accept_state_partition, partition_set_size, i);
      return false;
    }
  }

  /* create non-accepting state set and add it to partition */
  KevBitSet* non_accept_states = kev_bitset_create(state_number);
  if (!non_accept_states) {
    kev_destroy_accept_state_partition_set(accept_state_partition, partition_set_size, partition_set_number);
    return false;
  }
  KevGraphNode* state = kev_fa_get_states(dfa);
  KevGraphNode* accept_state = kev_fa_get_accept_state(dfa);
  uint64_t non_accept_states_size = 0;
  while (state != accept_state) {
    kev_bitset_set(non_accept_states, state->id);
    state = state->next;
    non_accept_states_size++;
  }

  if (non_accept_states_size != 0) {
    if (!kev_setcrosslist_insert(kev_setcrosslist_iterate_begin(partition),
                                 non_accept_states, non_accept_states_size)) {
      kev_destroy_accept_state_partition_set(accept_state_partition, partition_set_size, partition_set_number);
      kev_bitset_delete(non_accept_states);
      return false;
    }
  } else {
    kev_bitset_delete(non_accept_states);
  }
  
  uint64_t i = 0;
  while (accept_state) {
    uint64_t set_index = accept_state_mapping ? accept_state_mapping[i] : 0;
    partition_set_size[set_index]++;
    kev_bitset_set(accept_state_partition[set_index], accept_state->id);
    accept_state = accept_state->next;
    ++i;
  }
  
  for (uint64_t i = 0; i < partition_set_number; ++i) {
    if (partition_set_size[i] == 0) {
      kev_bitset_delete(accept_state_partition[i]);
      continue;
    }
    KevSetCrossListNode* node = kev_setcrosslist_insert(kev_setcrosslist_iterate_begin(partition),
                                                        accept_state_partition[i], partition_set_size[i]);
    if (!node) {
      for (uint64_t j = i; j < partition_set_number; ++j)
        kev_bitset_delete(accept_state_partition[j]);
      free(accept_state_partition);
      free(partition_set_size);
      return false;
    }
    kev_setcrosslist_add_to_worklist(partition, node);
  }

  free(accept_state_partition);
  free(partition_set_size);
  return true;
}

static void kev_destroy_accept_state_partition_set(KevBitSet** partition_set, uint64_t* partition_set_size, uint64_t set_number) {
  for (uint64_t i = 0; i < set_number; ++i) {
    kev_bitset_delete(partition_set[i]);
  }
  free(partition_set);
  free(partition_set_size);
}

static uint64_t kev_get_partition_set_number(KevFA* dfa, uint64_t* accept_state_mapping) {
  if (!accept_state_mapping) return 1;
  uint64_t max_owner = 0;
  KevGraphNode* acc_node = kev_fa_get_accept_state(dfa);
  while (acc_node) {
    if (*accept_state_mapping > max_owner)
      max_owner = *accept_state_mapping;
    accept_state_mapping++;
    acc_node = acc_node->next;
  }
  return max_owner + 1;
}
