#include "tokenizer_generator/include/finite_automaton/hashmap/address_map.h"

#include <stdlib.h>

inline static uint64_t kev_addressmap_hashing(void* key) {
  return (uint64_t)key;
}

static void kev_addressmap_rehash(KevAddressMap* to, KevAddressMap* from) {
  uint64_t from_capacity = from->capacity;
  uint64_t to_capacity = to->capacity;
  KevAddressMapNode** from_array = from->array;
  KevAddressMapNode** to_array = to->array;
  uint64_t mask = to_capacity - 1;
  for (uint64_t i = 0; i < from_capacity; ++i) {
    KevAddressMapNode* node = from_array[i];
    while (node) {
      KevAddressMapNode* tmp = node->next;
      uint64_t hash_val = kev_addressmap_hashing(node->key);
      uint64_t index = hash_val & mask;
      node->next = to_array[index];
      to_array[index] = node;
      node = tmp;
    }
  }
  to->size = from->size;
  free(from->array);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool kev_addressmap_expand(KevAddressMap* map) {
  KevAddressMap new_map;
  if (!kev_addressmap_init(&new_map, map->capacity << 1))
    return false;
  kev_addressmap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_addressmap_bucket_free(KevAddressMapNode* bucket) {
  while (bucket) {
    KevAddressMapNode* tmp = bucket->next;
    free(bucket);
    bucket = tmp;
  }
}

inline static uint64_t pow_of_2_above(uint64_t num) {
  uint64_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool kev_addressmap_init(KevAddressMap* map, uint64_t capacity) {
  if (!map) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KevAddressMapNode** array = (KevAddressMapNode**)malloc(sizeof (KevAddressMapNode*) * capacity);
  if (!array) {
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
    return false;
  }

  for (uint64_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  return true;
}

void kev_addressmap_destroy(KevAddressMap* map) {
  if (map) {
    KevAddressMapNode** array = map->array;
    uint64_t capacity = map->capacity;
    for (uint64_t i = 0; i < capacity; ++i)
      kev_addressmap_bucket_free(array[i]);
    free(array);
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

bool kev_addressmap_insert(KevAddressMap* map, void* key, void* value) {
  if (map->size >= map->capacity && !kev_addressmap_expand(map))
    return false;

  KevAddressMapNode* new_node = (KevAddressMapNode*)malloc(sizeof (*new_node));
  if (!new_node) return false;

  uint64_t index = (map->capacity - 1) & kev_addressmap_hashing(key);
  new_node->key = key;
  new_node->value = value;
  new_node->next = map->array[index];
  map->array[index] = new_node;
  map->size++;
  return true;
}

KevAddressMapNode* kev_addressmap_search(KevAddressMap* map, void* key) {
  uint64_t index = (map->capacity - 1) & kev_addressmap_hashing(key);
  KevAddressMapNode* node = map->array[index];
  while (node) {
    if (node->key == key)
      break;
    node = node->next;
  }

  return node;
}
