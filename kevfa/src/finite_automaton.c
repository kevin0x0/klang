#include "kevfa/include/finite_automaton.h"
#include "kevfa/include/graph.h"
#include "kevfa/include/object_pool/fa_pool.h"

#define KEV_NFA_STATE_NAME_PLACE_HOLDER   (0)

bool kev_nfa_init_empty(KevFA* fa) {
  if (!fa) return false;
  
  KevGraphNode* start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!start || !accept) {
    kev_graphnode_delete(start);
    kev_graphnode_delete(accept);
    fa->start_state = NULL;
    fa->accept_states = NULL;
    kev_graph_init(&fa->transition, NULL);
    return false;
  }

  fa->start_state = start;
  fa->accept_states = accept;
  start->next = accept;
  return kev_graph_init_set(&fa->transition, start, accept);
}

bool kev_nfa_init_epsilon(KevFA* fa) {
  if (!fa) return false;
  
  KevGraphNode* start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!start || !accept ||
      !kev_graphnode_connect_epsilon(start, accept)) {
    kev_graphnode_delete(start);
    kev_graphnode_delete(accept);
    fa->start_state = NULL;
    fa->accept_states = NULL;
    kev_graph_init(&fa->transition, NULL);
    return false;
  }

  fa->start_state = start;
  fa->accept_states = accept;
  start->next = accept;
  return kev_graph_init_set(&fa->transition, start, accept);
}

bool kev_nfa_init(KevFA* fa, KevNFAChar character) {
  if (!fa) return false;
  
  KevGraphNode* start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!start || !accept ||
      !kev_graphnode_connect(start, accept, character)) {
    kev_graphnode_delete(start);
    kev_graphnode_delete(accept);
    fa->start_state = NULL;
    fa->accept_states = NULL;
    kev_graph_init(&fa->transition, NULL);
    return false;
  }

  fa->start_state = start;
  fa->accept_states = accept;
  start->next = accept;
  return kev_graph_init_set(&fa->transition, start, accept);
}

bool kev_fa_init_copy(KevFA* fa, KevFA* src) {
  if (!fa) return false;
  if (!src || !src->start_state || !src->accept_states) {
    kev_graph_init(&fa->transition, NULL);
    fa->start_state = NULL;
    fa->accept_states = NULL;
    return false;
  }

  if (!kev_graph_init_copy(&fa->transition, &src->transition)) {
    fa->start_state = NULL;
    fa->accept_states = NULL;
    return false;
  }
  KevGraphNode* fa_node = fa->transition.head;
  KevGraphNode* src_node = src->transition.head;
  while (src_node) {
    if (src_node == src->start_state) {
      fa->start_state = fa_node;
    } else if (src_node == src->accept_states) {
      fa->accept_states = fa_node;
    }
    src_node = src_node->next;
    fa_node = fa_node->next;
  }
  return true;
}

bool kev_fa_init_move(KevFA* fa, KevFA* src) {
  if (!fa || !src) return false;
  kev_graph_init_move(&fa->transition, &src->transition);
  fa->start_state = src->start_state;
  fa->accept_states = src->accept_states;
  src->start_state = NULL;
  src->accept_states = NULL;
  return true;
}

bool kev_fa_init_set(KevFA* fa, KevGraphNodeList* state_list, KevGraphNode* start, KevGraphNode* accept) {
  if (!fa) return false;

  if (!kev_graph_init(&fa->transition, state_list)) {
    fa->start_state = NULL;
    fa->start_state = NULL;
    return true;
  }
  fa->start_state = start;
  fa->accept_states = accept;
  return true;
}

void kev_fa_destroy(KevFA* fa) {
  if (fa) {
    fa->start_state = NULL;
    fa->accept_states = NULL;
    kev_graph_destroy(&fa->transition);
  }
}

KevFA* kev_nfa_create_empty(void) {
  KevFA* fa = kev_fa_pool_allocate();
  if (!fa) return NULL;

  if (!kev_nfa_init_empty(fa)) {
    kev_fa_pool_deallocate(fa);
    return NULL;
  }
  return fa;
}

KevFA* kev_nfa_create_epsilon(void) {
  KevFA* fa = kev_fa_pool_allocate();
  if (!fa) return NULL;

  if (!kev_nfa_init_epsilon(fa)) {
    kev_fa_pool_deallocate(fa);
    return NULL;
  }
  return fa;
}

KevFA* kev_nfa_create(KevNFAChar character) {
  KevFA* fa = kev_fa_pool_allocate();
  if (!fa) return NULL;

  if (!kev_nfa_init(fa, character)) {
    kev_fa_pool_deallocate(fa);
    return NULL;
  }
  return fa;
}

KevFA* kev_fa_create_copy(KevFA* src) {
  KevFA* fa = kev_fa_pool_allocate();
  if (!fa) return NULL;

  if (!kev_fa_init_copy(fa, src)) {
    kev_fa_pool_deallocate(fa);
    return NULL;
  }
  return fa;
}

KevFA* kev_fa_create_move(KevFA* src) {
  KevFA* fa = kev_fa_pool_allocate();
  if (!fa) return NULL;

  if (!kev_fa_init_move(fa, src)) {
    kev_fa_pool_deallocate(fa);
    return NULL;
  }
  return fa;
}

KevFA* kev_fa_create_set(KevGraphNodeList* state_list, KevGraphNode* start, KevGraphNode* accept) {
  KevFA* fa = kev_fa_pool_allocate();
  if (!fa) return NULL;

  if (!kev_fa_init_set(fa, state_list, start, accept)) {
    kev_fa_pool_deallocate(fa);
    return NULL;
  }
  return fa;
}

void kev_fa_delete(KevFA* fa) {
  kev_fa_destroy(fa);
  kev_fa_pool_deallocate(fa);
}

bool kev_nfa_concatenation(KevFA* dest, KevFA* src) {
  dest->accept_states->edges = src->start_state->edges;
  dest->accept_states->epsilons = src->start_state->epsilons;
  src->start_state->edges = NULL;
  src->start_state->epsilons = NULL;
  dest->accept_states = src->accept_states;
  src->start_state = NULL;
  src->accept_states = NULL;
  /* the head state is starting state, remove starting state of 'src' */
  KevGraphNode* tmp = src->transition.head;
  src->transition.head = src->transition.head->next;
  kev_graphnode_delete(tmp);
  return kev_graph_merge(&dest->transition, &src->transition);
}

bool kev_nfa_alternation(KevFA* dest, KevFA* src) {
  KevGraphNode* new_start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* new_accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!new_start || !new_accept) {
    kev_graphnode_delete(new_start);
    kev_graphnode_delete(new_accept);
    return false;
  }

  kev_graph_merge(&dest->transition, &src->transition);
  kev_graph_insert_node(&dest->transition, new_accept);
  kev_graph_insert_node(&dest->transition, new_start);  /* start state should be at the start of the node list */
  KevGraphNode* src_start = src->start_state;
  KevGraphNode* src_accept = src->accept_states;
  src->start_state = NULL;
  src->accept_states = NULL;
  if (!kev_graphnode_connect_epsilon(new_start, dest->start_state)    ||
      !kev_graphnode_connect_epsilon(new_start, src_start)            ||
      !kev_graphnode_connect_epsilon(dest->accept_states, new_accept) ||
      !kev_graphnode_connect_epsilon(src_accept, new_accept)) {
    dest->start_state = new_start;
    dest->accept_states = new_accept;
    return false;
  }
  dest->start_state = new_start;
  dest->accept_states = new_accept;
  return true;
}

bool kev_nfa_positive(KevFA* nfa) {
  KevGraphNode* new_start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* new_accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!new_start || !new_accept) {
    kev_graphnode_delete(new_start);
    kev_graphnode_delete(new_accept);
    return false;
  }

  kev_graph_insert_node(&nfa->transition, new_accept);
  kev_graph_insert_node(&nfa->transition, new_start); /* start state should be at the start of the node list */
  if (!kev_graphnode_connect_epsilon(new_start, nfa->start_state)   ||
      !kev_graphnode_connect_epsilon(nfa->accept_states, new_accept) ||
      !kev_graphnode_connect_epsilon(nfa->accept_states, nfa->start_state)) {
    nfa->start_state = new_start;
    nfa->accept_states = new_accept;
    return false;
  }
  nfa->start_state = new_start;
  nfa->accept_states = new_accept;
  return true;
}

size_t kev_fa_state_assign_id(KevFA* fa, KevStateId start_id) {
  KevGraphNodeList* nodes = kev_graph_get_nodes(&fa->transition);
  size_t count = start_id;
  for (KevGraphNode* current_node = nodes; current_node; current_node = current_node->next) {
    current_node->id = count;
    ++count;
  }
  return count;
}
