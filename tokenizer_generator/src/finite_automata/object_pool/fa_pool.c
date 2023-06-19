#include "tokenizer_generator/include/finite_automata/object_pool/fa_pool.h"

#include <stdlib.h>

union KevNFAPool {
  KevFA nfa;
  union KevNFAPool* next;
};

static union KevNFAPool* nfa_pool = NULL;

inline KevFA* kev_fa_pool_acquire(void) {
  return (KevFA*)malloc(sizeof (union KevNFAPool));
}

inline KevFA* kev_fa_pool_allocate(void) {
  if (nfa_pool) {
    KevFA* retval = &nfa_pool->nfa;
    nfa_pool = nfa_pool->next;
    return retval;
  } else {
    return kev_fa_pool_acquire();
  }
}

inline void kev_fa_pool_deallocate(KevFA* nfa) {
  if (nfa) {
    union KevNFAPool* freed_node = (union KevNFAPool*)nfa;
    freed_node->next = nfa_pool;
    nfa_pool = freed_node;
  }
}

void kev_fa_pool_free(void) {
  union KevNFAPool* pool = nfa_pool;
  nfa_pool = NULL;
  while (pool) {
    union KevNFAPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
