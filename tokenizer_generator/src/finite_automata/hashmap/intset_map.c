#include "tokenizer_generator/include/finite_automata/hashmap/intset_map.h"
#include "tokenizer_generator/include/finite_automata/bitset/bitset.h"
#include "tokenizer_generator/include/finite_automata/object_pool/intsetmap_node_pool.h"
#include "tokenizer_generator/include/general/int_type.h"


#include <stdlib.h>

inline static uint64_t kev_intset_map_hashing(uint64_t key) {
  return (uint64_t)key;
}

static void kev_intset_map_rehash(KevIntSetMap* to, KevIntSetMap* from) {
  if (!to || !from) return;

  uint64_t from_capacity = from->capacity;
  uint64_t to_capacity = to->capacity;
  KevIntSetMapNode** from_array = from->array;
  KevIntSetMapNode** to_array = to->array;
  uint64_t mask = to_capacity - 1;
  for (uint64_t i = 0; i < from_capacity; ++i) {
    KevIntSetMapNode* node = from_array[i];
    while (node) {
      KevIntSetMapNode* tmp = node->next;
      uint64_t hash_val = kev_intset_map_hashing(node->key);
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

static bool kev_intset_map_expand(KevIntSetMap* map) {
  if (!map) return false;

  KevIntSetMap new_map;
  if (!kev_intset_map_init(&new_map, map->capacity << 1))
    return false;
  kev_intset_map_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_intset_map_bucket_free(KevIntSetMapNode* bucket) {
  while (bucket) {
    KevIntSetMapNode* tmp = bucket->next;
    kev_intsetmap_node_pool_deallocate(bucket);
    bucket = tmp;
  }
}

inline static uint64_t pow_of_2_above(uint64_t num) {
  uint64_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool kev_intset_map_init(KevIntSetMap* map, uint64_t capacity) {
  if (!map) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KevIntSetMapNode** array = (KevIntSetMapNode**)malloc(sizeof (KevIntSetMapNode*) * capacity);
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

void kev_intset_map_destroy(KevIntSetMap* map) {
  if (map) {
    KevIntSetMapNode** array = map->array;
    uint64_t capacity = map->capacity;
    for (uint64_t i = 0; i < capacity; ++i)
      kev_intset_map_bucket_free(array[i]);
    free(array);
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

bool kev_intset_map_insert(KevIntSetMap* map, uint64_t key, KevBitSet* value) {
  if (map->size >= map->capacity && !kev_intset_map_expand(map))
    return false;

  KevIntSetMapNode* new_node = kev_intsetmap_node_pool_allocate();
  if (!new_node) return false;

  uint64_t index = (map->capacity - 1) & kev_intset_map_hashing(key);
  new_node->key = key;
  new_node->value = value;
  new_node->next = map->array[index];
  map->array[index] = new_node;
  map->size++;
  return true;
}

KevIntSetMapNode* kev_intset_map_search(KevIntSetMap* map, uint64_t key) {
  uint64_t index = (map->capacity - 1) & kev_intset_map_hashing(key);
  KevIntSetMapNode* node = map->array[index];
  while (node) {
    if (node->key == key)
      break;
    node = node->next;
  }
  return node;
}

void kev_intset_map_make_empty(KevIntSetMap* map) {
  if (!map) return;

  KevIntSetMapNode** array = map->array;
  uint64_t capacity = map->capacity;
  for (uint64_t i = 0; i < capacity; ++i) {
    kev_intset_map_bucket_free(array[i]);
    array[i] = NULL;
  }
  map->size = 0;
}

KevIntSetMapNode* kev_intset_map_iterate_next(KevIntSetMap* map, KevIntSetMapNode* current) {
  if (!current) return NULL;
  if (current->next) return current->next;
  uint64_t index = kev_intset_map_hashing(current->key);
  for (uint64_t i = index + 1; i < map->capacity; ++i)
    if (map->array[i]) return map->array[i];
  return NULL;
}


