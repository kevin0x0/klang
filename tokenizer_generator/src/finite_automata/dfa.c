#include "tokenizer_generator/include/finite_automata/dfa.h"

#include <stdlib.h>

static inline KevDFA* kev_dfa_pool_allocate(void) {
  return (KevDFA*)malloc(sizeof (KevDFA));
}

static inline void kev_dfa_pool_deallocate(KevDFA* dfa) {
  free(dfa);
}


bool kev_dfa_init(KevDFA* dfa) {
  if (!dfa) return false;
  
  dfa->start_state = NULL;
  dfa->accept_states = NULL;
  return kev_graph_init(&dfa->transition, NULL);
}

void kev_dfa_destroy(KevDFA* dfa) {
  if (dfa) {
    dfa->start_state = NULL;
    dfa->accept_states = NULL;
    kev_graph_destroy(&dfa->transition);
  }
}

KevDFA* kev_dfa_create(void) {
  KevDFA* dfa = kev_dfa_pool_allocate();
  if (!dfa) return false;

  if (!kev_dfa_init(dfa)) {
    kev_dfa_pool_deallocate(dfa);
    return NULL;
  }

  return dfa;
}

void kev_dfa_delete(KevDFA* dfa) {
  kev_dfa_destroy(dfa);
  kev_dfa_pool_deallocate(dfa);
}

bool kev_dfa_minimization(KevDFA* dfa, KevGraphNodeList** pre_partition) {

  return false;
}
