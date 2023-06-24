#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_SET_PARTITION_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_SET_PARTITION_H
/* only used for hopcroft algorithm in dfa_minimization.c */

#include "tokenizer_generator/include/finite_automaton/graph.h"
#include "tokenizer_generator/include/finite_automaton/list/node_list.h"
#include "tokenizer_generator/include/general/global_def.h"
#include <stdint.h>

typedef KevGraphNode* KevPartitionElementType;
typedef KevPartitionElementType* KevPartitionUniverse;

typedef struct tagKevPartitionSet {
  uint64_t begin;
  uint64_t end;
} KevPartitionSet;

KevPartitionUniverse kev_partition_universe_create(uint64_t element_number);
void kev_partition_universe_delete(KevPartitionUniverse universe);
KevPartitionSet* kev_partition_refine(KevPartitionUniverse universe, KevPartitionSet* set, KevNodeList* target);
static inline void kev_partition_swap_element(KevPartitionUniverse universe, uint64_t element1, uint64_t element2);
static inline void kev_partition_indeces(KevPartitionUniverse universe, KevPartitionSet* set, uint64_t index);
KevPartitionSet* kev_partition_set_create(KevPartitionUniverse universe, KevNodeList* nodes, uint64_t begin_position);
KevPartitionSet* kev_partition_set_create_from_graphnode(KevPartitionUniverse universe, KevGraphNode* nodes, KevGraphNode* end, uint64_t begin_position);
static inline KevGraphNode* kev_partition_set_representative(KevPartitionUniverse universe, KevPartitionSet* set);
static inline bool kev_partition_set_has(KevPartitionSet* set, KevGraphNode* element);
static inline bool kev_partition_set_size(KevPartitionSet* set);
void kev_partition_set_delete(KevPartitionSet* set);

static inline bool kev_partition_set_has(KevPartitionSet* set, KevGraphNode* element) {
  return set->begin <= element->id && element->id < set->end;
}

static inline bool kev_partition_set_size(KevPartitionSet* set) {
  return set->end - set->begin;
}

static inline void kev_partition_swap_element(KevPartitionUniverse universe, uint64_t element1, uint64_t element2) {
  KevGraphNode* tmp = universe[element1];
  universe[element1] = universe[element2];
  universe[element2] = tmp;
  universe[element1]->id = element1;
  universe[element2]->id = element2;
}

static inline void kev_partition_indeces(KevPartitionUniverse universe, KevPartitionSet* set, uint64_t index) {
  for (uint64_t i = set->begin; i < set->end; ++i)
    universe[i]->id = index;
}

static inline KevGraphNode* kev_partition_set_representative(KevPartitionUniverse universe, KevPartitionSet* set) {
  return universe[set->begin];
}

#endif
