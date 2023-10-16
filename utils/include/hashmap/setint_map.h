#ifndef KEVCC_UTILS_INCLUDE_HASHMAP_SETINT_MAP_H
#define KEVCC_UTILS_INCLUDE_HASHMAP_SETINT_MAP_H

#include "utils/include/set/bitset.h"
#include "utils/include/general/global_def.h"

typedef struct tagKevSetIntMapNode {
  KBitSet* key;
  size_t value;
  size_t hashval;
  struct tagKevSetIntMapNode* next;
} KevSetIntMapNode;

typedef struct tagKevSetIntMap {
  KevSetIntMapNode** array;
  size_t capacity;
  size_t size;
} KevSetIntMap;

bool kev_setintmap_init(KevSetIntMap* map, size_t capacity);
void kev_setintmap_destroy(KevSetIntMap* map);

bool kev_setintmap_insert(KevSetIntMap* map, KBitSet* key, size_t value);
KevSetIntMapNode* kev_setintmap_search(KevSetIntMap* map, KBitSet* key);
void kev_setintmap_make_empty(KevSetIntMap* map);

#endif
