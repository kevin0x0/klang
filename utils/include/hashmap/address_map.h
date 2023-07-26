#ifndef KEVCC_UTILS_INCLUDE_HASHMAP_ADDRESS_MAP_H
#define KEVCC_UTILS_INCLUDE_HASHMAP_ADDRESS_MAP_H

#include "utils/include/general/global_def.h"

typedef struct tagKevAddressMapNode {
  void* key;
  void* value;
  struct tagKevAddressMapNode* next;
} KevAddressMapNode;

typedef struct tagKevAddressMap {
  KevAddressMapNode** array;
  size_t capacity;
  size_t size;
} KevAddressMap;

bool kev_addressmap_init(KevAddressMap* map, size_t capacity);
void kev_addressmap_destroy(KevAddressMap* map);

bool kev_addressmap_insert(KevAddressMap* map, void* key, void* value);
KevAddressMapNode* kev_addressmap_search(KevAddressMap* map, void* key);

#endif
