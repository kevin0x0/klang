#include "utils/include/queue/addr_queue.h"

#include <stdlib.h>
#include <string.h>

#define KEV_INTQUEUE_DEFAULT_SIZE   (32)

bool kev_addrqueue_init(KevAddrQueue* queue) {
  if (k_unlikely(!queue)) return false;
  queue->array = NULL;
  queue->head = 0;
  queue->array = (void**)malloc(sizeof (void*) * KEV_INTQUEUE_DEFAULT_SIZE);
  if (k_unlikely(!queue->array)) {
    queue->tail = 0;
    queue->capacity = 0;
    return false;
  }
  queue->tail = 0;
  queue->capacity = 8;
  return true;
}

void kev_addrqueue_destroy(KevAddrQueue* queue) {
  if (k_unlikely(!queue))
    return;
  free(queue->array);
  queue->array = NULL;
  queue->capacity = 0;
  queue->head = 0;
  queue->tail = 0;

}

bool kev_addrqueue_expand(KevAddrQueue* queue) {
  size_t new_size = queue->capacity * 2;
  void** array = (void**)malloc(new_size * sizeof (void*));
  if (k_unlikely(!array)) return false;
  if (queue->head < queue->tail) {
    memcpy(array, queue->array + queue->head, sizeof (void*) * (queue->tail - queue->head));
    free(queue->array);
    queue->array = array;
    queue->tail = queue->tail - queue->head;
    queue->head = 0;
    queue->capacity = new_size;
  } else if (queue->head > queue->tail) {
    memcpy(array, queue->array + queue->head, sizeof (void*) * (queue->capacity - queue->head));
    memcpy(array + (queue->capacity - queue->head), queue->array, queue->tail * sizeof (void*));
    queue->tail = queue->tail + queue->capacity - queue->head;
    queue->head = 0;
    queue->capacity = new_size;
  } else {
    queue->capacity = new_size;
  }
  return true;
}

