#include "tokenizer_generator/include/regex_parser/hashmap/strfa_map.h"

#include <stdint.h>
#include <stdlib.h>

inline static uint64_t kev_strfamap_hashing(char* key) {
  uint64_t hash_val = 0;
  uint64_t count = 0;
  while (*key != '\0' && count < 8) {
    hash_val ^= ((uint64_t)*key << (hash_val & 0x3F));
  }
  return hash_val;
}

static void kev_strfamap_rehash(KevStringFaMap* to, KevStringFaMap* from) {
  uint64_t from_capacity = from->capacity;
  uint64_t to_capacity = to->capacity;
  KevStringFaMapBucket* from_array = from->array;
  KevStringFaMapBucket* to_array = to->array;
  KevStringFaMapBucket* bucket_head = NULL;
  uint64_t mask = to_capacity - 1;
  for (uint64_t i = 0; i < from_capacity; ++i) {
    KevStringFaMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevStringFaMapNode* tmp = node->next;
      uint64_t hash_val = kev_strfamap_hashing(node->key);
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

static bool kev_strfamap_expand(KevStringFaMap* map) {
  KevStringFaMap new_map;
  if (!kev_strfamap_init(&new_map, map->capacity << 1))
    return false;
  kev_strfamap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_strfamap_bucket_free(KevStringFaMapBucket* bucket) {
  KevStringFaMapNode* node = bucket->map_node_list;
  while (node) {
    KevStringFaMapNode* tmp = node->next;
    free(node);
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

bool kev_strfamap_init(KevStringFaMap* map, uint64_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KevStringFaMapBucket* array = (KevStringFaMapBucket*)malloc(sizeof (KevStringFaMapBucket) * capacity);
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

void kev_strfamap_destroy(KevStringFaMap* map) {
  if (map) {
    KevStringFaMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KevStringFaMapBucket* tmp = bucket->next;
      kev_strfamap_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

bool kev_strfamap_insert(KevStringFaMap* map, char* key, KevFA* value) {
  if (map->size >= map->capacity && !kev_strfamap_expand(map))
    return false;

  KevStringFaMapNode* new_node = (KevStringFaMapNode*)malloc(sizeof (KevStringFaMapNode));
  if (!new_node) return false;

  uint64_t index = (map->capacity - 1) & kev_strfamap_hashing(key);
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

KevStringFaMapNode* kev_strfamap_search(KevStringFaMap* map, char* key) {
  uint64_t index = (map->capacity - 1) & kev_strfamap_hashing(key);
  KevStringFaMapNode* node = map->array[index].map_node_list;
  while (node) {
    if (node->key == key)
      break;
    node = node->next;
  }
  return node;
}

void kev_strfamap_make_empty(KevStringFaMap* map) {
  KevStringFaMapBucket* bucket = map->bucket_head;
  while (bucket) {
    KevStringFaMapBucket* tmp = bucket->next;
    kev_strfamap_bucket_free(bucket);
    bucket = tmp;
  }
  
  map->bucket_head = NULL;
  map->size = 0;
}

KevStringFaMapNode* kev_strfamap_iterate_next(KevStringFaMap* map, KevStringFaMapNode* current) {
  if (current->next) return current->next;
  uint64_t index = (map->capacity - 1) & kev_strfamap_hashing(current->key);
  KevStringFaMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}