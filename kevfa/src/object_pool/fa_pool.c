#include "kevfa/include/object_pool/fa_pool.h"

#include <stdlib.h>

union KevFAPool {
  KevFA fa;
  union KevFAPool* next;
};

static union KevFAPool* fa_pool = NULL;

static inline KevFA* kev_fa_pool_acquire(void);


static inline KevFA* kev_fa_pool_acquire(void) {
  return (KevFA*)malloc(sizeof (union KevFAPool));
}

KevFA* kev_fa_pool_allocate(void) {
  if (fa_pool) {
    KevFA* retval = &fa_pool->fa;
    fa_pool = fa_pool->next;
    return retval;
  } else {
    return kev_fa_pool_acquire();
  }
}

void kev_fa_pool_deallocate(KevFA* fa) {
  if (fa) {
    union KevFAPool* freed_node = (union KevFAPool*)fa;
    freed_node->next = fa_pool;
    fa_pool = freed_node;
  }
}

void kev_fa_pool_free(void) {
  union KevFAPool* pool = fa_pool;
  fa_pool = NULL;
  while (pool) {
    union KevFAPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
