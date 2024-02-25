#ifndef KEVCC_UTILS_INCLUDE_QUEUE_ADDR_QUEUE_H
#define KEVCC_UTILS_INCLUDE_QUEUE_ADDR_QUEUE_H

#include "utils/include/general/global_def.h"
#include "utils/include/utils/utils.h"

typedef struct tagKevAddrQueue {
  void** array;
  size_t capacity;
  size_t head;
  size_t tail;
} KevAddrQueue;

bool kev_addrqueue_init(KevAddrQueue* queue);
void kev_addrqueue_destroy(KevAddrQueue* queue);

bool kev_addrqueue_expand(KevAddrQueue* queue);
static inline bool kev_addrqueue_insert(KevAddrQueue* queue, void* element);
static inline void* kev_addrqueue_pop(KevAddrQueue* queue);

static inline bool kev_addrqueue_empty(KevAddrQueue* queue) {
  return queue->head == queue->tail;
}

static inline bool kev_addrqueue_insert(KevAddrQueue* queue, void* element) {
  size_t new_tail = (queue->tail + 1) & (queue->capacity - 1);
  if (new_tail == queue->head) {
    if (k_unlikely(!kev_addrqueue_expand(queue)))
      return false;
    new_tail = (queue->tail + 1) & (queue->capacity - 1);
  }
  queue->array[queue->tail] = element;
  queue->tail = new_tail;
  return true;
}

static inline void* kev_addrqueue_pop(KevAddrQueue* queue) {
  void* retval = queue->array[queue->head];
  queue->head = (queue->head + 1) & (queue->capacity - 1);
  return retval;
}

#endif
