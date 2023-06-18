#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_NFA_POOL_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_OBJECT_POOL_NFA_POOL_H
#include "tokenizer_generator/include/finite_automata/nfa.h"

KevNFA* kev_nfa_pool_acquire(void);
KevNFA* kev_nfa_pool_allocate(void);
void kev_nfa_pool_deallocate(KevNFA* nfa);
void kev_nfa_pool_free(void);

#endif
