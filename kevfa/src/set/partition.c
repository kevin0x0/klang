#include "kevfa/include/set/partition.h"
#include "kevfa/include/object_pool/partition_set_pool.h"

#include <stdlib.h>

KevPartitionUniverse kev_partition_universe_create(size_t element_number) {
  KevPartitionUniverse universe = (KevPartitionUniverse)malloc(sizeof (KevPartitionElementType) * element_number);
  return universe;
}

KevPartitionSet* kev_partition_set_create(KevPartitionUniverse universe, KevNodeList* nodes, size_t begin_position) {
  KevPartitionSet* set = kev_partition_set_pool_allocate();
  if (!set) return NULL;
  size_t end_position = begin_position;
  while (nodes) {
    nodes->element->id = end_position;
    universe[end_position++] = nodes->element;
    nodes = nodes->next;
  }
  set->begin = begin_position;
  set->end = end_position;
  return set;
}

KevPartitionSet* kev_partition_set_create_from_graphnode(KevPartitionUniverse universe, KevGraphNode* nodes, KevGraphNode* end, size_t begin_position) {
  KevPartitionSet* set = kev_partition_set_pool_allocate();
  if (!set) return NULL;
  size_t end_position = begin_position;
  while (nodes != end) {
    nodes->id = end_position;
    universe[end_position++] = nodes;
    nodes = nodes->next;
  }
  set->begin = begin_position;
  set->end = end_position;
  return set;
}

void kev_partition_universe_delete(KevPartitionUniverse universe) {
  free(universe);
}

KevPartitionSet* kev_partition_refine(KevPartitionUniverse partition, KevPartitionSet* set, KevNodeList* target) {
  KevPartitionSet* new_set = kev_partition_set_pool_allocate();
  if (!new_set) return NULL;
  size_t begin = set->begin;
  size_t end = set->end;
  size_t pivot = end;
  for (KevNodeListNode* node = target; node; node = node->next) {
    size_t id = node->element->id;
    if (begin <= id && id < pivot) {
      kev_partition_swap_element(partition, id, --pivot);
    }
  }
  set->end = pivot;
  new_set->begin = pivot;
  new_set->end = end;
  return new_set;
}

void kev_partition_set_delete(KevPartitionSet* set) {
  kev_partition_set_pool_deallocate(set);
}
