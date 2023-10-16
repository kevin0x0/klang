#include "klang/include/value/value.h"

#include <stdlib.h>

union KlValuePool {
  KlValue value;
  union KlValuePool* next;
};

static union KlValuePool* klvalue_pool = NULL;

static inline KlValue* klvalue_pool_acquire(void);


static inline KlValue* klvalue_pool_acquire(void) {
  return (KlValue*)malloc(sizeof (union KlValuePool));
}

KlValue* klvalue_pool_allocate(void) {
  if (klvalue_pool) {
    KlValue* retval = &klvalue_pool->value;
    klvalue_pool = klvalue_pool->next;
    return retval;
  } else {
    return klvalue_pool_acquire();
  }
}

void klvalue_pool_deallocate(KlValue* value) {
  if (value) {
    union KlValuePool* freed_node = (union KlValuePool*)value;
    freed_node->next = klvalue_pool;
    klvalue_pool = freed_node;
  }
}

void klvalue_pool_free(void) {
  union KlValuePool* pool = klvalue_pool;
  klvalue_pool = NULL;
  while (pool) {
    union KlValuePool* tmp = pool->next;
    free(pool);
    pool = tmp;
  }
}
