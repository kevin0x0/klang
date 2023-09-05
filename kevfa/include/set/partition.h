#ifndef KEVCC_KEVFA_INCLUDE_SET_PARTITION_H
#define KEVCC_KEVFA_INCLUDE_SET_PARTITION_H
/* only used for hopcroft algorithm in dfa_minimization.c */

#include "kevfa/include/graph.h"
#include "kevfa/include/list/node_list.h"
#include "utils/include/general/global_def.h"

typedef KevGraphNode* KevPartitionElementType;
typedef KevPartitionElementType* KevPartitionUniverse;

typedef struct tagKevPartitionSet {
  size_t begin;
  size_t end;
} KevPartitionSet;

KevPartitionUniverse kev_partition_universe_create(size_t element_number);
void kev_partition_universe_delete(KevPartitionUniverse universe);
KevPartitionSet* kev_partition_refine(KevPartitionUniverse universe, KevPartitionSet* set, KevNodeList* target);
static inline void kev_partition_swap_element(KevPartitionUniverse universe, size_t element1, size_t element2);
static inline void kev_partition_indeces(KevPartitionUniverse universe, KevPartitionSet* set, size_t index);
KevPartitionSet* kev_partition_set_create(KevPartitionUniverse universe, KevNodeList* nodes, size_t begin_position);
KevPartitionSet* kev_partition_set_create_from_graphnode(KevPartitionUniverse universe, KevGraphNode* nodes, KevGraphNode* end, size_t begin_position);
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

static inline void kev_partition_swap_element(KevPartitionUniverse universe, size_t element1, size_t element2) {
  KevGraphNode* tmp = universe[element1];
  universe[element1] = universe[element2];
  universe[element2] = tmp;
  universe[element1]->id = element1;
  universe[element2]->id = element2;
}

static inline void kev_partition_indeces(KevPartitionUniverse universe, KevPartitionSet* set, size_t index) {
  for (size_t i = set->begin; i < set->end; ++i)
    universe[i]->id = index;
}

static inline KevGraphNode* kev_partition_set_representative(KevPartitionUniverse universe, KevPartitionSet* set) {
  return universe[set->begin];
}

#endif
