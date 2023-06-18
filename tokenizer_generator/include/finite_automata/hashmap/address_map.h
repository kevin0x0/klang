#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_HASHMAP_ADDRESS_MAP_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_HASHMAP_ADDRESS_MAP_H

#include "tokenizer_generator/include/general/int_type.h"

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


bool kev_address_map_init(KevAddressMap* map, uint64_t capacity);
void kev_address_map_destroy(KevAddressMap* map);

bool kev_address_map_insert(KevAddressMap* map, void* key, void* value);
KevAddressMapNode* kev_address_map_search(KevAddressMap* map, void* key);




#endif
