#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_HASHMAP_ADDRESS_MAP_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATON_HASHMAP_ADDRESS_MAP_H

#include "tokenizer_generator/include/general/global_def.h"

typedef struct tagKevAddressMapNode {
  void* key;
  void* value;
  struct tagKevAddressMapNode* next;
} KevAddressMapNode;

typedef struct tagKevAddressMap {
  KevAddressMapNode** array;
  uint64_t capacity;
  uint64_t size;
} KevAddressMap;


bool kev_addressmap_init(KevAddressMap* map, uint64_t capacity);
void kev_addressmap_destroy(KevAddressMap* map);

bool kev_addressmap_insert(KevAddressMap* map, void* key, void* value);
KevAddressMapNode* kev_addressmap_search(KevAddressMap* map, void* key);




#endif
