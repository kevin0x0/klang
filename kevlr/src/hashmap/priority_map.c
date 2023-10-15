#include "kevlr/include/hashmap/priority_map.h"

#include <stdlib.h>

inline static size_t klr_priomap_hashing(KlrSymbol* symbol, KlrPrioPos pos) {
  return ((size_t)symbol) + pos;
}

static void klr_priomap_rehash(KlrPrioMap* to, KlrPrioMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KlrPrioMapBucket* from_array = from->array;
  KlrPrioMapBucket* to_array = to->array;
  KlrPrioMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KlrPrioMapNode* node = from_array[i].map_node_list;
    while (node) {
      KlrPrioMapNode* tmp = node->next;
      size_t hashval = klr_priomap_hashing(node->symbol, node->pos);
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

static bool klr_priomap_expand(KlrPrioMap* map) {
  KlrPrioMap new_map;
  if (!klr_priomap_init(&new_map, map->capacity << 1))
    return false;
  klr_priomap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void klr_priomap_bucket_free(KlrPrioMapBucket* bucket) {
  KlrPrioMapNode* node = bucket->map_node_list;
  while (node) {
    KlrPrioMapNode* tmp = node->next;
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

bool klr_priomap_init(KlrPrioMap* map, size_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KlrPrioMapBucket* array = (KlrPrioMapBucket*)malloc(sizeof (KlrPrioMapBucket) * capacity);
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

KlrPrioMap* klr_priomap_create(size_t capacity) {
  KlrPrioMap* map = (KlrPrioMap*)malloc(sizeof (KlrPrioMap));
  if (!map || !klr_priomap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
}

void klr_priomap_destroy(KlrPrioMap* map) {
  if (map) {
    KlrPrioMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KlrPrioMapBucket* tmp = bucket->next;
      klr_priomap_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

void klr_priomap_delete(KlrPrioMap* map) {
  klr_priomap_destroy(map);
  free(map);
}

void klr_priomap_make_empty(KlrPrioMap* map) {
  KlrPrioMapBucket* bucket = map->bucket_head;
  while (bucket) {
    KlrPrioMapBucket* tmp = bucket->next;
    klr_priomap_bucket_free(bucket);
    bucket = tmp;
  }
  
  map->bucket_head = NULL;
  map->size = 0;
}

bool klr_priomap_insert(KlrPrioMap* map, KlrSymbol* symbol, KlrPrioPos pos, size_t priority) {
  if (map->size >= map->capacity && !klr_priomap_expand(map))
    return false;

  KlrPrioMapNode* new_node = (KlrPrioMapNode*)malloc(sizeof (KlrPrioMapNode));
  if (!new_node) return false;

  size_t index = (map->capacity - 1) & klr_priomap_hashing(symbol, pos);
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

KlrPrioMapNode* klr_priomap_search(KlrPrioMap* map, KlrSymbol* symbol, KlrPrioPos pos) {
  size_t index = (map->capacity - 1) & klr_priomap_hashing(symbol, pos);
  KlrPrioMapNode* node = map->array[index].map_node_list;
  while (node) {
    if (symbol == node->symbol && pos == node->pos) break;
    node = node->next;
  }
  return node;
}

KlrPrioMapNode* klr_priomap_iterate_next(KlrPrioMap* map, KlrPrioMapNode* current) {
  if (current->next) return current->next;
  size_t index = (map->capacity - 1) & klr_priomap_hashing(current->symbol, current->pos);
  KlrPrioMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}
