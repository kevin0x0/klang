#include "kevlr/include/object_pool/itemsettrans_pool.h"

#include <stdlib.h>

union KlrItemSetGotoPool {
  KlrItemSetTransition itemsettrans;
  union KlrItemSetGotoPool* next;
};

static union KlrItemSetGotoPool* itemsettrans_pool = NULL;

static inline KlrItemSetTransition* kev_itemsettrans_pool_acquire(void);


static inline KlrItemSetTransition* kev_itemsettrans_pool_acquire(void) {
  return (KlrItemSetTransition*)malloc(sizeof (union KlrItemSetGotoPool));
}

KlrItemSetTransition* klr_itemsettrans_pool_allocate(void) {
  if (itemsettrans_pool) {
    KlrItemSetTransition* retval = &itemsettrans_pool->itemsettrans;
    itemsettrans_pool = itemsettrans_pool->next;
    return retval;
  } else {
    return kev_itemsettrans_pool_acquire();
  }
}

void klr_itemsettrans_pool_deallocate(KlrItemSetTransition* itemsettrans) {
  if (itemsettrans) {
    union KlrItemSetGotoPool* freed_node = (union KlrItemSetGotoPool*)itemsettrans;
    freed_node->next = itemsettrans_pool;
    itemsettrans_pool = freed_node;
  }
}

void klr_itemsettrans_pool_free(void) {
  union KlrItemSetGotoPool* pool = itemsettrans_pool;
  itemsettrans_pool = NULL;
  while (pool) {
    union KlrItemSetGotoPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
