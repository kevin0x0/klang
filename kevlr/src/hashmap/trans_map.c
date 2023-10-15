#include "kevlr/include/hashmap/trans_map.h"
#include "kevlr/include/object_pool/transmap_node_pool.h"

#include <stdlib.h>

inline static size_t klr_gotomap_hashing(void* key) {
  return (size_t)key >> 3;
}

static void klr_gotomap_rehash(KlrTransMap* to, KlrTransMap* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KlrTransMapBucket* from_array = from->array;
  KlrTransMapBucket* to_array = to->array;
  KlrTransMapBucket* bucket_head = NULL;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KlrTransMapNode* node = from_array[i].map_node_list;
    while (node) {
      KlrTransMapNode* tmp = node->next;
      size_t hashval = klr_gotomap_hashing(node->key);
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

static bool klr_gotomap_expand(KlrTransMap* map) {
  KlrTransMap new_map;
  if (!klr_transmap_init(&new_map, map->capacity << 1))
    return false;
  klr_gotomap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

static void klr_gotomap_bucket_free(KlrTransMapBucket* bucket) {
  KlrTransMapNode* node = bucket->map_node_list;
  while (node) {
    KlrTransMapNode* tmp = node->next;
    klr_transmap_node_pool_deallocate(node);
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

bool klr_transmap_init(KlrTransMap* map, size_t capacity) {
  if (!map) return false;

  map->bucket_head = NULL;
  capacity = pow_of_2_above(capacity);
  KlrTransMapBucket* array = (KlrTransMapBucket*)malloc(sizeof (KlrTransMapBucket) * capacity);
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

KlrTransMap* klr_transmap_create(size_t capacity) {
  KlrTransMap* map = (KlrTransMap*)malloc(sizeof (KlrTransMap));
  if (!map || !klr_transmap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
}

void klr_transmap_destroy(KlrTransMap* map) {
  if (map) {
    KlrTransMapBucket* bucket = map->bucket_head;
    while (bucket) {
      KlrTransMapBucket* tmp = bucket->next;
      klr_gotomap_bucket_free(bucket);
      bucket = tmp;
    }
    free(map->array);
    map->bucket_head = NULL;
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
  }
}

void klr_transmap_delete(KlrTransMap* map) {
  klr_transmap_destroy(map);
  free(map);
}

void klr_transmap_make_empty(KlrTransMap* map) {
  KlrTransMapBucket* bucket = map->bucket_head;
  while (bucket) {
    KlrTransMapBucket* tmp = bucket->next;
    klr_gotomap_bucket_free(bucket);
    bucket = tmp;
  }
  
  map->bucket_head = NULL;
  map->size = 0;
}

bool klr_transmap_insert(KlrTransMap* map, KlrSymbol* key, KlrItemSet* value) {
  if (map->size >= map->capacity && !klr_gotomap_expand(map))
    return false;

  KlrTransMapNode* new_node = klr_transmap_node_pool_allocate();
  if (!new_node) return false;

  size_t index = (map->capacity - 1) & klr_gotomap_hashing(key);
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

KlrTransMapNode* klr_transmap_search(KlrTransMap* map, KlrSymbol* key) {
  size_t index = (map->capacity - 1) & klr_gotomap_hashing(key);
  KlrTransMapNode* node = map->array[index].map_node_list;
  while (node) {
    if (key == node->key) break;
    node = node->next;
  }
  return node;
}

KlrTransMapNode* klr_transmap_iterate_next(KlrTransMap* map, KlrTransMapNode* current) {
  if (current->next) return current->next;
  size_t index = (map->capacity - 1) & klr_gotomap_hashing(current->key);
  KlrTransMapBucket* current_bucket = &map->array[index];
  if (current_bucket->next)
    return current_bucket->next->map_node_list;
  return NULL;
}
