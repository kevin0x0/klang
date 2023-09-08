#ifndef KEVCC_KEVFA_INCLUDE_OBJECT_POOL_PARTITION_SET_POOL_H
#define KEVCC_KEVFA_INCLUDE_OBJECT_POOL_PARTITION_SET_POOL_H

#include "kevfa/include/set/partition.h"

KevPartitionSet* kev_partition_set_pool_allocate(void);
void kev_partition_set_pool_deallocate(KevPartitionSet* intlist_node);
void kev_partition_set_pool_free(void);

#endif
