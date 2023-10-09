#ifndef KEVCC_UTILS_INCLUDE_HASHMAP_STR_MAP_H
#define KEVCC_UTILS_INCLUDE_HASHMAP_STR_MAP_H

#include "utils/include/general/global_def.h"

typedef struct tagKevStringMapNode {
  char* key;
  char* value;
  size_t hashval;
  struct tagKevStringMapNode* next;
} KevStringMapNode;

typedef struct tagKevStringMapBucket {
  KevStringMapNode* map_node_list;
  struct tagKevStringMapBucket* next;
} KevStringMapBucket;

typedef struct tagKevStringMap {
  KevStringMapBucket* array;
  KevStringMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevStringMap;

bool kev_strmap_init(KevStringMap* map, size_t capacity);
KevStringMap* kev_strmap_create(size_t capacity);
void kev_strmap_destroy(KevStringMap* map);
void kev_strmap_delete(KevStringMap* map);

bool kev_strmap_insert(KevStringMap* map, const char* key, const char* value);
bool kev_strmap_insert_move(KevStringMap* map, const char* key, char* value);
KevStringMapNode* kev_strmap_search(KevStringMap* map, const char* key);
bool kev_strmap_update(KevStringMap* map, const char* key, const char* value);
static inline char* kev_strmap_node_swap_value(KevStringMapNode* mapnode, char* value);
bool kev_strmap_update_move(KevStringMap* map, const char* key, char* value);

static inline const char* kev_strmap_node_get_key(KevStringMapNode* node);
static inline const char* kev_strmap_node_get_value(KevStringMapNode* node);

bool kev_strmap_node_set_value(KevStringMapNode* node, const char* value);
void kev_strmap_node_set_value_move(KevStringMapNode* node, char* value);

static inline KevStringMapNode* kev_strmap_iterate_begin(KevStringMap* map);
KevStringMapNode* kev_strmap_iterate_next(KevStringMap* map, KevStringMapNode* current);

static inline KevStringMapNode* kev_strmap_iterate_begin(KevStringMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

static inline char* kev_strmap_node_swap_value(KevStringMapNode* mapnode, char* value) {
  char* ret = mapnode->value;
  mapnode->value = value;
  return ret;
}

static inline const char* kev_strmap_node_get_value(KevStringMapNode* node) {
  return node->value;
}

static inline const char* kev_strmap_node_get_key(KevStringMapNode* node) {
  return node->key;
}

#endif
