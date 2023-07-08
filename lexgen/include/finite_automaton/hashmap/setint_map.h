#ifndef KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_HASHMAP_SETINT_MAP_H
#define KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_HASHMAP_SETINT_MAP_H

#include "lexgen/include/finite_automaton/set/bitset.h"
#include "lexgen/include/general/global_def.h"

typedef struct tagKevSetIntMapNode {
  KevBitSet* key;
  size_t value;
  struct tagKevSetIntMapNode* next;
} KevSetIntMapNode;

typedef struct tagKevSetIntMap {
  KevSetIntMapNode** array;
  size_t capacity;
  size_t size;
} KevSetIntMap;

bool kev_setintmap_init(KevSetIntMap* map, size_t capacity);
void kev_setintmap_destroy(KevSetIntMap* map);

bool kev_setintmap_insert(KevSetIntMap* map, KevBitSet* key, size_t value);
KevSetIntMapNode* kev_setintmap_search(KevSetIntMap* map, KevBitSet* key);
void kev_setintmap_make_empty(KevSetIntMap* map);

#endif
