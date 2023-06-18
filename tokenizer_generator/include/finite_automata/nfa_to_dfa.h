#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_NFA_TO_DFA_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_NFA_TO_DFA_H
#include "tokenizer_generator/include/finite_automata/dfa.h"
#include "tokenizer_generator/include/finite_automata/nfa.h"
#include "tokenizer_generator/include/general/int_type.h"


KevDFA* kev_nfa_to_dfa(KevNFA** nfa_array, uint64_t** p_accept_state_mapping_array);

#endif
