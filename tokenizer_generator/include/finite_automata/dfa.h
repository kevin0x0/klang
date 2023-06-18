#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_DFA_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_DFA_H

#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/general/int_type.h"

typedef struct tagKevDFA {
  KevGraph transition;
  KevGraphNode* start_state;
  KevGraphNodeList* accept_states;
} KevDFA;

bool kev_dfa_init(KevDFA* dfa);
void kev_dfa_destroy(KevDFA* dfa);
KevDFA* kev_dfa_create(void);
void kev_dfa_delete(KevDFA* dfa);

static inline KevGraphNodeList* kev_dfa_get_accept_states(KevDFA* dfa) {
  return dfa ? dfa->accept_states : NULL;
}

static inline KevGraphNodeList* kev_dfa_get_states(KevDFA* dfa) {
  return dfa ? kev_graph_get_nodes(&dfa->transition) : NULL;
}

/* pre_partition: An array of nodelist. Each element of the array is
 * a pointer of node list. Value NULL marks the end of the array.   
 * "pre_partition" is used to pre-group the states in the DFA. 
 * During the DFA minimization process, the states that have been
 * pre-grouped will not be merged. If this parameter is set to NULL, 
 * pre-grouping will not be performed.
 * */
bool kev_dfa_minimization(KevDFA* dfa, KevGraphNodeList** pre_partition);


#endif
