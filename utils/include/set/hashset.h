#ifndef KEVCC_UTILS_INCLUDE_SET_HASHSET_H
#define KEVCC_UTILS_INCLUDE_SET_HASHSET_H

#include "utils/include/general/global_def.h"

typedef struct tagKevHashSetNode {
  void* element;
  struct tagKevHashSetNode* next;
} KevHashSetNode;

typedef struct tagKevHashSet {
  KevHashSetNode** array;
  size_t capacity;
  size_t size;
} KevHashSet;

bool kev_hashset_init(KevHashSet* set, size_t capacity);
void kev_hashset_destroy(KevHashSet* set);

bool kev_hashset_insert(KevHashSet* set, void* element);
KevHashSetNode* kev_hashset_search(KevHashSet* set, void* element);

#endif
