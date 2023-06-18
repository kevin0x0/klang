#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_HASHMAP_SETINT_MAP_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_HASHMAP_SETINT_MAP_H

#include "tokenizer_generator/include/finite_automata/bitset/bitset.h"
#include "tokenizer_generator/include/general/int_type.h"

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


bool kev_setint_map_init(KevSetIntMap* map, uint64_t capacity);
void kev_setint_map_destroy(KevSetIntMap* map);

bool kev_setint_map_insert(KevSetIntMap* map, KevBitSet* key, uint64_t value);
KevSetIntMapNode* kev_setint_map_search(KevSetIntMap* map, KevBitSet* key);
void kev_setint_map_make_empty(KevSetIntMap* map);

#endif
