#include "utils/include/hashmap/strx_map.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>
#include <string.h>


inline static size_t kev_strxmap_hashing(const char* key) {
  if (!key) return 0;
  size_t hashval = 0;
  size_t count = 0;
  while (*key != '\0' && count++ < 8) {
    hashval ^= ((size_t)*key++ << (hashval & 0x3F));
  }
  return hashval;
}

static void kev_strxmap_rehash(KevStrXMap* to, KevStrXMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevStrXMapBucket* from_array = from->array;
  KevStrXMapBucket* to_array = to->array;
  KevStrXMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevStrXMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevStrXMapNode* tmp = node->next;
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

static bool kev_strxmap_expand(KevStrXMap* map) {
  KevStrXMap new_map;
  if (!kev_strxmap_init(&new_map, map->capacity << 1))
    return false;
  kev_strxmap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_strxmap_bucket_free(KevStrXMapBucket* bucket) {
  KevStrXMapNode* node = bucket->map_node_list;
  while (node) {
    KevStrXMapNode* tmp = node->next;
    free(node->key);
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

bool kev_strxmap_init(KevStrXMap* map, size_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KevStrXMapBucket* array = (KevStrXMapBucket*)malloc(sizeof (KevStrXMapBucket) * capacity);
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

KevStrXMap* kev_strxmap_create(size_t capacity) {
  KevStrXMap* map = (KevStrXMap*)malloc(sizeof (KevStrXMap));
  if (!map || !kev_strxmap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
}

void kev_strxmap_destroy(KevStrXMap* map) {
  if (map) {
    KevStrXMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KevStrXMapBucket* tmp = bucket->next;
      kev_strxmap_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

void kev_strxmap_delete(KevStrXMap* map) {
  kev_strxmap_destroy(map);
  free(map);
}

bool kev_strxmap_insert(KevStrXMap* map, const char* key, void* value) {
  if (map->size >= map->capacity && !kev_strxmap_expand(map))
    return false;

  KevStrXMapNode* new_node = (KevStrXMapNode*)malloc(sizeof (KevStrXMapNode));
  if (!new_node) return false;

  size_t hashval = kev_strxmap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  new_node->key = kev_str_copy(key);
  new_node->hashval = hashval;
  new_node->value = value;
  if (!new_node->key) {
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

KevStrXMapNode* kev_strxmap_search(KevStrXMap* map, const char* key) {
  size_t hashval = kev_strxmap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  KevStrXMapNode* node = map->array[index].map_node_list;
  for (; node; node = node->next) {
    if (node->hashval == hashval &&
        strcmp(node->key, key) == 0)
      break;
  }
  return node;
}

KevStrXMapNode* kev_strxmap_iterate_next(KevStrXMap* map, KevStrXMapNode* current) {
  if (current->next) return current->next;
  size_t index = (map->capacity - 1) & current->hashval;
  KevStrXMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}
