#include "kevlr/include/object_pool/item_pool.h"

#include <stdlib.h>

union KlrItemPool {
  KlrItem kernel_item;
  union KlrItemPool* next;
};

static union KlrItemPool* item_pool = NULL;

static inline KlrItem* klr_item_pool_acquire(void);


static inline KlrItem* klr_item_pool_acquire(void) {
  return (KlrItem*)malloc(sizeof (union KlrItemPool));
}

KlrItem* klr_item_pool_allocate(void) {
  if (item_pool) {
    KlrItem* retval = &item_pool->kernel_item;
    item_pool = item_pool->next;
    return retval;
  } else {
    return klr_item_pool_acquire();
  }
}

void klr_item_pool_deallocate(KlrItem* item) {
  if (item) {
    union KlrItemPool* freed_node = (union KlrItemPool*)item;
    freed_node->next = item_pool;
    item_pool = freed_node;
  }
}

void klr_item_pool_free(void) {
  union KlrItemPool* pool = item_pool;
  item_pool = NULL;
  while (pool) {
    union KlrItemPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
