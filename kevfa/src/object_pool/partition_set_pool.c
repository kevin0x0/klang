#include "kevfa/include/object_pool/partition_set_pool.h"

#include <stdlib.h>

union KevPartitionSetPool {
  KevPartitionSet partition_set;
  union KevPartitionSetPool* next;
};

static union KevPartitionSetPool* partition_set_pool = NULL;

static inline KevPartitionSet* kev_partition_set_pool_acquire(void);


static inline KevPartitionSet* kev_partition_set_pool_acquire(void) {
  return (KevPartitionSet*)malloc(sizeof (union KevPartitionSetPool));
}

KevPartitionSet* kev_partition_set_pool_allocate(void) {
  if (partition_set_pool) {
    KevPartitionSet* retval = &partition_set_pool->partition_set;
    partition_set_pool = partition_set_pool->next;
    return retval;
  } else {
    return kev_partition_set_pool_acquire();
  }
}

void kev_partition_set_pool_deallocate(KevPartitionSet* partition_set) {
  if (partition_set) {
    union KevPartitionSetPool* freed_node = (union KevPartitionSetPool*)partition_set;
    freed_node->next = partition_set_pool;
    partition_set_pool = freed_node;
  }
}

void kev_partition_set_pool_free(void) {
  union KevPartitionSetPool* pool = partition_set_pool;
  partition_set_pool = NULL;
  while (pool) {
    union KevPartitionSetPool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
