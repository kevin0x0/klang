#include "utils/include/hashmap/address_map.h"
#include "utils/include/utils/utils.h"

#include <stdlib.h>

inline static size_t kev_addressmap_hashing(void* key) {
  return (size_t)key >> 3;
}

static void kev_addressmap_rehash(KevAddressMap* to, KevAddressMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevAddressMapNode** from_array = from->array;
  KevAddressMapNode** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevAddressMapNode* node = from_array[i];
    while (node) {
      KevAddressMapNode* tmp = node->next;
      size_t hashval = kev_addressmap_hashing(node->key);
      size_t index = hashval & mask;
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
  if (k_unlikely(!kev_addressmap_init(&new_map, map->capacity << 1)))
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

inline static size_t pow_of_2_above(size_t num) {
  size_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool kev_addressmap_init(KevAddressMap* map, size_t capacity) {
  if (!map) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KevAddressMapNode** array = (KevAddressMapNode**)malloc(sizeof (KevAddressMapNode*) * capacity);
  if (k_unlikely(!array)) {
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
    return false;
  }

  for (size_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  return true;
}

void kev_addressmap_destroy(KevAddressMap* map) {
  if (k_unlikely(!map)) return;

  KevAddressMapNode** array = map->array;
  size_t capacity = map->capacity;
  for (size_t i = 0; i < capacity; ++i)
    kev_addressmap_bucket_free(array[i]);
  free(array);
  map->array = NULL;
  map->capacity = 0;
  map->size = 0;
}

bool kev_addressmap_insert(KevAddressMap* map, void* key, void* value) {
  if (k_unlikely(map->size >= map->capacity && !kev_addressmap_expand(map)))
    return false;

  KevAddressMapNode* new_node = (KevAddressMapNode*)malloc(sizeof (*new_node));
  if (k_unlikely(!new_node)) return false;

  size_t index = (map->capacity - 1) & kev_addressmap_hashing(key);
  new_node->key = key;
  new_node->value = value;
  new_node->next = map->array[index];
  map->array[index] = new_node;
  map->size++;
  return true;
}

KevAddressMapNode* kev_addressmap_search(KevAddressMap* map, void* key) {
  size_t index = (map->capacity - 1) & kev_addressmap_hashing(key);
  KevAddressMapNode* node = map->array[index];
  for (; node; node = node->next) {
    if (node->key == key) break;
  }

  return node;
}
