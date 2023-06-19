#include "tokenizer_generator/include/finite_automata/hashmap/setint_map.h"
#include "tokenizer_generator/include/finite_automata/bitset/bitset.h"
#include "tokenizer_generator/include/general/int_type.h"

#include <stdlib.h>

inline static uint64_t kev_setint_map_hashing(KevBitSet* key) {
  if (!key) return 0;
  uint64_t length = key->length;
  uint64_t* bits = key->bits;
  uint64_t index = 0;
  for (uint64_t i = 0; i < length; ++i) {
    index += bits[i] >> (index & 0x3F);
  }
  return index;
}

static void kev_setint_map_rehash(KevSetIntMap* to, KevSetIntMap* from) {
  uint64_t from_capacity = from->capacity;
  uint64_t to_capacity = to->capacity;
  KevSetIntMapNode** from_array = from->array;
  KevSetIntMapNode** to_array = to->array;
  uint64_t mask = to_capacity - 1;
  for (uint64_t i = 0; i < from_capacity; ++i) {
    KevSetIntMapNode* node = from_array[i];
    while (node) {
      KevSetIntMapNode* tmp = node->next;
      uint64_t hash_val = kev_setint_map_hashing(node->key);
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

static bool kev_setint_map_expand(KevSetIntMap* map) {
  KevSetIntMap new_map;
  if (!kev_setint_map_init(&new_map, map->capacity << 1))
    return false;
  kev_setint_map_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_setint_map_bucket_free(KevSetIntMapNode* bucket) {
  while (bucket) {
    KevSetIntMapNode* tmp = bucket->next;
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

bool kev_setint_map_init(KevSetIntMap* map, uint64_t capacity) {
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

  for (uint64_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  return true;
}

void kev_setint_map_destroy(KevSetIntMap* map) {
  if (map) {
    KevSetIntMapNode** array = map->array;
    uint64_t capacity = map->capacity;
    for (uint64_t i = 0; i < capacity; ++i)
      kev_setint_map_bucket_free(array[i]);
    free(array);
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

bool kev_setint_map_insert(KevSetIntMap* map, KevBitSet* key, uint64_t value) {
  if (map->size >= map->capacity && !kev_setint_map_expand(map))
    return false;

  KevSetIntMapNode* new_node = (KevSetIntMapNode*)malloc(sizeof (KevSetIntMapNode));
  if (!new_node) return false;

  uint64_t index = (map->capacity - 1) & kev_setint_map_hashing(key);
  new_node->key = key;
  new_node->value = value;
  new_node->next = map->array[index];
  map->array[index] = new_node;
  map->size++;
  return true;
}

KevSetIntMapNode* kev_setint_map_search(KevSetIntMap* map, KevBitSet* key) {
  uint64_t index = (map->capacity - 1) & kev_setint_map_hashing(key);
  KevSetIntMapNode* node = map->array[index];
  while (node) {
    if (kev_bitset_equal(node->key, key))
      break;
    node = node->next;
  }
  return node;
}

void kev_setint_map_make_empty(KevSetIntMap* map) {
  KevSetIntMapNode** array = map->array;
  uint64_t capacity = map->capacity;
  for (uint64_t i = 0; i < capacity; ++i) {
    kev_setint_map_bucket_free(array[i]);
    array[i] = NULL;
  }
  map->size = 0;
}
