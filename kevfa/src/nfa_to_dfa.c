#include "kevfa/include/finite_automaton.h"
#include "kevfa/include/array/node_array.h"
#include "utils/include/set/bitset.h"
#include "utils/include/hashmap/intset_map.h"
#include "utils/include/hashmap/setint_map.h"
#include "utils/include/queue/int_queue.h"
#include "utils/include/array/set_array.h"

#include <stdlib.h>

/* Package some objects that works collaboratively to maintain the generated DFA states */
typedef struct tagKevStateSets {
  KevSetIntMap set_index_map;
  KevSetArray state_sets;
  KevNodeArray dfa_states;
} KevStateSets;

/* get size of NULL-terminated array */
static inline size_t kev_get_array_size(KevFA** nfa_array);
/* build mapping array between node number and address of node */
static KevGraphNode** kev_build_node_mapping_array(KevFA** nfa_array, size_t state_number);
/* compute epsilon-closure of single state set */
static bool kev_compute_state_closure(size_t state, KevGraphNode** state_mapping, KBitSet* closures, KevIntQueue* queue);
/* compute epsilon-closure for every single state set */
static KBitSet* kev_compute_closure_array(KevGraphNode** state_mapping, size_t state_number);
/* return a set containing start state of every nfa */
static inline KBitSet* kev_compute_start_state_set(KevFA** nfa_array, KBitSet* closures, size_t state_number);
/* return a set containing accept state of every nfa */
static inline KBitSet* kev_compute_accept_state_set(KevFA** nfa_array, size_t state_number);
/* insert a state set to "sets", record the index of inserted set in "map" and 
 * insert a new node corresponding to the inserted set to "dfa_states". */
static inline bool kev_state_sets_insert(KevStateSets* dfa_states_sets, KBitSet* set);
/* Compute all transitions starting from "closure", where transitions are
 * recorded in the form of a tuple <transition character,
 * set of transition states> in the "transition_map".*/
static bool kev_compute_all_transition(KevIntSetMap* transition_map, KBitSet* closure, KBitSet* closures,
                                       KevGraphNode** state_mapping);
/* Add the corresponding transitions to the DFA states based on
 * the tuples in the "transition_map". The tuples contain
 * the transition character and the set of transition states. */
static bool kev_update_transition_and_state_set(KevIntSetMap* transition_map, KevStateSets* dfa_states_sets, KevGraphNode* state);
/* The array "ownership" is used to record to which NFA a state belongs.
 * For example, if ownership[2] = 0, it means that the state with ID 2 comes
 * from the first NFA in the "nfa_array" (array indices start at 0). */
static size_t* kev_get_state_ownership_array(KevFA** nfa_array, size_t state_number);
static KevFA* kev_construct_dfa(KevNodeArray* dfa_states, KevSetArray* state_sets, KevFA** nfa_array,
                                size_t state_number, size_t** p_accept_state_mapping_array);
/* Organize the DFA state nodes in the "dfa_states" into a structure of the struct KevDFA. */
static KevFA* kev_classify_dfa_states(KevNodeArray* dfa_states, KevSetArray* state_sets, KBitSet* accept_state_set,
                                      size_t* state_ownership, size_t* p_dfa_accept_state_number);
static size_t* kev_build_accept_state_mapping_array(KevFA* dfa, size_t dfa_accept_state_number);
static bool kev_subset_construction_algorithm_init(KevFA** nfa_array, KevGraphNode** state_mapping_array, KBitSet* closures,
                                            KevStateSets* dfa_states_sets, KevIntSetMap* transition_map, size_t state_number);
static bool kev_do_subset_construct_algorithm(KevGraphNode** state_mapping_array, KBitSet* closures,
                                            KevStateSets* dfa_states_sets, KevIntSetMap* transition_map);
static size_t kev_label_all_nfa_states(KevFA** nfa_array, size_t array_size);

/* destroy all closures and free the array */
static void kev_destroy_closure_array(KBitSet* closure_array, size_t state_number);
static void kev_destroy_transition_map(KevIntSetMap* map);
/* if the delete_node is set to true, then the dfa nodes in "nodes" would be deleted. */
static inline void kev_destroy_state_sets(KevStateSets* dfa_states_sets, bool delete_node);

KevFA* kev_nfa_to_dfa(KevFA** nfa_array, size_t** p_accept_state_mapping_array) {
  size_t array_size = kev_get_array_size(nfa_array);
  if (array_size == 0) return NULL;
  /* Label all states */
  size_t state_number = kev_label_all_nfa_states(nfa_array, array_size);
  /* Indeces all states */
  KevGraphNode** state_mapping_array = kev_build_node_mapping_array(nfa_array, state_number);
  /* Compute closure for every single state set */
  KBitSet* closures = kev_compute_closure_array(state_mapping_array, state_number);
  if (!state_mapping_array || !closures) {
    free(state_mapping_array);
    kev_destroy_closure_array(closures, state_number);
    return NULL;
  }

  KevStateSets dfa_states_sets;
  KevIntSetMap transition_map;
  if (!kev_subset_construction_algorithm_init(nfa_array, state_mapping_array, closures,
                                              &dfa_states_sets, &transition_map, state_number)) {
    return NULL;
  }
  if (!kev_do_subset_construct_algorithm(state_mapping_array, closures, &dfa_states_sets, &transition_map)) {
      kev_destroy_closure_array(closures, state_number);
      kev_destroy_state_sets(&dfa_states_sets, true);
      kev_destroy_transition_map(&transition_map);
      return NULL;
  }
  free(state_mapping_array);
  kev_destroy_closure_array(closures, state_number);
  kev_destroy_transition_map(&transition_map);

  KevFA* dfa = kev_construct_dfa(&dfa_states_sets.dfa_states, &dfa_states_sets.state_sets,
                                 nfa_array, state_number, p_accept_state_mapping_array);
  if (!dfa) {
    kev_destroy_state_sets(&dfa_states_sets, true);
    return NULL;
  }
  kev_destroy_state_sets(&dfa_states_sets, false);
  return dfa;
}

static inline size_t kev_get_array_size(KevFA** nfa_array) {
  KevFA** begin = nfa_array;
  KevFA** end = nfa_array;
  while (*end) end++;
  return end - begin;
}

static KevGraphNode** kev_build_node_mapping_array(KevFA** nfa_array, size_t state_number) {
  KevGraphNode** mapping_array = (KevGraphNode**)malloc(sizeof (KevGraphNode*) * state_number);
  if (!mapping_array) return NULL;
  KevFA** pnfa = nfa_array - 1;
  while (*++pnfa != NULL) {
    KevGraphNodeList* nodelst = kev_fa_get_states(*pnfa);
    for (KevGraphNode* node = nodelst; node; node = node->next) {
      mapping_array[node->id] = node;
    }
  }
  return mapping_array;
}

static bool kev_compute_state_closure(size_t state, KevGraphNode** state_mapping, KBitSet* closures, KevIntQueue* queue) {
  KBitSet* closure = &closures[state];
  if (!kbitset_set(closure, state) || !kev_intqueue_insert(queue, state))
    return false;
  
  while (!kev_intqueue_empty(queue)) {
    size_t state = kev_intqueue_pop(queue);
    for (KevGraphEdge* epsilons = kev_graphnode_get_epsilon(state_mapping[state]); epsilons; epsilons = epsilons->next) {
      if (!kbitset_has_element(closure, epsilons->node->id)) {
        KevGraphNodeId id = epsilons->node->id;
        if (epsilons->node->id < state) {
          if (!kbitset_union(closure, closures + id))
            return false;
          continue;
        }
        if (!kbitset_set(closure, id) ||
            !kev_intqueue_insert(queue, id)) {
          return false;
        }
      }
    }
  }
  return true;
}

static KBitSet* kev_compute_closure_array(KevGraphNode** state_mapping, size_t state_number) {
  KBitSet* state_closure = (KBitSet*)malloc(sizeof (KBitSet) * state_number);
  if (!state_closure) return NULL;
  KevIntQueue queue;
  if (!kev_intqueue_init(&queue)) {
    free(state_closure);
    return NULL;
  }

  for (size_t i = 0; i < state_number; ++i) {
    if (!kbitset_init(state_closure + i, state_number) ||
        !kev_compute_state_closure(i, state_mapping, state_closure, &queue)) {
      for (size_t j = 0; j < i; ++j)
        kbitset_destroy(state_closure + j);
      free(state_closure);
      kev_intqueue_destroy(&queue);
      return NULL;
    }
  }
  kev_intqueue_destroy(&queue);
  return state_closure;
}

static void kev_destroy_closure_array(KBitSet* closure_array, size_t state_number) {
  if (!closure_array) return;
  for (size_t i = 0; i < state_number; ++i)
    kbitset_destroy(closure_array + i);
  free(closure_array);
}

static inline KBitSet* kev_compute_start_state_set(KevFA** nfa_array, KBitSet* closures, size_t state_number) {
  KBitSet* start_state_set = kbitset_create(state_number);
  if (start_state_set) {
    KevFA** pnfa = nfa_array - 1;
    while (*++pnfa)
      kbitset_union(start_state_set, &closures[kev_fa_get_start_state(*pnfa)->id]);
  }
  return start_state_set;
}

static inline KBitSet* kev_compute_accept_state_set(KevFA** nfa_array, size_t state_number) {
  KBitSet* accept_state_set = kbitset_create(state_number);
  if (accept_state_set) {
    KevFA** pnfa = nfa_array - 1;
    while (*++pnfa)
      kbitset_set(accept_state_set, kev_fa_get_accept_state(*pnfa)->id);
  }
  return accept_state_set;
}

static inline bool kev_state_sets_insert(KevStateSets* dfa_states_sets, KBitSet* set) {
  KevGraphNode* new_node = kev_graphnode_create(0);
  if (!new_node || !kev_nodearray_push_back(&dfa_states_sets->dfa_states, new_node)) {
    kev_graphnode_delete(new_node);
    return false;
  }
  return kev_setintmap_insert(&dfa_states_sets->set_index_map, set, kev_setarray_size(&dfa_states_sets->state_sets)) &&
         kev_setarray_push_back(&dfa_states_sets->state_sets, set);
}

static inline void kev_destroy_state_sets(KevStateSets* dfa_states_sets, bool delete_node) {
  size_t size = kev_setarray_size(&dfa_states_sets->state_sets);
  for (size_t i = 0; i < size; ++i)
    kbitset_delete(kev_setarray_visit(&dfa_states_sets->state_sets, i));
  kev_setarray_destroy(&dfa_states_sets->state_sets);
  kev_setintmap_destroy(&dfa_states_sets->set_index_map);
  if (delete_node) {
    for (size_t i = 0; i < size; ++i) {
      kev_graphnode_delete(kev_nodearray_visit(&dfa_states_sets->dfa_states, i));
    }
  }
  kev_nodearray_destroy(&dfa_states_sets->dfa_states);
}

static bool kev_compute_all_transition(KevIntSetMap* transition_map, KBitSet* closure,
                                       KBitSet* closures, KevGraphNode** state_mapping) {
  /* It is guaranteed that the closure is not empty. */
  size_t state = 0;
  size_t next_state = kbitset_iter_begin(closure);
  do {
    state = next_state;
    KevGraphEdge* edge = kev_graphnode_get_edges(state_mapping[state]);
    for (; edge; edge = edge->next) {
      KevIntSetMapNode* map_node = kev_intsetmap_search(transition_map, edge->attr);
      KBitSet* set = NULL;
      if (map_node == NULL) {
        if (!(set = kbitset_create_copy(&closures[edge->node->id])) ||
            !kev_intsetmap_insert(transition_map, edge->attr, set)) {
          kbitset_delete(set);
          return false;
        }
      } else {
        /* This never fails. Because the capacity of every bitset is initialized
         * to state number, so overflow will never happen. */
        kbitset_union(map_node->value, &closures[edge->node->id]);
      }
    }
    next_state = kbitset_iter_next(closure, state);
  } while (next_state != state);
  return true;
}

static void kev_destroy_transition_map(KevIntSetMap* map) {
  for (KevIntSetMapNode* itr = kev_intsetmap_iterate_begin(map);
      itr != NULL;
      itr = kev_intsetmap_iterate_next(map, itr)) {
    kbitset_delete(itr->value);
  }
  kev_intsetmap_destroy(map);
}

static bool kev_update_transition_and_state_set(KevIntSetMap* transition_map, KevStateSets* dfa_states_sets, KevGraphNode* state) {
  for (KevIntSetMapNode* itr = kev_intsetmap_iterate_begin(transition_map);
      itr != NULL;
      itr = kev_intsetmap_iterate_next(transition_map, itr)) {
    KevSetIntMapNode* target_node = kev_setintmap_search(&dfa_states_sets->set_index_map, itr->value);
    if (target_node == NULL) {
      size_t target_state_no = kev_nodearray_size(&dfa_states_sets->dfa_states);
      if (!kev_state_sets_insert(dfa_states_sets, itr->value)) {
        return false;
      }
      itr->value = NULL;
      if (!kev_graphnode_connect(state, kev_nodearray_visit(&dfa_states_sets->dfa_states, target_state_no), itr->key)) {
        return false;
      }
    } else {
      kbitset_delete(itr->value);
      itr->value = NULL;
      if (!kev_graphnode_connect(state, kev_nodearray_visit(&dfa_states_sets->dfa_states, target_node->value), itr->key)) {
        return false;
      }
    }
  }
  return true;
}

static size_t* kev_get_state_ownership_array(KevFA** nfa_array, size_t state_number) {
  size_t* ownership = (size_t*)malloc(sizeof (size_t) * state_number);
  if (ownership) {
    KevFA** pnfa = nfa_array;
    size_t count = 0;
    while (*pnfa) {
      for (KevGraphNode* node = kev_fa_get_states(*pnfa); node; node = node->next) {
        ownership[node->id] = count;
      }
      count++; pnfa++;
    }
  }
  return ownership;
}

static KevFA* kev_construct_dfa(KevNodeArray* dfa_states, KevSetArray* state_sets, KevFA** nfa_array, size_t state_number, size_t** p_accept_state_mapping_array) {
  size_t* ownership = NULL;
  size_t dfa_accept_state_number = 0;
  if (p_accept_state_mapping_array) {
    if (!(ownership = kev_get_state_ownership_array(nfa_array, state_number))) {
      return NULL;
    }
  }
  KBitSet* accept_state_set = kev_compute_accept_state_set(nfa_array, state_number);
  if (!accept_state_set) {
    free(ownership);
    return NULL;
  }
  KevFA* dfa = kev_classify_dfa_states(dfa_states, state_sets, accept_state_set, ownership, &dfa_accept_state_number);
  if (!dfa) {
    free(ownership);
    kbitset_delete(accept_state_set);
    return NULL;
  }
  free(ownership);
  kbitset_delete(accept_state_set);
  if (p_accept_state_mapping_array) {
    *p_accept_state_mapping_array = kev_build_accept_state_mapping_array(dfa, dfa_accept_state_number);
    if (!*p_accept_state_mapping_array) {
      kev_fa_delete(dfa);
      return NULL;
    }
  }
  return dfa;
}

bool kev_subset_construction_algorithm_init(KevFA** nfa_array, KevGraphNode** state_mapping_array, KBitSet* closures,
                                            KevStateSets* dfa_states_sets, KevIntSetMap* transition_map, size_t state_number) {
  bool all_done = true;
  all_done = kev_setarray_init(&dfa_states_sets->state_sets) && all_done;
  all_done = kev_nodearray_init(&dfa_states_sets->dfa_states) && all_done;
  all_done = kev_setintmap_init(&dfa_states_sets->set_index_map, state_number) && all_done;
  all_done = kev_intsetmap_init(transition_map, 48) && all_done;
  if (!all_done) {
    free(state_mapping_array);
    kev_destroy_closure_array(closures, state_number);
    kev_setarray_destroy(&dfa_states_sets->state_sets);
    kev_nodearray_destroy(&dfa_states_sets->dfa_states);
    kev_intsetmap_destroy(transition_map);
    kev_setintmap_destroy(&dfa_states_sets->set_index_map);
    return false;
  }

  /* get and insert the start states set */
  KBitSet* start_state_set = kev_compute_start_state_set(nfa_array, closures, state_number);
  if (!start_state_set ||
      !kev_state_sets_insert(dfa_states_sets, start_state_set)) {
    free(state_mapping_array);
    kev_destroy_closure_array(closures, state_number);
    kev_destroy_state_sets(dfa_states_sets, true);
    kbitset_delete(start_state_set);
    kev_intsetmap_destroy(transition_map);
    return false;
  }

  return true;
}

bool kev_do_subset_construct_algorithm(KevGraphNode** state_mapping_array, KBitSet* closures,
                                            KevStateSets* dfa_states_sets, KevIntSetMap* transition_map) {
  for (size_t i = 0; i < kev_setarray_size(&dfa_states_sets->state_sets); ++i) {
    KBitSet* state_set = kev_setarray_visit(&dfa_states_sets->state_sets, i);
    kev_intsetmap_make_empty(transition_map);
    if (!kev_compute_all_transition(transition_map, state_set, closures, state_mapping_array) ||
        !kev_update_transition_and_state_set(transition_map, dfa_states_sets, kev_nodearray_visit(&dfa_states_sets->dfa_states, i))) {
      return false;
    }
  }
  return true;
}

static size_t* kev_build_accept_state_mapping_array(KevFA* dfa, size_t dfa_accept_state_number) {
  size_t* accept_state_mapping_array = (size_t*)malloc(sizeof (size_t) * dfa_accept_state_number);
  if (!accept_state_mapping_array) {
    return NULL;
  }
  KevGraphNode* accept_node = kev_fa_get_accept_state(dfa);
  for (size_t i = 0; i < dfa_accept_state_number; ++i) {
    accept_state_mapping_array[i] = accept_node->id;
    accept_node = accept_node->next;
  }
  return accept_state_mapping_array;
}

static size_t kev_label_all_nfa_states(KevFA** nfa_array, size_t array_size) {
  size_t start_label = 0;
  for (size_t i = 0; i < array_size; ++i)
    start_label = kev_fa_state_assign_id(nfa_array[i], start_label);
  return start_label;
}

static KevFA* kev_classify_dfa_states(KevNodeArray* dfa_states, KevSetArray* state_sets, KBitSet* accept_state_set,
                                      size_t* state_ownership, size_t* p_dfa_accept_state_number) {
  KevGraphNode* accept_states = NULL;
  KevGraphNode* normal_state_tail = NULL;
  KevGraphNode* normal_states = NULL;
  size_t accept_state_number = 0;
  size_t size = kev_setarray_size(state_sets);
  for (size_t i = 0; i < size; ++i) {
    KevGraphNode* dfa_node = kev_nodearray_visit(dfa_states, i);
    KBitSet* state_set = kev_setarray_visit(state_sets, i);
    kbitset_intersection(state_set, accept_state_set);
    if (!kbitset_empty(state_set)) {
      size_t min_accept_state = kbitset_iter_begin(state_set);
      if (state_ownership)
        dfa_node->id = state_ownership[min_accept_state];
      dfa_node->next = accept_states;
      accept_states = dfa_node;
      accept_state_number++;
    } else {
      dfa_node->next = normal_states;
      if (!normal_states) normal_state_tail = dfa_node;
      normal_states = dfa_node;
    }
  }
  if (normal_state_tail) {
    normal_state_tail->next = accept_states;
  } else {
    normal_states = accept_states;
  }
  KevFA* dfa = kev_fa_create_set(normal_states, kev_nodearray_visit(dfa_states, 0), accept_states);
  *p_dfa_accept_state_number = accept_state_number;
  return dfa;
}
