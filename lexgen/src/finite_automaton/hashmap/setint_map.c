#include "lexgen/include/finite_automaton/hashmap/setint_map.h"

#include <stdlib.h>

inline static size_t kev_setintmap_hashing(KevBitSet* key) {
  if (!key) return 0;
  size_t length = key->length;
  size_t* bits = key->bits;
  size_t hashing_value = 0;
  for (size_t i = 0; i < length; ++i) {
    hashing_value += bits[i] >> (hashing_value & 0x3F);
  }
  return hashing_value;
}

static void kev_setintmap_rehash(KevSetIntMap* to, KevSetIntMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevSetIntMapNode** from_array = from->array;
  KevSetIntMapNode** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevSetIntMapNode* node = from_array[i];
    while (node) {
      KevSetIntMapNode* tmp = node->next;
      size_t hash_val = kev_setintmap_hashing(node->key);
      size_t index = hash_val & mask;
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

static bool kev_setintmap_expand(KevSetIntMap* map) {
  KevSetIntMap new_map;
  if (!kev_setintmap_init(&new_map, map->capacity << 1))
    return false;
  kev_setintmap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_setintmap_bucket_free(KevSetIntMapNode* bucket) {
  while (bucket) {
    KevSetIntMapNode* tmp = bucket->next;
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

bool kev_setintmap_init(KevSetIntMap* map, size_t capacity) {
  if (!map) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KevSetIntMapNode** array = (KevSetIntMapNode**)malloc(sizeof (KevSetIntMapNode*) * capacity);
  if (!array) {
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

void kev_setintmap_destroy(KevSetIntMap* map) {
  if (map) {
    KevSetIntMapNode** array = map->array;
    size_t capacity = map->capacity;
    for (size_t i = 0; i < capacity; ++i)
      kev_setintmap_bucket_free(array[i]);
    free(array);
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

bool kev_setintmap_insert(KevSetIntMap* map, KevBitSet* key, size_t value) {
  if (map->size >= map->capacity && !kev_setintmap_expand(map))
    return false;

  KevSetIntMapNode* new_node = (KevSetIntMapNode*)malloc(sizeof (KevSetIntMapNode));
  if (!new_node) return false;

  size_t index = (map->capacity - 1) & kev_setintmap_hashing(key);
  new_node->key = key;
  new_node->value = value;
  new_node->next = map->array[index];
  map->array[index] = new_node;
  map->size++;
  return true;
}

KevSetIntMapNode* kev_setintmap_search(KevSetIntMap* map, KevBitSet* key) {
  size_t index = (map->capacity - 1) & kev_setintmap_hashing(key);
  KevSetIntMapNode* node = map->array[index];
  while (node) {
    if (kev_bitset_equal(node->key, key))
      break;
    node = node->next;
  }
  return node;
}

void kev_setintmap_make_empty(KevSetIntMap* map) {
  KevSetIntMapNode** array = map->array;
  size_t capacity = map->capacity;
  for (size_t i = 0; i < capacity; ++i) {
    kev_setintmap_bucket_free(array[i]);
    array[i] = NULL;
  }
  map->size = 0;
}
