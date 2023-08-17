#include "pargen/include/lr/object_pool/kernel_item_pool.h"

#include <stdlib.h>

union KevItemPool {
  KevItem kernel_item;
  union KevItemPool* next;
};

static union KevItemPool* item_pool = NULL;

inline KevItem* kev_item_pool_acquire(void) {
  return (KevItem*)malloc(sizeof (union KevItemPool));
}

inline KevItem* kev_item_pool_allocate(void) {
  if (item_pool) {
    KevItem* retval = &item_pool->kernel_item;
    item_pool = item_pool->next;
    return retval;
  } else {
    return kev_item_pool_acquire();
  }
}

inline void kev_item_pool_deallocate(KevItem* item) {
  if (item) {
    union KevItemPool* freed_node = (union KevItemPool*)item;
    freed_node->next = item_pool;
    item_pool = freed_node;
  }
}

void kev_item_pool_free(void) {
  union KevItemPool* pool = item_pool;
  item_pool = NULL;
  while (pool) {
    union KevItemPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
