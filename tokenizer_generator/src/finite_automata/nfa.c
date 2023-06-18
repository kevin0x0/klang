#include "tokenizer_generator/include/finite_automata/nfa.h"
#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/finite_automata/object_pool/nfa_pool.h"
#include <stdint.h>


#define KEV_NFA_STATE_NAME_PLACE_HOLDER   (0)
#define KEV_NFA_STATE_NAME_START_MARK     (-1)
#define KEV_NFA_STATE_NAME_ACCEPT_MARK    (-2)


bool kev_nfa_init(KevNFA* nfa, int64_t symbol) {
  if (!nfa) return false;
  
  KevGraphNode* start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!start || !accept ||
      !(symbol == KEV_NFA_SYMBOL_EMPTY || kev_graphnode_connect(start, accept, symbol))) {
    kev_graphnode_delete(start);
    kev_graphnode_delete(accept);
    nfa->start_state = NULL;
    nfa->accept_state = NULL;
    kev_graph_init(&nfa->transition, NULL);
    return false;
  }

  nfa->start_state = start;
  nfa->accept_state = accept;
  start = kev_graphnode_list_insert(start, accept);
  return kev_graph_init(&nfa->transition, start);
}

bool kev_nfa_init_copy(KevNFA* nfa, KevNFA* src) {
  if (!nfa) return false;
  if (!src || !src->start_state || !src->accept_state) {
    kev_graph_init(&nfa->transition, NULL);
    nfa->start_state = NULL;
    nfa->accept_state = NULL;
    return false;
  }

  int64_t original_start_symbol = src->start_state->attr;
  int64_t original_accept_symbol = src->accept_state->attr;
  src->start_state->attr = KEV_NFA_STATE_NAME_START_MARK;
  src->accept_state->attr = KEV_NFA_STATE_NAME_ACCEPT_MARK;
  if (!kev_graph_init_copy(&nfa->transition, &src->transition)) {
    nfa->start_state = NULL;
    nfa->accept_state = NULL;
    src->start_state->attr = original_start_symbol;
    src->accept_state->attr = original_accept_symbol;
    return false;
  }
  src->start_state->attr = original_start_symbol;
  src->accept_state->attr = original_accept_symbol;

  int count = 0;
  KevGraphNode* node = nfa->transition.head;
  while (count != 2) {
    if (node->attr == KEV_NFA_STATE_NAME_START_MARK) {
      src->start_state = node;
      ++count;
    } else if (node->attr == KEV_NFA_STATE_NAME_ACCEPT_MARK) {
      src->accept_state = node;
      ++count;
    }
    node = node->next;
  }
  src->start_state->attr = KEV_NFA_STATE_NAME_PLACE_HOLDER;
  src->accept_state->attr = KEV_NFA_STATE_NAME_PLACE_HOLDER;
  return true;
}

void kev_nfa_destroy(KevNFA* nfa) {
  if (nfa) {
    nfa->start_state = NULL;
    nfa->accept_state = NULL;
    kev_graph_destroy(&nfa->transition);
  }
}

KevNFA* kev_nfa_create(int64_t symbol) {
  KevNFA* nfa = kev_nfa_pool_allocate();
  if (!nfa) return false;

  if (!kev_nfa_init(nfa, symbol)) {
    kev_nfa_pool_deallocate(nfa);
    return NULL;
  }

  return nfa;
}

void kev_nfa_delete(KevNFA* nfa) {
  kev_nfa_destroy(nfa);
  kev_nfa_pool_deallocate(nfa);
}

bool kev_nfa_concatenation(KevNFA* dest, KevNFA* src) {
  if (!dest || !src) return false;

  kev_graph_merge(&dest->transition, &src->transition);
  KevGraphNode* src_start = src->start_state;
  KevGraphNode* src_accept = src->accept_state;
  src->start_state = NULL;
  src->accept_state = NULL;
  KevGraphNode* dest_accept = dest->accept_state;
  dest->accept_state = src_accept;
  return kev_graphnode_connect(dest_accept, src_start, KEV_NFA_SYMBOL_EPSILON);
}

bool kev_nfa_alternation(KevNFA* dest, KevNFA* src) {
  if (!dest || !src) return false;
  KevGraphNode* new_start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* new_accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!new_start || !new_accept) {
    kev_graphnode_delete(new_start);
    kev_graphnode_delete(new_accept);
    return false;
  }

  kev_graph_merge(&dest->transition, &src->transition);
  kev_graph_add_node(&dest->transition, new_start);
  kev_graph_add_node(&dest->transition, new_accept);
  KevGraphNode* src_start = src->start_state;
  KevGraphNode* src_accept = src->accept_state;
  src->start_state = NULL;
  src->accept_state = NULL;
  if (!kev_graphnode_connect(new_start, dest->start_state, KEV_NFA_SYMBOL_EPSILON)   ||
      !kev_graphnode_connect(new_start, src_start, KEV_NFA_SYMBOL_EPSILON)    ||
      !kev_graphnode_connect(dest->accept_state, new_accept, KEV_NFA_SYMBOL_EPSILON) ||
      !kev_graphnode_connect(src_accept, new_accept, KEV_NFA_SYMBOL_EPSILON)) {
    dest->start_state = new_start;
    dest->accept_state = new_accept;
    return false;
  }
  dest->start_state = new_start;
  dest->accept_state = new_accept;
  return true;
}

bool kev_nfa_positive(KevNFA* nfa) {
  KevGraphNode* new_start = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  KevGraphNode* new_accept = kev_graphnode_create(KEV_NFA_STATE_NAME_PLACE_HOLDER);
  if (!new_start || !new_accept) {
    kev_graphnode_delete(new_start);
    kev_graphnode_delete(new_accept);
    return false;
  }
  
  kev_graph_add_node(&nfa->transition, new_start);
  kev_graph_add_node(&nfa->transition, new_accept);
  if (!kev_graphnode_connect(new_start, nfa->start_state, KEV_NFA_SYMBOL_EPSILON)   ||
      !kev_graphnode_connect(nfa->accept_state, new_accept, KEV_NFA_SYMBOL_EPSILON) ||
      !kev_graphnode_connect(nfa->accept_state, nfa->start_state, KEV_NFA_SYMBOL_EPSILON)) {
    nfa->start_state = new_start;
    nfa->accept_state = new_accept;
    return false;
  }
  nfa->start_state = new_start;
  nfa->accept_state = new_accept;
  return true;
}

uint64_t kev_nfa_state_labeling(KevNFA* nfa, uint64_t start_number) {
  if (!nfa) return 0;

  KevGraphNodeList* nodes = kev_graph_get_nodes(&nfa->transition);
  uint64_t count = start_number;
  KevGraphNode* current_node = nodes;
  while (current_node) {
    current_node->attr = count;
    current_node = current_node->next;
    ++count;
  }
  return count;
}
