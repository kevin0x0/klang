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

bool khashset_init(KevHashSet* set, size_t capacity);
void khashset_destroy(KevHashSet* set);

bool khashset_insert(KevHashSet* set, void* element);
bool khashset_has(KevHashSet* set, void* element);

#endif
