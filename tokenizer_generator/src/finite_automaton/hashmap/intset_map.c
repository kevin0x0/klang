#include "tokenizer_generator/include/finite_automaton/hashmap/intset_map.h"
#include "tokenizer_generator/include/finite_automaton/object_pool/intsetmap_node_pool.h"


#include <stdlib.h>

inline static uint64_t kev_intsetmap_hashing(uint64_t key) {
  return (uint64_t)key;
}

static void kev_intsetmap_rehash(KevIntSetMap* to, KevIntSetMap* from) {
  uint64_t from_capacity = from->capacity;
  uint64_t to_capacity = to->capacity;
  KevIntSetMapBucket* from_array = from->array;
  KevIntSetMapBucket* to_array = to->array;
  KevIntSetMapBucket* bucket_head = NULL;
  uint64_t mask = to_capacity - 1;
  for (uint64_t i = 0; i < from_capacity; ++i) {
    KevIntSetMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevIntSetMapNode* tmp = node->next;
      uint64_t hash_val = kev_intsetmap_hashing(node->key);
      uint64_t index = hash_val & mask;
      node->next = to_array[index].map_node_list;
      to_array[index].map_node_list = node;
      if (node->next == NULL) {
        to_array[index].next = bucket_head;
        bucket_head = &to_array[index];
      }
      node = tmp;
    }
  }
  to->size = from->size;
  to->bucket_head = bucket_head;
  free(from->array);
  from->bucket_head = NULL;
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool kev_intsetmap_expand(KevIntSetMap* map) {
  KevIntSetMap new_map;
  if (!kev_intsetmap_init(&new_map, map->capacity << 1))
    return false;
  kev_intsetmap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_intset_map_bucket_free(KevIntSetMapBucket* bucket) {
  KevIntSetMapNode* node = bucket->map_node_list;
  while (node) {
    KevIntSetMapNode* tmp = node->next;
    kev_intsetmap_node_pool_deallocate(node);
    node = tmp;
  }
  bucket->map_node_list = NULL;
  bucket->next = NULL;
}

inline static uint64_t pow_of_2_above(uint64_t num) {
  uint64_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool kev_intsetmap_init(KevIntSetMap* map, uint64_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KevIntSetMapBucket* array = (KevIntSetMapBucket*)malloc(sizeof (KevIntSetMapBucket) * capacity);
  if (!array) {
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
    return false;
  }

  for (uint64_t i = 0; i < capacity; ++i) {
    array[i].map_node_list = NULL;
  }
  
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  return true;
}

void kev_intsetmap_destroy(KevIntSetMap* map) {
  if (map) {
    KevIntSetMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KevIntSetMapBucket* tmp = bucket->next;
      kev_intset_map_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

bool kev_intsetmap_insert(KevIntSetMap* map, uint64_t key, KevBitSet* value) {
  if (map->size >= map->capacity && !kev_intsetmap_expand(map))
    return false;

  KevIntSetMapNode* new_node = kev_intsetmap_node_pool_allocate();
  if (!new_node) return false;

  uint64_t index = (map->capacity - 1) & kev_intsetmap_hashing(key);
  new_node->key = key;
  new_node->value = value;
  new_node->next = map->array[index].map_node_list;
  map->array[index].map_node_list = new_node;
  map->size++;
  if (new_node->next == NULL) {
    map->array[index].next = map->bucket_head;
    map->bucket_head = &map->array[index];
  }
  return true;
}

KevIntSetMapNode* kev_intsetmap_search(KevIntSetMap* map, uint64_t key) {
  uint64_t index = (map->capacity - 1) & kev_intsetmap_hashing(key);
  KevIntSetMapNode* node = map->array[index].map_node_list;
  while (node) {
    if (node->key == key)
      break;
    node = node->next;
  }
  return node;
}

void kev_intsetmap_make_empty(KevIntSetMap* map) {
  KevIntSetMapBucket* bucket = map->bucket_head;
  while (bucket) {
    KevIntSetMapBucket* tmp = bucket->next;
    kev_intset_map_bucket_free(bucket);
    bucket = tmp;
  }
  
  map->bucket_head = NULL;
  map->size = 0;
}

KevIntSetMapNode* kev_intsetmap_iterate_next(KevIntSetMap* map, KevIntSetMapNode* current) {
  if (current->next) return current->next;
  uint64_t index = (map->capacity - 1) & kev_intsetmap_hashing(current->key);
  KevIntSetMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}


