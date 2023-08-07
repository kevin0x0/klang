#include "pargen/include/lr/object_pool/kernel_item_pool.h"

#include <stdlib.h>

union KevKernelItemPool {
  KevKernelItem kernel_item;
  union KevKernelItemPool* next;
};

static union KevKernelItemPool* kernel_item_pool = NULL;

inline KevKernelItem* kev_kernel_item_pool_acquire(void) {
  return (KevKernelItem*)malloc(sizeof (union KevKernelItemPool));
}

inline KevKernelItem* kev_kernel_item_pool_allocate(void) {
  if (kernel_item_pool) {
    KevKernelItem* retval = &kernel_item_pool->kernel_item;
    kernel_item_pool = kernel_item_pool->next;
    return retval;
  } else {
    return kev_kernel_item_pool_acquire();
  }
}

inline void kev_kernel_item_pool_deallocate(KevKernelItem* kernel_item) {
  if (kernel_item) {
    union KevKernelItemPool* freed_node = (union KevKernelItemPool*)kernel_item;
    freed_node->next = kernel_item_pool;
    kernel_item_pool = freed_node;
  }
}

void kev_kernel_item_pool_free(void) {
  union KevKernelItemPool* pool = kernel_item_pool;
  kernel_item_pool = NULL;
  while (pool) {
    union KevKernelItemPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
