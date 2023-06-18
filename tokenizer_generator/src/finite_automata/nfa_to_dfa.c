#include "tokenizer_generator/include/finite_automata/nfa_to_dfa.h"
#include "tokenizer_generator/include/finite_automata/dfa.h"
#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/finite_automata/nfa.h"
#include "tokenizer_generator/include/finite_automata/bitset/bitset.h"
#include "tokenizer_generator/include/finite_automata/hashmap/intset_map.h"
#include "tokenizer_generator/include/finite_automata/hashmap/setint_map.h"
#include "tokenizer_generator/include/finite_automata/queue/int_queue.h"
#include "tokenizer_generator/include/finite_automata/array/set_array.h"
#include "tokenizer_generator/include/finite_automata/array/node_array.h"
#include "tokenizer_generator/include/general/int_type.h"

#include <stdint.h>
#include <stdlib.h>


/* get size of NULL-terminated array */
static inline uint64_t kev_get_array_size(KevNFA** nfa_array) {
  KevNFA** begin = nfa_array;
  KevNFA** end = nfa_array;
  while (*end) end++;
  return end - begin;
}

/* build mapping array between node number and address of node */
static KevGraphNode** kev_build_node_mapping_array(KevNFA** nfa_array, uint64_t state_number) {
  KevGraphNode** mapping_array = (KevGraphNode**)malloc(sizeof (KevGraphNode*) * state_number);
  if (!mapping_array) return NULL;
  KevNFA** pnfa = nfa_array - 1;
  while (*++pnfa != NULL) {
    KevGraphNodeList* nodelst = kev_graph_get_nodes(&(*pnfa)->transition);
    KevGraphNode* node = nodelst;
    while (node) {
      mapping_array[node->attr] = node;
      node = node->next;
    }
  }
  return mapping_array;
}

/* compute epsilon-closure of state */
static bool kev_compute_state_closure(uint64_t state, KevGraphNode** state_mapping, KevBitSet* closure, KevIntQueue* queue) {
  if (!kev_bitset_set(closure, state) || !kev_intqueue_insert(queue, state))
    return false;
  
  while (!kev_intqueue_empty(queue)) {
    uint64_t state = kev_intqueue_pop(queue);
    KevGraphEdge* edge = state_mapping[state]->edges;
    while (edge) {
      if (edge->attr == KEV_NFA_SYMBOL_EPSILON &&
          !kev_bitset_has_element(closure, edge->node->attr)) {
        if (!kev_bitset_set(closure, edge->node->attr) ||
            !kev_intqueue_insert(queue, edge->node->attr)) {
          return false;
        }
      }
      edge = edge->next;
    }
  }
  return true;
}

/* compute e-closure for every state */
static KevBitSet* kev_compute_closure_array(KevGraphNode** state_mapping, uint64_t state_number) {
  KevBitSet* state_closure = (KevBitSet*)malloc(sizeof (KevBitSet) * state_number);
  if (!state_closure) return NULL;
  KevIntQueue queue;
  if (!kev_intqueue_init(&queue)) {
    free(state_closure);
    return NULL;
  }

  for (uint64_t i = 0; i < state_number; ++i) {
    if (!kev_bitset_init(state_closure + i, state_number) ||
        !kev_compute_state_closure(i, state_mapping, state_closure + i, &queue)) {
      for (uint64_t j = 0; j < i; ++j)
        kev_bitset_destroy(state_closure + j);
      free(state_closure);
      kev_intqueue_destroy(&queue);
      return NULL;
    }
  }
  kev_intqueue_destroy(&queue);
  return state_closure;
}

static void kev_destroy_closure_array(KevBitSet* closure_array, uint64_t state_number) {
  if (!closure_array) return;
  for (uint64_t i = 0; i < state_number; ++i)
    kev_bitset_destroy(closure_array + i);
  free(closure_array);
}

static inline KevBitSet* kev_compute_start_state_set(KevNFA** nfa_array, KevBitSet* closures, uint64_t state_number) {
  KevBitSet* start_state_set = kev_bitset_create(state_number);
  if (start_state_set) {
    KevNFA** pnfa = nfa_array - 1;
    while (*++pnfa)
      kev_bitset_union(start_state_set, &closures[kev_nfa_get_start_state(*pnfa)->attr]);
  }
  return start_state_set;
}

static inline KevBitSet* kev_compute_accept_state_set(KevNFA** nfa_array, uint64_t state_number) {
  KevBitSet* accept_state_set = kev_bitset_create(state_number);
  if (accept_state_set) {
    KevNFA** pnfa = nfa_array - 1;
    while (*++pnfa)
      kev_bitset_set(accept_state_set, kev_nfa_get_accept_state(*pnfa)->attr);
  }
  return accept_state_set;
}

static inline bool kev_state_sets_push_back(KevSetIntMap* map, KevSetArray* sets, KevNodeArray* dfa_states, KevBitSet* set) {
  KevGraphNode* new_node = kev_graphnode_create(0);
  if (!new_node || !kev_nodearray_push_back(dfa_states, new_node)) {
    kev_graphnode_delete(new_node);
    return false;
  }
  return kev_setint_map_insert(map, set, kev_setarray_size(sets)) &&
         kev_setarray_push_back(sets, set);
}

static inline void kev_destroy_state_sets(KevSetIntMap* map, KevSetArray* sets, KevNodeArray* nodes) {
  uint64_t size = kev_setarray_size(sets);
  for (uint64_t i = 0; i < size; ++i)
    kev_bitset_delete(kev_setarray_visit(sets, i));
  kev_setarray_destroy(sets);
  kev_setint_map_destroy(map);
  for (uint64_t i = 0; i < size; ++i) {
    kev_graphnode_delete(kev_nodearray_visit(nodes, i));
  }
  kev_nodearray_destroy(nodes);
}

static bool kev_compute_all_transition(KevIntSetMap* transition_map, KevBitSet* closure,
                                       KevBitSet* closures, KevGraphNode** state_mapping) {
  uint64_t state = 0;
  uint64_t next_state = kev_bitset_iterate_begin(closure);

  do {
    state = next_state;
    KevGraphEdge* edge = kev_graphnode_get_edges(state_mapping[state]);
    while (edge) {
      if (edge->attr != KEV_NFA_SYMBOL_EPSILON) {
        KevIntSetMapNode* map_node = kev_intset_map_search(transition_map, edge->attr);
        KevBitSet* set = NULL;
        if (map_node == NULL) {
          if (!(set = kev_bitset_create_copy(&closures[edge->node->attr])) ||
              !kev_intset_map_insert(transition_map, edge->attr, set))
            return false;
        } else {
          kev_bitset_union(map_node->value, &closures[edge->node->attr]);
        }
      }
      edge = edge->next;
    }
    next_state = kev_bitset_iterate_next(closure, state);
  } while (next_state != state);
  return true;
}

static void kev_destroy_transition_map(KevIntSetMap* map) {
  for (KevIntSetMapNode* itr = kev_intset_map_iterate_begin(map);
      itr != NULL;
      itr = kev_intset_map_iterate_next(map, itr)) {
    kev_bitset_delete(itr->value);
  }
  kev_intset_map_destroy(map);
}

static bool kev_update_transition_and_state_set(KevIntSetMap* transition_map, KevSetIntMap* set_index_map,
                                    KevSetArray* sets, KevNodeArray* dfa_states, KevGraphNode* state) {
  for (KevIntSetMapNode* itr = kev_intset_map_iterate_begin(transition_map);
      itr != NULL;
      itr = kev_intset_map_iterate_next(transition_map, itr)) {
    KevSetIntMapNode* target_node = kev_setint_map_search(set_index_map, itr->value);
    if (target_node == NULL) {
      uint64_t target_state_no = kev_nodearray_size(dfa_states);
      if (!kev_state_sets_push_back(set_index_map, sets, dfa_states, itr->value)) {
        return false;
      }
      itr->value = NULL;
      if (!kev_graphnode_connect(state, kev_nodearray_visit(dfa_states, target_state_no), itr->key)) {
        return false;
      }
    } else {
      kev_bitset_delete(itr->value);
      itr->value = NULL;
      if (!kev_graphnode_connect(state, kev_nodearray_visit(dfa_states, target_node->value), itr->key)) {
        return false;
      }
    }
  }
  return true;
}

static uint64_t* kev_get_state_ownership_array(KevNFA** nfa_array, uint64_t state_number) {
  uint64_t* ownership = (uint64_t*)malloc(sizeof (uint64_t) * state_number);
  if (ownership) {
    for (uint64_t i = 0; i < state_number; ++i) {
      KevGraphNode* node = kev_graph_get_nodes(&nfa_array[i]->transition);
      while (node) {
        ownership[node->attr] = i;
        node = node->next;
      }
    }
  }
  return ownership;
}

static KevDFA* kev_construct_dfa(KevNodeArray* dfa_states, KevSetArray* state_sets, KevBitSet* accept_state_set,
                                            uint64_t* state_ownership, uint64_t* dfa_accept_state_number) {
  KevDFA* dfa = kev_dfa_create();
  if (!dfa) return NULL;
  uint64_t size = kev_setarray_size(state_sets);
  KevGraphNode* accept_states = NULL;
  KevGraphNode* normal_state_tail = NULL;
  KevGraphNode* normal_states = NULL;
  uint64_t accept_state_number = 0;
  for (uint64_t i = 0; i < size; ++i) {
    KevGraphNode* dfa_node = kev_nodearray_visit(dfa_states, i);
    //KevBitSet* state_set = kev_setarray_visit(state_sets, i);
    KevBitSet* state_set = kev_bitset_create_copy(kev_setarray_visit(state_sets, i));
    kev_bitset_intersection(state_set, accept_state_set);
    if (!kev_bitset_empty(state_set)) {
      uint64_t min_accept_state = kev_bitset_iterate_begin(state_set);
      if (state_ownership)
        dfa_node->attr = state_ownership[min_accept_state];
      dfa_node->next = accept_states;
      accept_states = dfa_node;
      accept_state_number++;
    } else {
      dfa_node->next = normal_states;
      if (!normal_states) normal_state_tail = dfa_node;
      normal_states = dfa_node;
    }
    kev_bitset_delete(state_set);
  }
  if (normal_state_tail) {
    normal_state_tail->next = accept_states;
  } else {
    normal_states = accept_states;
  }
  dfa->start_state = kev_nodearray_visit(dfa_states, 0);
  dfa->accept_states = accept_states;
  kev_graph_init(&dfa->transition, normal_states);
  *dfa_accept_state_number = accept_state_number;
  return dfa;
}

KevDFA* kev_nfa_to_dfa(KevNFA** nfa_array, uint64_t** p_accept_state_mapping_array) {
  if (!nfa_array) return NULL;
  uint64_t array_size = kev_get_array_size(nfa_array);
  if (array_size == 0) return NULL;
  /* Label all states */
  uint64_t start_label = 0;
  for (uint64_t i = 0; i < array_size; ++i)
    start_label = kev_nfa_state_labeling(nfa_array[i], start_label);
  uint64_t state_number = start_label;

  /* Indeces all states */
  KevGraphNode** state_mapping_array = kev_build_node_mapping_array(nfa_array, state_number);
  /* Compute closure for every single state set */
  KevBitSet* closures = kev_compute_closure_array(state_mapping_array, state_number);
  if (!state_mapping_array || !closures) {
    free(state_mapping_array);
    kev_destroy_closure_array(closures, state_number);
    return NULL;
  }

  KevSetIntMap set_index_map;
  KevSetArray state_sets;
  KevNodeArray dfa_states;
  KevIntSetMap transition_map;
  bool all_success = true;
  all_success = kev_setarray_init(&state_sets) && all_success;
  all_success = kev_nodearray_init(&dfa_states) && all_success;
  all_success = kev_setint_map_init(&set_index_map, state_number) && all_success;
  all_success = kev_intset_map_init(&transition_map, 48) && all_success;
  if (!all_success) {
    free(state_mapping_array);
    kev_destroy_closure_array(closures, state_number);
    kev_setarray_destroy(&state_sets);
    kev_nodearray_destroy(&dfa_states);
    kev_intset_map_destroy(&transition_map);
    kev_setint_map_destroy(&set_index_map);
    return NULL;
  }

  /* get and insert the start states set */
  KevBitSet* start_state_set = kev_compute_start_state_set(nfa_array, closures, state_number);
  if (!start_state_set ||
      !kev_state_sets_push_back(&set_index_map, &state_sets, &dfa_states, start_state_set)) {
    free(state_mapping_array);
    kev_destroy_closure_array(closures, state_number);
    kev_destroy_state_sets(&set_index_map, &state_sets, &dfa_states);
    kev_bitset_delete(start_state_set);
    kev_intset_map_destroy(&transition_map);
    return NULL;
  }
  
  /* subset construction algorithm */
  for (uint64_t i = 0; i < kev_setarray_size(&state_sets); ++i) {
    KevBitSet* state_set = kev_setarray_visit(&state_sets, i);
    kev_intset_map_make_empty(&transition_map);
    if (!kev_compute_all_transition(&transition_map, state_set, closures, state_mapping_array) ||
        !kev_update_transition_and_state_set(&transition_map, &set_index_map, &state_sets, &dfa_states, kev_nodearray_visit(&dfa_states, i))) {
      free(state_mapping_array);
      kev_destroy_closure_array(closures, state_number);
      kev_destroy_state_sets(&set_index_map, &state_sets, &dfa_states);
      kev_destroy_transition_map(&transition_map);
      return NULL;
    }
  }

  /* Free unused memory */
  free(state_mapping_array);
  kev_destroy_closure_array(closures, state_number);
  kev_destroy_transition_map(&transition_map);

  KevBitSet* accept_state_set = kev_compute_accept_state_set(nfa_array, state_number);
  uint64_t* ownership = NULL;
  KevDFA* dfa = NULL;
  uint64_t dfa_accept_state_number = 0;
  if (p_accept_state_mapping_array) {
    if (!(ownership = kev_get_state_ownership_array(nfa_array, state_number))) {
      kev_destroy_state_sets(&set_index_map, &state_sets, &dfa_states);
      kev_bitset_delete(accept_state_set);
      return NULL;
    }
  }
  if (!accept_state_set ||
      !(dfa = kev_construct_dfa(&dfa_states, &state_sets, accept_state_set, ownership, &dfa_accept_state_number))) {
    kev_destroy_state_sets(&set_index_map, &state_sets, &dfa_states);
    kev_bitset_delete(accept_state_set);
    free(ownership);
    return NULL;
  }
  /* Free unused memory */
  kev_bitset_delete(accept_state_set);
  free(ownership);
  kev_setint_map_destroy(&set_index_map);
  kev_setarray_destroy(&state_sets);
  kev_nodearray_destroy(&dfa_states);

  if (p_accept_state_mapping_array) {
    uint64_t* accept_state_mapping_array = (uint64_t*)malloc(sizeof (uint64_t) * dfa_accept_state_number);
    if (!accept_state_mapping_array) {
      kev_dfa_delete(dfa);
      return NULL;
    }
    KevGraphNode* accept_node = kev_dfa_get_accept_states(dfa);
    for (uint64_t i = 0; i < dfa_accept_state_number; ++i) {
      accept_state_mapping_array[i] = accept_node->attr;
      accept_node = accept_node->next;
    }
    *p_accept_state_mapping_array = accept_state_mapping_array;
  }
  return dfa;
}
