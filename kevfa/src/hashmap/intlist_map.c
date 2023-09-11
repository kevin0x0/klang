#include "kevfa/include/hashmap/intlist_map.h"
#include "kevfa/include/object_pool/intlistmap_node_pool.h"

#include <stdlib.h>

inline static size_t kev_intlistmap_hashing(size_t key) {
  return (size_t)key;
}

static void kev_intlistmap_rehash(KevIntListMap* to, KevIntListMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevIntListMapBucket* from_array = from->array;
  KevIntListMapBucket* to_array = to->array;
  KevIntListMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevIntListMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevIntListMapNode* tmp = node->next;
      size_t hashval = kev_intlistmap_hashing(node->key);
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

static bool kev_intlistmap_expand(KevIntListMap* map) {
  KevIntListMap new_map;
  if (!kev_intlistmap_init(&new_map, map->capacity << 1))
    return false;
  kev_intlistmap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_intlist_map_bucket_free(KevIntListMapBucket* bucket) {
  KevIntListMapNode* node = bucket->map_node_list;
  while (node) {
    KevIntListMapNode* tmp = node->next;
    kev_intlistmap_node_pool_deallocate(node);
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

bool kev_intlistmap_init(KevIntListMap* map, size_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KevIntListMapBucket* array = (KevIntListMapBucket*)malloc(sizeof (KevIntListMapBucket) * capacity);
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

void kev_intlistmap_destroy(KevIntListMap* map) {
  if (map) {
    KevIntListMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KevIntListMapBucket* tmp = bucket->next;
      kev_intlist_map_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

KevIntListMapNode* kev_intlistmap_insert(KevIntListMap* map, size_t key, KevNodeList* value) {
  if (map->size >= map->capacity && !kev_intlistmap_expand(map))
    return NULL;

  KevIntListMapNode* new_node = kev_intlistmap_node_pool_allocate();
  if (!new_node) return NULL;

  size_t index = (map->capacity - 1) & kev_intlistmap_hashing(key);
  new_node->key = key;
  new_node->value = value;
  new_node->next = map->array[index].map_node_list;
  map->array[index].map_node_list = new_node;
  map->size++;
  if (new_node->next == NULL) {
    map->array[index].next = map->bucket_head;
    map->bucket_head = &map->array[index];
  }
  return new_node;
}

KevIntListMapNode* kev_intlistmap_search(KevIntListMap* map, size_t key) {
  size_t index = (map->capacity - 1) & kev_intlistmap_hashing(key);
  KevIntListMapNode* node = map->array[index].map_node_list;
  for (; node; node = node->next) {
    if (node->key == key)
      break;
  }
  return node;
}

void kev_intlistmap_make_empty(KevIntListMap* map) {
  KevIntListMapBucket* bucket = map->bucket_head;
  while (bucket) {
    KevIntListMapBucket* tmp = bucket->next;
    kev_intlist_map_bucket_free(bucket);
    bucket = tmp;
  }
  
  map->bucket_head = NULL;
  map->size = 0;
}

KevIntListMapNode* kev_intlistmap_iterate_next(KevIntListMap* map, KevIntListMapNode* current) {
  if (current->next) return current->next;
  size_t index = (map->capacity - 1) & kev_intlistmap_hashing(current->key);
  KevIntListMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}


