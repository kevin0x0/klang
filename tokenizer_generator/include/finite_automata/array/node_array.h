#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_ARRAY_INT_ARRAY_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_ARRAY_INT_ARRAY_H

#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/general/global_def.h"

typedef struct tagKevNodeArray {
  KevGraphNode** begin;
  KevGraphNode** end;
  KevGraphNode** current;
} KevNodeArray;

bool kev_nodearray_init(KevNodeArray* array);
void kev_nodearray_destroy(KevNodeArray* array);

bool kev_nodearray_push_back(KevNodeArray* array, KevGraphNode* node);
static inline void kev_nodearray_pop_back(KevNodeArray* array);
bool kev_nodearray_expand(KevNodeArray* array);
static inline void kev_nodearray_assign(KevNodeArray* array, uint64_t index, KevGraphNode* value);
static inline KevGraphNode* kev_nodearray_visit(KevNodeArray* array, uint64_t index);
static inline uint64_t kev_nodearray_size(KevNodeArray* array);

static inline uint64_t kev_nodearray_size(KevNodeArray* array) {
  return array->current - array->begin;
}

static inline void kev_nodearray_pop_back(KevNodeArray* array) {
  array->current--;
}

static inline KevGraphNode* kev_nodearray_visit(KevNodeArray* array, uint64_t index) {
  return array->begin[index];
}

static inline void kev_nodearray_assign(KevNodeArray* array, uint64_t index, KevGraphNode* value) {
  array->begin[index] = value;
}

#endif
