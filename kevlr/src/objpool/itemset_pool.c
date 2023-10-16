#include "kevlr/include/objpool/itemset_pool.h"

#include <stdlib.h>

union KlrItemSetPool {
  KlrItemSet itemset;
  union KlrItemSetPool* next;
};

static union KlrItemSetPool* itemset_pool = NULL;

static inline KlrItemSet* klr_itemset_pool_acquire(void);


static inline KlrItemSet* klr_itemset_pool_acquire(void) {
  return (KlrItemSet*)malloc(sizeof (union KlrItemSetPool));
}

KlrItemSet* klr_itemset_pool_allocate(void) {
  if (itemset_pool) {
    KlrItemSet* retval = &itemset_pool->itemset;
    itemset_pool = itemset_pool->next;
    return retval;
  } else {
    return klr_itemset_pool_acquire();
  }
}

void klr_itemset_pool_deallocate(KlrItemSet* itemset) {
  if (itemset) {
    union KlrItemSetPool* freed_node = (union KlrItemSetPool*)itemset;
    freed_node->next = itemset_pool;
    itemset_pool = freed_node;
  }
}

void klr_itemset_pool_free(void) {
  union KlrItemSetPool* pool = itemset_pool;
  itemset_pool = NULL;
  while (pool) {
    union KlrItemSetPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
