#include "tokenizer_generator/include/finite_automaton/fa.h"
#include "tokenizer_generator/include/finite_automaton/graph.h"
#include "tokenizer_generator/include/finite_automaton/list/node_list.h"
#include "tokenizer_generator/include/finite_automaton/set/partition.h"
#include "tokenizer_generator/include/finite_automaton/list/set_cross_list.h"
#include "tokenizer_generator/include/finite_automaton/hashmap/intlist_map.h"
#include "tokenizer_generator/include/general/global_def.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEV_DFA_UNKNOWN_STATE          (0)

static bool kev_initialize_hopcroft(KevPartitionUniverse* universe, KevSetCrossList* setlist,
                                    KevFA* dfa, uint64_t* accept_state_mapping);
static bool kev_do_hopcroft(KevSetCrossList* setlist, KevPartitionUniverse universe, KevFA* dfa);
static KevFA* kev_construct_min_dfa(KevSetCrossList* setlist, KevPartitionUniverse universe, KevFA* dfa,
                                    uint64_t* accept_state_mapping);
static bool kev_hopcroft_compute_all_targets(KevIntListMap* all_targets, KevPartitionSet* workset, KevGraphNode* states);
static bool kev_hopcroft_do_partition(KevSetCrossList* setlist, KevPartitionUniverse universe, KevIntListMap* all_targets);
static bool kev_hopcroft_do_partition_for_single_target(KevSetCrossList* setlist, KevPartitionUniverse universe, KevNodeList* target);
static KevGraphNode** kev_construct_min_dfa_states(KevSetCrossList* setlist, KevPartitionUniverse universe,
                                                   uint64_t* p_min_dfa_state_number);
static bool kev_construct_min_dfa_edges(KevGraphNode** min_dfa_states, KevSetCrossList* setlist, KevPartitionUniverse universe);
static KevFA* kev_classify_min_dfa_states(KevGraphNode** min_dfa_states, uint64_t min_dfa_state_number, KevFA* dfa,
                                          uint64_t* accept_state_mapping);
static bool kev_construct_min_dfa_acc_state_map(KevGraphNode** min_dfa_states, uint64_t min_dfa_acc_state_number, KevFA* dfa, uint64_t* accept_state_mapping);
static bool kev_do_pre_partition_for_accept_state(KevIntListMap* map, KevGraphNode* acc_state, uint64_t* accept_state_mapping);

static void kev_destroy_all_list(KevIntListMap* map);
static void kev_destroy_all_sets(KevSetCrossList* setlist);
static void kev_destroy_all_min_dfa_states(KevGraphNode** state_array, uint64_t state_number);

KevFA* kev_dfa_minimization(KevFA* dfa, uint64_t* accept_state_mapping) {
  KevPartitionUniverse universe = NULL;
  KevSetCrossList setlist;
  if (!kev_setcrosslist_init(&setlist)) return NULL;
  if (!kev_initialize_hopcroft(&universe, &setlist, dfa, accept_state_mapping)) {
    kev_setcrosslist_destroy(&setlist);
    return NULL;
  }
  printf("just finished initialize_hopcroft()\n");
  getchar();
  if (!kev_do_hopcroft(&setlist, universe, dfa)) {
    kev_partition_universe_delete(universe);
    kev_destroy_all_sets(&setlist);
    kev_setcrosslist_destroy(&setlist);
    return NULL;
  }
  printf("just finished do_hopcroft()\n");
  getchar();
  KevFA* min_dfa = kev_construct_min_dfa(&setlist, universe, dfa, accept_state_mapping);
  printf("just finished construct_min_dfa()\n");
  getchar();
  kev_partition_universe_delete(universe);
  kev_destroy_all_sets(&setlist);
  kev_setcrosslist_destroy(&setlist);
  return min_dfa;
}

static bool kev_initialize_hopcroft(KevPartitionUniverse* universe, KevSetCrossList* setlist,
                                    KevFA* dfa, uint64_t* accept_state_mapping) {
  uint64_t dfa_state_number = kev_fa_state_number(dfa);
  KevPartitionUniverse universe_set = kev_partition_universe_create(dfa_state_number);
  if (!universe_set) return false;
  uint64_t current_position = 0;
  KevGraphNode* state = kev_fa_get_states(dfa);
  KevGraphNode* acc_state = kev_fa_get_accept_state(dfa);
  KevPartitionSet* set = kev_partition_set_create_from_graphnode(universe_set, state, acc_state, current_position);
  if (!set) {
    kev_partition_universe_delete(universe_set);
    return false;
  }
  current_position = set->end;
  if (!kev_setcrosslist_insert(kev_setcrosslist_iterate_begin(setlist), set)) {
    kev_partition_universe_delete(universe_set);
    kev_partition_set_delete(set);
    return false;
  }

  KevIntListMap pre_partition;
  if (!kev_intlistmap_init(&pre_partition, 16)) {
    kev_partition_universe_delete(universe_set);
      kev_destroy_all_sets(setlist);
    return false;
  }
  if (!kev_do_pre_partition_for_accept_state(&pre_partition, acc_state, accept_state_mapping)) {
    kev_partition_universe_delete(universe_set);
    kev_intlistmap_destroy(&pre_partition);
      kev_destroy_all_sets(setlist);
    return false;
  }
  
  for (KevIntListMapNode* itr = kev_intlistmap_iterate_begin(&pre_partition);
       itr != NULL;
       itr = kev_intlistmap_iterate_next(&pre_partition, itr)) {
    set = kev_partition_set_create(universe_set, itr->value, current_position);
    if (!set) {
      kev_partition_universe_delete(universe_set);
      kev_destroy_all_list(&pre_partition);
      kev_destroy_all_sets(setlist);
      kev_intlistmap_destroy(&pre_partition);
      return false;
    }
    current_position = set->end;
    if (!kev_setcrosslist_insert(kev_setcrosslist_iterate_begin(setlist), set)) {
      kev_partition_universe_delete(universe_set);
      kev_destroy_all_list(&pre_partition);
      kev_destroy_all_sets(setlist);
      kev_intlistmap_destroy(&pre_partition);
      return false;
    }
    kev_setcrosslist_add_to_worklist(setlist, kev_setcrosslist_iterate_begin(setlist));
  }
  *universe = universe_set;
  kev_destroy_all_list(&pre_partition);
  kev_intlistmap_destroy(&pre_partition);
  return true;
}

static bool kev_do_hopcroft(KevSetCrossList* setlist, KevPartitionUniverse universe, KevFA* dfa) {
  KevIntListMap all_targets;
  KevGraphNode* dfa_states = kev_fa_get_states(dfa);
  if (!kev_intlistmap_init(&all_targets, 48))
    return false;
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(setlist);
  KevSetCrossListNode* workset = NULL;
  while ((workset = kev_setcrosslist_get_workset(setlist)) != end) {
    if (!kev_hopcroft_compute_all_targets(&all_targets, workset->set, dfa_states)) {
      kev_destroy_all_list(&all_targets);
      kev_intlistmap_destroy(&all_targets);
      return false;
    }
    if (!kev_hopcroft_do_partition(setlist, universe, &all_targets)) {
      kev_destroy_all_list(&all_targets);
      kev_intlistmap_destroy(&all_targets);
      return false;
    }
    kev_destroy_all_list(&all_targets);
    kev_intlistmap_make_empty(&all_targets);
  }
  kev_intlistmap_destroy(&all_targets);
  return true;
}

static KevFA* kev_construct_min_dfa(KevSetCrossList* setlist, KevPartitionUniverse universe,
                                    KevFA* dfa, uint64_t* accept_state_mapping) {
  uint64_t min_dfa_state_number = 0;
  KevGraphNode** min_dfa_states = kev_construct_min_dfa_states(setlist, universe, &min_dfa_state_number);
  if (!min_dfa_states) return NULL;
  if (!kev_construct_min_dfa_edges(min_dfa_states, setlist, universe)) {
    kev_destroy_all_min_dfa_states(min_dfa_states, min_dfa_state_number);
    free(min_dfa_states);
    return NULL;
  }
  KevFA* min_dfa = kev_classify_min_dfa_states(min_dfa_states, min_dfa_state_number, dfa, accept_state_mapping);
  free(min_dfa_states);
  return min_dfa;
}

static bool kev_hopcroft_compute_all_targets(KevIntListMap* all_targets, KevPartitionSet* restrict workset, KevGraphNode* states) {
  while (states) {
    KevGraphEdge* edge = kev_graphnode_get_edges(states);
    while (edge) {
      if (kev_partition_set_has(workset, edge->node)) {
        KevIntListMapNode* target = kev_intlistmap_search(all_targets, edge->attr);
        if (!target) {
          target = kev_intlistmap_insert(all_targets, edge->attr, NULL);
          if (!target) return false;
        }
        KevNodeList* lst = kev_nodelist_insert(target->value, states);
        if (!lst) return false;
        target->value = lst;
      }
      edge = edge->next;
    }
    states = states->next;
  }
  return true;
}

static bool kev_hopcroft_do_partition(KevSetCrossList* setlist, KevPartitionUniverse universe, KevIntListMap* all_targets) {
  for (KevIntListMapNode* itr = kev_intlistmap_iterate_begin(all_targets);
       itr != NULL;
       itr = kev_intlistmap_iterate_next(all_targets, itr)) {
    if (!kev_hopcroft_do_partition_for_single_target(setlist, universe, itr->value)) {
      return false;
    }
  }
  return true;
}

static void kev_destroy_all_list(KevIntListMap* map) {
  for (KevIntListMapNode* itr = kev_intlistmap_iterate_begin(map);
       itr != NULL;
       itr = kev_intlistmap_iterate_next(map, itr)) {
    kev_nodelist_delete(itr->value);
  }
}

static bool kev_hopcroft_do_partition_for_single_target(KevSetCrossList* setlist, KevPartitionUniverse universe, KevNodeList* target) {
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(setlist);
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(setlist);
       itr != end;
       itr = kev_setcrosslist_iterate_next(itr)) {
    KevPartitionSet* intersection = kev_partition_refine(universe, itr->set, target);
    if (!intersection) return false;
    uint64_t int_size = kev_partition_set_size(intersection);
    uint64_t dif_size = kev_partition_set_size(itr->set);
    if (int_size == 0 || dif_size == 0) {
      if (dif_size == 0) {
        kev_partition_set_delete(itr->set);
        itr->set = intersection;
      }
      continue;
    }
    KevSetCrossListNode* new_node = kev_setcrosslist_insert(itr, intersection);
    if (!new_node) {
      kev_partition_set_delete(intersection);
      return false;
    }
    if (kev_setcrosslist_node_in_worklist(itr) || int_size < dif_size) {
      kev_setcrosslist_add_to_worklist(setlist, new_node);
    } else {
      kev_setcrosslist_add_to_worklist(setlist, itr);
    }
  }
  return true;
}

static void kev_destroy_all_sets(KevSetCrossList* setlist) {
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(setlist);
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(setlist);
       itr != end;
       itr = kev_setcrosslist_iterate_next(itr)) {
    kev_partition_set_delete(itr->set);
  }
}

static KevGraphNode** kev_construct_min_dfa_states(KevSetCrossList* setlist, KevPartitionUniverse universe,
                                                   uint64_t* p_min_dfa_state_number) {
  uint64_t min_dfa_state_number = 0;
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(setlist);
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(setlist);
       itr != end;
       itr = kev_setcrosslist_iterate_next(itr), ++min_dfa_state_number) {
    kev_partition_indeces(universe, itr->set, min_dfa_state_number);
  }
  KevGraphNode** state_array = (KevGraphNode**)malloc(sizeof (KevGraphNode*) * min_dfa_state_number);
  if (!state_array) return NULL;
  for (uint64_t i = 0; i < min_dfa_state_number; ++i) {
    if (!(state_array[i] = kev_graphnode_create(KEV_DFA_UNKNOWN_STATE))) {
      kev_destroy_all_min_dfa_states(state_array, i);
      free(state_array);
      return NULL;
    }
  }
  *p_min_dfa_state_number = min_dfa_state_number;
  return state_array;
}

static bool kev_construct_min_dfa_edges(KevGraphNode** min_dfa_states, KevSetCrossList* setlist,
                                        KevPartitionUniverse universe) {
  uint64_t i = 0;
  KevSetCrossListNode* end = kev_setcrosslist_iterate_end(setlist);
  for (KevSetCrossListNode* itr = kev_setcrosslist_iterate_begin(setlist);
       itr != end;
       itr = kev_setcrosslist_iterate_next(itr), ++i) {
    KevGraphNode* representative = kev_partition_set_representative(universe, itr->set);
    KevGraphEdge* edge = kev_graphnode_get_edges(representative);
    while (edge) {
      if (!kev_graphnode_connect(min_dfa_states[i], min_dfa_states[edge->node->id], edge->attr)) {
        return false;
      }
      edge = edge->next;
    }
  }
  return true;
}

static void kev_destroy_all_min_dfa_states(KevGraphNode** state_array, uint64_t state_number) {
  for (uint64_t i = 0; i < state_number; ++i) {
    kev_graphnode_delete(state_array[i]);
  }
}

static KevFA* kev_classify_min_dfa_states(KevGraphNode** min_dfa_states, uint64_t min_dfa_state_number,
                                        KevFA* dfa, uint64_t* accept_state_mapping) {
  KevGraphNode* min_dfa_accept_states = NULL;
  KevGraphNode* min_dfa_all_states = NULL;
  KevGraphNode* dfa_accept_state = kev_fa_get_accept_state(dfa);
  uint64_t min_dfa_acc_state_number = 0;
  while (dfa_accept_state) {
    KevGraphNode* min_dfa_acc_state = min_dfa_states[dfa_accept_state->id];
    if (min_dfa_acc_state->id == KEV_DFA_UNKNOWN_STATE) {
      min_dfa_acc_state->next = min_dfa_accept_states;
      min_dfa_accept_states = min_dfa_acc_state;
      min_dfa_acc_state->id = ++min_dfa_acc_state_number;
    }
    dfa_accept_state = dfa_accept_state->next;
  }
  min_dfa_all_states = min_dfa_accept_states;
  for (uint64_t i = 0; i < min_dfa_state_number; ++i) {
    KevGraphNode* state = min_dfa_states[i];
    if (state->id == KEV_DFA_UNKNOWN_STATE) {
      state->next = min_dfa_all_states;
      min_dfa_all_states = state;
    }
  }
  if (accept_state_mapping) {
    if (!kev_construct_min_dfa_acc_state_map(min_dfa_states, min_dfa_acc_state_number, dfa, accept_state_mapping))
      return NULL;
  }
  KevGraphNode* min_dfa_start_state = min_dfa_states[kev_fa_get_states(dfa)->id];
  return kev_fa_create_set(min_dfa_all_states, min_dfa_start_state, min_dfa_accept_states);
}

static bool kev_construct_min_dfa_acc_state_map(KevGraphNode** min_dfa_states, uint64_t min_dfa_acc_state_number, KevFA* dfa, uint64_t* accept_state_mapping) {
  uint64_t* min_dfa_acc_state_mapping = (uint64_t*)malloc(sizeof (uint64_t) * min_dfa_acc_state_number);
  if (!min_dfa_acc_state_mapping) return false;
  KevGraphNode* dfa_acc_state = kev_fa_get_accept_state(dfa);
  uint64_t i = 0;
  while (dfa_acc_state) {
    min_dfa_acc_state_mapping[min_dfa_acc_state_number - min_dfa_states[dfa_acc_state->id]->id] = accept_state_mapping[i++];
    dfa_acc_state = dfa_acc_state->next;
  }
  memcpy(accept_state_mapping, min_dfa_acc_state_mapping, sizeof (uint64_t) * min_dfa_acc_state_number);
  free(min_dfa_acc_state_mapping);
  return true;
}

static bool kev_do_pre_partition_for_accept_state(KevIntListMap* pre_partition, KevGraphNode* acc_state, uint64_t* accept_state_mapping) {
  uint64_t i = 0;
  while (acc_state) {
    uint64_t owner = accept_state_mapping ? accept_state_mapping[i] : 0;
    ++i;
    KevIntListMapNode* lstnode = kev_intlistmap_search(pre_partition, owner);
    if (!lstnode) {
      lstnode = kev_intlistmap_insert(pre_partition, owner, NULL);
      if (!lstnode) {
        kev_destroy_all_list(pre_partition);
        kev_intlistmap_destroy(pre_partition);
        return false;
      }
    }
    KevNodeList* lst = kev_nodelist_insert(lstnode->value, acc_state);
    if (!lst) {
        kev_destroy_all_list(pre_partition);
        kev_intlistmap_destroy(pre_partition);
        return false;
    }
    lstnode->value = lst;
    acc_state = acc_state->next;
  }
  return true;
}
