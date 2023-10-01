#include "kevlr/include/hashmap/priority_map.h"

#include <stdlib.h>

inline static size_t kev_priomap_hashing(KevSymbol* symbol, KevPrioPos pos) {
  return ((size_t)symbol) + pos;
}

static void kev_priomap_rehash(KevPrioMap* to, KevPrioMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevPrioMapBucket* from_array = from->array;
  KevPrioMapBucket* to_array = to->array;
  KevPrioMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevPrioMapNode* node = from_array[i].map_node_list;
    while (node) {
      KevPrioMapNode* tmp = node->next;
      size_t hashval = kev_priomap_hashing(node->symbol, node->pos);
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

static bool kev_priomap_expand(KevPrioMap* map) {
  KevPrioMap new_map;
  if (!kev_priomap_init(&new_map, map->capacity << 1))
    return false;
  kev_priomap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void kev_priomap_bucket_free(KevPrioMapBucket* bucket) {
  KevPrioMapNode* node = bucket->map_node_list;
  while (node) {
    KevPrioMapNode* tmp = node->next;
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

bool kev_priomap_init(KevPrioMap* map, size_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KevPrioMapBucket* array = (KevPrioMapBucket*)malloc(sizeof (KevPrioMapBucket) * capacity);
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

KevPrioMap* kev_priomap_create(size_t capacity) {
  KevPrioMap* map = (KevPrioMap*)malloc(sizeof (KevPrioMap));
  if (!map || !kev_priomap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
}

void kev_priomap_destroy(KevPrioMap* map) {
  if (map) {
    KevPrioMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KevPrioMapBucket* tmp = bucket->next;
      kev_priomap_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

void kev_priomap_delete(KevPrioMap* map) {
  kev_priomap_destroy(map);
  free(map);
}

void kev_priomap_make_empty(KevPrioMap* map) {
  KevPrioMapBucket* bucket = map->bucket_head;
  while (bucket) {
    KevPrioMapBucket* tmp = bucket->next;
    kev_priomap_bucket_free(bucket);
    bucket = tmp;
  }
  
  map->bucket_head = NULL;
  map->size = 0;
}

bool kev_priomap_insert(KevPrioMap* map, KevSymbol* symbol, KevPrioPos pos, size_t priority) {
  if (map->size >= map->capacity && !kev_priomap_expand(map))
    return false;

  KevPrioMapNode* new_node = (KevPrioMapNode*)malloc(sizeof (KevPrioMapNode));
  if (!new_node) return false;

  size_t index = (map->capacity - 1) & kev_priomap_hashing(symbol, pos);
  new_node->symbol = symbol;
  new_node->pos = pos;
  new_node->priority = priority;
  new_node->next = map->array[index].map_node_list;
  map->array[index].map_node_list = new_node;
  map->size++;
  if (new_node->next == NULL) {
    map->array[index].next = map->bucket_head;
    map->bucket_head = &map->array[index];
  }
  return true;
}

KevPrioMapNode* kev_priomap_search(KevPrioMap* map, KevSymbol* symbol, KevPrioPos pos) {
  size_t index = (map->capacity - 1) & kev_priomap_hashing(symbol, pos);
  KevPrioMapNode* node = map->array[index].map_node_list;
  while (node) {
    if (symbol == node->symbol && pos == node->pos) break;
    node = node->next;
  }
  return node;
}

KevPrioMapNode* kev_priomap_iterate_next(KevPrioMap* map, KevPrioMapNode* current) {
  if (current->next) return current->next;
  size_t index = (map->capacity - 1) & kev_priomap_hashing(current->symbol, current->pos);
  KevPrioMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}
