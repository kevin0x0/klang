#include "lexgen/include/parser/hashmap/strfa_map.h"

#include <stdlib.h>
#include <string.h>

inline static size_t kev_strfamap_hashing(char* key) {
  if (!key) return 0;
  size_t hashval = 0;
  size_t count = 0;
  while (*key != '\0' && count++ < 8) {
    hashval ^= ((size_t)*key++ << (hashval & 0x3F));
  }
  return hashval;
}

static void kev_strfamap_rehash(KevStringFaMap* to, KevStringFaMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevStringFaMapBucket* from_array = from->array;
  KevStringFaMapBucket* to_array = to->array;
  KevStringFaMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevStringFaMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevStringFaMapNode* tmp = node->next;
      size_t hashval = node->hashval;
      size_t index = hashval & mask;
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

inline static size_t pow_of_2_above(size_t num) {
  size_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool kev_strfamap_init(KevStringFaMap* map, size_t capacity) {
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

  for (size_t i = 0; i < capacity; ++i) {
    array[i].map_node_list = NULL;
  }
  
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  return true;
}

KevStringFaMap* kev_strfamap_create(size_t capacity) {
  KevStringFaMap* map = (KevStringFaMap*)malloc(sizeof (KevStringFaMap));
  if (!map || !kev_strfamap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
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

void kev_strfamap_delete(KevStringFaMap* map) {
  kev_strfamap_destroy(map);
  free(map);
}

bool kev_strfamap_insert(KevStringFaMap* map, char* key, KevFA* value) {
  if (map->size >= map->capacity && !kev_strfamap_expand(map))
    return false;

  KevStringFaMapNode* new_node = (KevStringFaMapNode*)malloc(sizeof (KevStringFaMapNode));
  if (!new_node) return false;

  size_t hashval = kev_strfamap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  new_node->key = key;
  new_node->hashval = hashval;
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
  size_t hashval = kev_strfamap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  KevStringFaMapNode* node = map->array[index].map_node_list;
  while (node) {
    if (node->hashval == hashval &&
        strcmp(node->key, key) == 0)
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
  size_t index = (map->capacity - 1) & current->hashval;
  KevStringFaMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}

bool kev_strfamap_update(KevStringFaMap* map, char* key, KevFA* value) {
  if (!map) return false;
  KevStringFaMapNode* node = kev_strfamap_search(map, key);
  if (node) {
    node->value = value;
    return true;
  }
  else if (!kev_strfamap_insert(map, key, value)) {
    return false;
  }
  return true;
}
