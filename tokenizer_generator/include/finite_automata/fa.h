#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_NFA_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_NFA_H

#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/general/int_type.h"
#include <stdint.h>

#define KEV_NFA_SYMBOL_EPSILON        (-1)
#define KEV_NFA_SYMBOL_EMPTY          (-2)


typedef struct tagKevFA {
  KevGraph transition;
  KevGraphNode* start_state;
  KevGraphNodeList* accept_states;
} KevFA;


bool kev_fa_init(KevFA* nfa, int64_t symbol);
bool kev_fa_init_copy(KevFA* nfa, KevFA* src);
bool kev_fa_init_set(KevFA* nfa, KevGraphNodeList* state_list, KevGraphNode* start, KevGraphNode* accept);
void kev_fa_destroy(KevFA* nfa);
KevFA* kev_fa_create(int64_t symbol);
KevFA* kev_fa_create_copy(KevFA* src);
KevFA* kev_fa_create_set(KevGraphNodeList* state_list, KevGraphNode* start, KevGraphNode* accept);
void kev_fa_delete(KevFA* nfa);

static inline bool kev_fa_add_symbol(KevFA* nfa, int64_t symbol);
uint64_t kev_fa_state_labeling(KevFA* nfa, uint64_t start_number);

/* Do concatenation between 'dest' and 'src'. 
 * The result is in 'dest', 'src' would be set to empty */
bool kev_nfa_concatenation(KevFA* dest, KevFA* src);
bool kev_nfa_alternation(KevFA* dest, KevFA* src);
static inline bool kev_nfa_kleene(KevFA* nfa);
bool kev_nfa_positive(KevFA* nfa);
/* Given an array of pointers to NFAs, use the subset construction algorithm
 * to convert the union of these NFAs into a DFA and return a pointer to the
 * resulting DFA. The array of NFA pointers must be terminated with a NULL pointer.
 * If the "p_accept_state_mapping_array" parameter is not NULL, it will point
 * to an array that records which NFA each accepting state in the DFA corresponds to.
 * For example, if this array is named "acc_state_mapping" and acc_state_mapping[2] = 0,
 * it means that the third accepting state in the DFA corresponds to the first
 * NFA in the input array of NFA pointers. */
KevFA* kev_nfa_to_dfa(KevFA** nfa_array, uint64_t** p_accept_state_mapping_array);

bool kev_dfa_minimization(KevFA* dfa, uint64_t* pre_partition);


/*begin the inline function definition */

static inline bool kev_fa_add_symbol(KevFA* nfa, int64_t symbol) {
  if (symbol == KEV_NFA_SYMBOL_EMPTY) return true;
  return kev_graphnode_connect(nfa->start_state, nfa->accept_states, symbol);
}

static inline bool kev_nfa_kleene(KevFA* nfa) {
  return kev_nfa_positive(nfa) &&
         kev_graphnode_connect(nfa->start_state, nfa->accept_states, KEV_NFA_SYMBOL_EPSILON);
}

static inline KevGraphNode* kev_fa_get_start_state(KevFA* nfa) {
  return nfa ? nfa->start_state : NULL;
}
static inline KevGraphNode* kev_fa_get_accept_state(KevFA* nfa) {
  return nfa ? nfa->accept_states : NULL;
}

#endif
