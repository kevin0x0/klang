#include "pargen/include/lr/object_pool/itemset_pool.h"

#include <stdlib.h>

union KevItemSetPool {
  KevItemSet itemset;
  union KevItemSetPool* next;
};

static union KevItemSetPool* itemset_pool = NULL;

inline KevItemSet* kev_itemset_pool_acquire(void) {
  return (KevItemSet*)malloc(sizeof (union KevItemSetPool));
}

inline KevItemSet* kev_itemset_pool_allocate(void) {
  if (itemset_pool) {
    KevItemSet* retval = &itemset_pool->itemset;
    itemset_pool = itemset_pool->next;
    return retval;
  } else {
    return kev_itemset_pool_acquire();
  }
}

inline void kev_itemset_pool_deallocate(KevItemSet* itemset) {
  if (itemset) {
    union KevItemSetPool* freed_node = (union KevItemSetPool*)itemset;
    freed_node->next = itemset_pool;
    itemset_pool = freed_node;
  }
}

void kev_itemset_pool_free(void) {
  union KevItemSetPool* pool = itemset_pool;
  itemset_pool = NULL;
  while (pool) {
    union KevItemSetPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
