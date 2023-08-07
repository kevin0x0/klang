#ifndef KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_KERNEL_ITEM_POOL_H
#define KEVCC_PARGEN_INCLUDE_LR_OBJECT_POOL_KERNEL_ITEM_POOL_H

#include "pargen/include/lr/item_def.h"

KevKernelItem* kev_kernel_item_pool_acquire(void);
KevKernelItem* kev_kernel_item_pool_allocate(void);
void kev_kernel_item_pool_deallocate(KevKernelItem* kernel_item);
void kev_kernel_item_pool_free(void);

#endif
