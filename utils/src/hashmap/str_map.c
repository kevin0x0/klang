#include "utils/include/hashmap/str_map.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>
#include <string.h>


inline static size_t kev_strmap_hashing(const char* key) {
  if (!key) return 0;
  size_t hashval = 0;
  size_t count = 0;
  while (*key != '\0' && count++ < 8) {
    hashval ^= ((size_t)*key++ << (hashval & 0x3F));
  }
  return hashval;
}

static void kev_strmap_rehash(KevStringMap* to, KevStringMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevStringMapBucket* from_array = from->array;
  KevStringMapBucket* to_array = to->array;
  KevStringMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevStringMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevStringMapNode* tmp = node->next;
      size_t hashval = node->hashval;
      size_t index = hashval & mask;
      node->next = to_array[index].map_node_list;
      to_array[index].map_node_list = node;
      /* this means 'node' is the first element that put in this bucket,
       * so this bucket has not added to bucket list yet. */
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

static bool kev_strmap_expand(KevStringMap* map) {
  KevStringMap new_map;
  if (!kev_strmap_init(&new_map, map->capacity << 1))
    return false;
  kev_strmap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_strmap_bucket_free(KevStringMapBucket* bucket) {
  KevStringMapNode* node = bucket->map_node_list;
  while (node) {
    KevStringMapNode* tmp = node->next;
    free(node->key);
    free(node->value);
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

bool kev_strmap_init(KevStringMap* map, size_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KevStringMapBucket* array = (KevStringMapBucket*)malloc(sizeof (KevStringMapBucket) * capacity);
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

KevStringMap* kev_strmap_create(size_t capacity) {
  KevStringMap* map = (KevStringMap*)malloc(sizeof (KevStringMap));
  if (!map || !kev_strmap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
}

void kev_strmap_destroy(KevStringMap* map) {
  if (map) {
    KevStringMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KevStringMapBucket* tmp = bucket->next;
      kev_strmap_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

void kev_strmap_delete(KevStringMap* map) {
  kev_strmap_destroy(map);
  free(map);
}

bool kev_strmap_insert(KevStringMap* map, const char* key, const char* value) {
  if (map->size >= map->capacity && !kev_strmap_expand(map))
    return false;

  KevStringMapNode* new_node = (KevStringMapNode*)malloc(sizeof (KevStringMapNode));
  if (!new_node) return false;

  size_t hashval = kev_strmap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  new_node->key = kev_str_copy(key);
  new_node->hashval = hashval;
  new_node->value = kev_str_copy(value);
  if (!new_node->key || !new_node->value) {
    free(new_node->key);
    free(new_node->value);
    return false;
  }
  new_node->next = map->array[index].map_node_list;
  map->array[index].map_node_list = new_node;
  map->size++;
  if (new_node->next == NULL) {
    map->array[index].next = map->bucket_head;
    map->bucket_head = &map->array[index];
  }
  return true;
}

bool kev_strmap_insert_move(KevStringMap* map, const char* key, char* value) {
  if (map->size >= map->capacity && !kev_strmap_expand(map))
    return false;

  KevStringMapNode* new_node = (KevStringMapNode*)malloc(sizeof (KevStringMapNode));
  if (!new_node) return false;

  size_t hashval = kev_strmap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  new_node->key = kev_str_copy(key);
  new_node->hashval = hashval;
  new_node->value = value;
  if (!new_node->key || !new_node->value) {
    free(new_node->key);
    return false;
  }
  new_node->next = map->array[index].map_node_list;
  map->array[index].map_node_list = new_node;
  map->size++;
  if (new_node->next == NULL) {
    map->array[index].next = map->bucket_head;
    map->bucket_head = &map->array[index];
  }
  return true;
}

KevStringMapNode* kev_strmap_search(KevStringMap* map, const char* key) {
  size_t  hashval = kev_strmap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  KevStringMapNode* node = map->array[index].map_node_list;
  for (; node; node = node->next) {
    if (node->hashval == hashval &&
        strcmp(node->key, key) == 0)
      break;
  }
  return node;
}

KevStringMapNode* kev_strmap_iterate_next(KevStringMap* map, KevStringMapNode* current) {
  if (current->next) return current->next;
  size_t index = (map->capacity - 1) & current->hashval;
  KevStringMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}

bool kev_strmap_update(KevStringMap* map, const char* key, const char* value) {
  if (!map) return false;
  KevStringMapNode* node = kev_strmap_search(map, key);
  if (node) {
    free(node->value);
    node->value = kev_str_copy(value);
    return node->value != NULL;
  }
  else if (!kev_strmap_insert(map, key, value)) {
    return false;
  }
  return true;
}

bool kev_strmap_update_move(KevStringMap* map, const char* key, char* value) {
  if (!map) return false;
  KevStringMapNode* node = kev_strmap_search(map, key);
  if (node) {
    free(node->value);
    node->value = value;
    return true;
  }
  else {
    return kev_strmap_insert_move(map, key, value);
  }
}

bool kev_strmap_node_set_value(KevStringMapNode* node, const char* value) {
  char* tmp = kev_str_copy(value);
  if (!tmp) return false;
  free(kev_strmap_node_swap_value(node, tmp));
  return true;
}

void kev_strmap_node_set_value_move(KevStringMapNode* node, char* value) {
  free(kev_strmap_node_swap_value(node, value));
}
