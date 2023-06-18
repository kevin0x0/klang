#include "tokenizer_generator/include/finite_automata/object_pool/nfa_pool.h"

#include <stdlib.h>

union KevNFAPool {
  KevNFA nfa;
  union KevNFAPool* next;
};

static union KevNFAPool* nfa_pool = NULL;

inline KevNFA* kev_nfa_pool_acquire(void) {
  return (KevNFA*)malloc(sizeof (union KevNFAPool));
}

inline KevNFA* kev_nfa_pool_allocate(void) {
  if (nfa_pool) {
    KevNFA* retval = &nfa_pool->nfa;
    nfa_pool = nfa_pool->next;
    return retval;
  } else {
    return kev_nfa_pool_acquire();
  }
}

inline void kev_nfa_pool_deallocate(KevNFA* nfa) {
  if (nfa) {
    union KevNFAPool* freed_node = (union KevNFAPool*)nfa;
    freed_node->next = nfa_pool;
    nfa_pool = freed_node;
  }
}

void kev_nfa_pool_free(void) {
  union KevNFAPool* pool = nfa_pool;
  nfa_pool = NULL;
  while (pool) {
    union KevNFAPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
