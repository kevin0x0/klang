#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_NFA_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_NFA_H

#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/general/int_type.h"
#include <stdint.h>

#define KEV_NFA_SYMBOL_EPSILON        (-1)
#define KEV_NFA_SYMBOL_EMPTY          (-2)


typedef struct tagKevNFA {
  KevGraph transition;
  KevGraphNode* start_state;
  KevGraphNode* accept_state;
} KevNFA;


bool kev_nfa_init(KevNFA* nfa, int64_t symbol);
bool kev_nfa_init_copy(KevNFA* nfa, KevNFA* from);
void kev_nfa_destroy(KevNFA* nfa);
KevNFA* kev_nfa_create(int64_t symbol);
void kev_nfa_delete(KevNFA* nfa);

static inline bool kev_nfa_add_symbol(KevNFA* nfa, int64_t symbol);

/* Do concatenation between 'to' and 'from'. 
 * The result is in 'to', 'from' would be deserted */
bool kev_nfa_concatenation(KevNFA* dest, KevNFA* src);
bool kev_nfa_alternation(KevNFA* dest, KevNFA* src);
static inline bool kev_nfa_kleene(KevNFA* nfa);
bool kev_nfa_positive(KevNFA* nfa);
uint64_t kev_nfa_state_labeling(KevNFA* nfa, uint64_t start_number);


static inline bool kev_nfa_add_symbol(KevNFA* nfa, int64_t symbol) {
  if (!nfa || !nfa->start_state || !nfa->accept_state) return false;
  if (symbol == KEV_NFA_SYMBOL_EMPTY) return true;
  return kev_graphnode_connect(nfa->start_state, nfa->accept_state, symbol);
}

static inline bool kev_nfa_kleene(KevNFA* nfa) {
  return kev_nfa_positive(nfa) &&
         kev_graphnode_connect(nfa->start_state, nfa->accept_state, KEV_NFA_SYMBOL_EPSILON);
}

static inline KevGraphNode* kev_nfa_get_start_state(KevNFA* nfa) {
  return nfa ? nfa->start_state : NULL;
}
static inline KevGraphNode* kev_nfa_get_accept_state(KevNFA* nfa) {
  return nfa ? nfa->accept_state : NULL;
}

#endif
