#include "kevlr/include/hashmap/goto_map.h"
#include "kevlr/include/object_pool/gotomap_node_pool.h"

#include <stdlib.h>

inline static size_t kev_gotomap_hashing(void* key) {
  return (size_t)key >> 3;
}

static void kev_gotomap_rehash(KevGotoMap* to, KevGotoMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevGotoMapBucket* from_array = from->array;
  KevGotoMapBucket* to_array = to->array;
  KevGotoMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevGotoMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevGotoMapNode* tmp = node->next;
      size_t hashval = kev_gotomap_hashing(node->key);
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

static bool kev_gotomap_expand(KevGotoMap* map) {
  KevGotoMap new_map;
  if (!kev_gotomap_init(&new_map, map->capacity << 1))
    return false;
  kev_gotomap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_gotomap_bucket_free(KevGotoMapBucket* bucket) {
  KevGotoMapNode* node = bucket->map_node_list;
  while (node) {
    KevGotoMapNode* tmp = node->next;
    kev_gotomap_node_pool_deallocate(node);
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

bool kev_gotomap_init(KevGotoMap* map, size_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KevGotoMapBucket* array = (KevGotoMapBucket*)malloc(sizeof (KevGotoMapBucket) * capacity);
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

KevGotoMap* kev_gotomap_create(size_t capacity) {
  KevGotoMap* map = (KevGotoMap*)malloc(sizeof (KevGotoMap));
  if (!map || !kev_gotomap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
}

void kev_gotomap_destroy(KevGotoMap* map) {
  if (map) {
    KevGotoMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KevGotoMapBucket* tmp = bucket->next;
      kev_gotomap_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

void kev_gotomap_delete(KevGotoMap* map) {
  kev_gotomap_destroy(map);
  free(map);
}

void kev_gotomap_make_empty(KevGotoMap* map) {
  KevGotoMapBucket* bucket = map->bucket_head;
  while (bucket) {
    KevGotoMapBucket* tmp = bucket->next;
    kev_gotomap_bucket_free(bucket);
    bucket = tmp;
  }
  
  map->bucket_head = NULL;
  map->size = 0;
}

bool kev_gotomap_insert(KevGotoMap* map, KevSymbol* key, KevItemSet* value) {
  if (map->size >= map->capacity && !kev_gotomap_expand(map))
    return false;

  KevGotoMapNode* new_node = kev_gotomap_node_pool_allocate();
  if (!new_node) return false;

  size_t index = (map->capacity - 1) & kev_gotomap_hashing(key);
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

KevGotoMapNode* kev_gotomap_search(KevGotoMap* map, KevSymbol* key) {
  size_t index = (map->capacity - 1) & kev_gotomap_hashing(key);
  KevGotoMapNode* node = map->array[index].map_node_list;
  while (node) {
    if (key == node->key) break;
    node = node->next;
  }
  return node;
}

KevGotoMapNode* kev_gotomap_iterate_next(KevGotoMap* map, KevGotoMapNode* current) {
  if (current->next) return current->next;
  size_t index = (map->capacity - 1) & kev_gotomap_hashing(current->key);
  KevGotoMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}
