#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_QUEUE_INT_QUEUE_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_QUEUE_INT_QUEUE_H

#include "tokenizer_generator/include/general/global_def.h"

typedef struct tagKevIntQueue {
  size_t* array;
  size_t capacity;
  size_t head;
  size_t tail;
} KevIntQueue;

bool kev_intqueue_init(KevIntQueue* queue);
void kev_intqueue_destroy(KevIntQueue* queue);

bool kev_intqueue_expand(KevIntQueue* queue);
static inline bool kev_intqueue_insert(KevIntQueue* queue, size_t element);
static inline size_t kev_intqueue_pop(KevIntQueue* queue);

static inline bool kev_intqueue_empty(KevIntQueue* queue) {
  return queue->head == queue->tail;
}

static inline bool kev_intqueue_insert(KevIntQueue* queue, size_t element) {
  if (!queue) return false;
  size_t new_tail = (queue->tail + 1) & (queue->capacity - 1);
  if (new_tail == queue->head) {
    if (!kev_intqueue_expand(queue))
      return false;
    new_tail = (queue->tail + 1) & (queue->capacity - 1);
  }
  
  queue->array[queue->tail] = element;
  queue->tail = new_tail;
  return true;
}

static inline size_t kev_intqueue_pop(KevIntQueue* queue) {
  if (!queue || queue->head == queue->tail) return -1;
  size_t retval = queue->array[queue->head];
  queue->head = (queue->head + 1) & (queue->capacity - 1);
  return retval;
}

#endif
