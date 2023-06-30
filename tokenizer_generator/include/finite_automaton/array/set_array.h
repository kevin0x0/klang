#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_ARRAY_SET_ARRAY_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_ARRAY_SET_ARRAY_H

#include "tokenizer_generator/include/finite_automaton/set/bitset.h"
#include "tokenizer_generator/include/general/global_def.h"

typedef struct tagKevSetArray {
  KevBitSet** begin;
  KevBitSet** end;
  KevBitSet** current;
} KevSetArray;

bool kev_setarray_init(KevSetArray* array);
void kev_setarray_destroy(KevSetArray* array);

bool kev_setarray_push_back(KevSetArray* array, KevBitSet* set);
static inline void kev_setarray_pop_back(KevSetArray* array);
bool kev_setarray_expand(KevSetArray* array);
static inline KevBitSet* kev_setarray_visit(KevSetArray* array, size_t index);
static inline size_t kev_setarray_size(KevSetArray* array);

static inline size_t kev_setarray_size(KevSetArray* array) {
  return array->current - array->begin;
}

static inline void kev_setarray_pop_back(KevSetArray* array) {
  if (array) array->current--;
}

static inline KevBitSet* kev_setarray_visit(KevSetArray* array, size_t index) {
  if (!array) return NULL;
  return array->begin[index];
}

#endif
