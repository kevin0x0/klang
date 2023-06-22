#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_HASHMAP_SETINT_MAP_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_HASHMAP_SETINT_MAP_H

#include "tokenizer_generator/include/finite_automaton/bitset/bitset.h"
#include "tokenizer_generator/include/general/global_def.h"

typedef struct tagKevSetIntMapNode {
  KevBitSet* key;
  uint64_t value;
  struct tagKevSetIntMapNode* next;
} KevSetIntMapNode;

typedef struct tagKevSetIntMap {
  KevSetIntMapNode** array;
  uint64_t capacity;
  uint64_t size;
} KevSetIntMap;


bool kev_setintmap_init(KevSetIntMap* map, uint64_t capacity);
void kev_setintmap_destroy(KevSetIntMap* map);

bool kev_setintmap_insert(KevSetIntMap* map, KevBitSet* key, uint64_t value);
KevSetIntMapNode* kev_setintmap_search(KevSetIntMap* map, KevBitSet* key);
void kev_setintmap_make_empty(KevSetIntMap* map);

#endif
