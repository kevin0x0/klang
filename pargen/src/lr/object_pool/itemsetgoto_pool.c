#include "pargen/include/lr/object_pool/itemsetgoto_pool.h"

#include <stdlib.h>

union KevItemSetGotoPool {
  KevItemSetGoto itemsetgoto;
  union KevItemSetGotoPool* next;
};

static union KevItemSetGotoPool* itemsetgoto_pool = NULL;

inline KevItemSetGoto* kev_itemsetgoto_pool_acquire(void) {
  return (KevItemSetGoto*)malloc(sizeof (union KevItemSetGotoPool));
}

inline KevItemSetGoto* kev_itemsetgoto_pool_allocate(void) {
  if (itemsetgoto_pool) {
    KevItemSetGoto* retval = &itemsetgoto_pool->itemsetgoto;
    itemsetgoto_pool = itemsetgoto_pool->next;
    return retval;
  } else {
    return kev_itemsetgoto_pool_acquire();
  }
}

inline void kev_itemsetgoto_pool_deallocate(KevItemSetGoto* itemsetgoto) {
  if (itemsetgoto) {
    union KevItemSetGotoPool* freed_node = (union KevItemSetGotoPool*)itemsetgoto;
    freed_node->next = itemsetgoto_pool;
    itemsetgoto_pool = freed_node;
  }
}

void kev_itemsetgoto_pool_free(void) {
  union KevItemSetGotoPool* pool = itemsetgoto_pool;
  itemsetgoto_pool = NULL;
  while (pool) {
    union KevItemSetGotoPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
